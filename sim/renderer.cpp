#include "renderer.h"
#include "framebuffer.h"
#include "config.h"

#include <GLES3/gl3.h>
#include <emscripten/html5.h>
#include <cmath>
#include <cstdio>

static constexpr float TAU = 6.283185307179586f;

static const char* VERT_SRC = R"(#version 300 es
in vec2 aPos;
out vec2 vUv;
void main() {
    vUv = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* FRAG_SRC = R"(#version 300 es
precision highp float;
precision highp int;

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uFb;
uniform int uNumSlices;
uniform int uNumLeds;
uniform float uHubFrac;
uniform float uGapFrac;
uniform int uPhaseSlices;
uniform float uArmAngle;
uniform float uArmSweep;
uniform int uNumArms;

const float TAU = 6.283185307179586;
const vec3 BG = vec3(10.0 / 255.0);

void main() {
    vec2 c = vec2(vUv.x * 2.0 - 1.0, 1.0 - vUv.y * 2.0);
    float dist = length(c);
    float innerR = uHubFrac;

    if (dist < innerR || dist > 1.0) {
        fragColor = vec4(BG, 1.0);
        return;
    }

    float ledH = (1.0 - innerR) / float(uNumLeds);
    int led = min(int(floor((dist - innerR) / ledH)), uNumLeds - 1);

    float frac = (dist - innerR) / ledH - float(led);
    float halfGap = uGapFrac * 0.5;
    if (frac < halfGap || frac > 1.0 - halfGap) {
        fragColor = vec4(BG, 1.0);
        return;
    }

    float angle = atan(c.y, c.x);
    if (angle < 0.0) angle += TAU;

    if (uArmSweep > 0.0 && uArmSweep < TAU) {
        float armSpacing = TAU / float(uNumArms);
        float angleBehind = uArmAngle - angle;
        if (angleBehind < 0.0) angleBehind += TAU;
        float closestBehind = mod(angleBehind, armSpacing);
        if (closestBehind > uArmSweep) {
            fragColor = vec4(BG, 1.0);
            return;
        }
    }

    int nomSlice = int(floor(angle / TAU * float(uNumSlices))) % uNumSlices;
    int fbSlice = ((nomSlice + uPhaseSlices) % uNumSlices + uNumSlices) % uNumSlices;

    float u = (float(led) + 0.5) / float(uNumLeds);
    float v = (float(fbSlice) + 0.5) / float(uNumSlices);
    vec4 t = texture(uFb, vec2(u, v));

    int br5 = int(t.r * 255.0) & 0x1F;
    float scale = float(br5) / 31.0;
    fragColor = vec4(t.a * scale, t.b * scale, t.g * scale, 1.0);
}
)";

static const char* OVERLAY_VERT_SRC = R"(#version 300 es
in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* OVERLAY_FRAG_SRC = R"(#version 300 es
precision highp float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

static struct {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = 0;
    int width  = 0;
    int height = 0;

    GLuint mainProg    = 0;
    GLuint overlayProg = 0;
    GLuint quadVao     = 0;
    GLuint lineVao     = 0;
    GLuint lineVbo     = 0;
    GLuint fbTex       = 0;
    int    fbTexW      = 0;
    int    fbTexH      = 0;

    struct {
        GLint fb, numSlices, numLeds, hubFrac, gapFrac;
        GLint phaseSlices, armAngle, armSweep, numArms;
    } loc;

    struct {
        GLint color;
    } overlayLoc;

    float hubFraction    = 0.0f;
    float gapFraction    = 0.54f;
    bool  showOverruns   = true;
    bool  showHallMarker = true;
    bool  showSliceGrid  = false;
    int   numArms        = 1;
} g;

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(sh, sizeof(buf), nullptr, buf);
        printf("Shader compile error: %s\n", buf);
    }
    return sh;
}

static GLuint link_program(const char* vertSrc, const char* fragSrc) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        printf("Program link error: %s\n", buf);
    }
    return prog;
}

static void init_quad() {
    float verts[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
    GLuint vbo;
    glGenVertexArrays(1, &g.quadVao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(g.quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    GLint loc = glGetAttribLocation(g.mainProg, "aPos");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

static void init_line_vao() {
    glGenVertexArrays(1, &g.lineVao);
    glGenBuffers(1, &g.lineVbo);
    glBindVertexArray(g.lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, g.lineVbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    GLint loc = glGetAttribLocation(g.overlayProg, "aPos");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

static GLuint create_texture() {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

bool renderer_init() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.antialias = false;
    attrs.alpha = false;

    g.ctx = emscripten_webgl_create_context("#pov-canvas", &attrs);
    if (g.ctx <= 0) {
        printf("Failed to create WebGL2 context\n");
        return false;
    }
    emscripten_webgl_make_context_current(g.ctx);

    g.mainProg = link_program(VERT_SRC, FRAG_SRC);
    g.loc.fb         = glGetUniformLocation(g.mainProg, "uFb");
    g.loc.numSlices  = glGetUniformLocation(g.mainProg, "uNumSlices");
    g.loc.numLeds    = glGetUniformLocation(g.mainProg, "uNumLeds");
    g.loc.hubFrac    = glGetUniformLocation(g.mainProg, "uHubFrac");
    g.loc.gapFrac    = glGetUniformLocation(g.mainProg, "uGapFrac");
    g.loc.phaseSlices = glGetUniformLocation(g.mainProg, "uPhaseSlices");
    g.loc.armAngle   = glGetUniformLocation(g.mainProg, "uArmAngle");
    g.loc.armSweep   = glGetUniformLocation(g.mainProg, "uArmSweep");
    g.loc.numArms    = glGetUniformLocation(g.mainProg, "uNumArms");

    g.overlayProg = link_program(OVERLAY_VERT_SRC, OVERLAY_FRAG_SRC);
    g.overlayLoc.color = glGetUniformLocation(g.overlayProg, "uColor");

    init_quad();
    init_line_vao();
    g.fbTex = create_texture();

    return true;
}

void renderer_resize(int width, int height) {
    g.width = width;
    g.height = height;
}

void renderer_set_hub_fraction(float f)    { g.hubFraction = f; }
void renderer_set_gap_fraction(float f)    { g.gapFraction = f; }
void renderer_set_show_overruns(bool v)    { g.showOverruns = v; }
void renderer_set_show_hall_marker(bool v) { g.showHallMarker = v; }
void renderer_set_show_slice_grid(bool v)  { g.showSliceGrid = v; }
void renderer_set_num_arms(int n)          { g.numArms = n; }

static void upload_fb(const Framebuffer& fb) {
    int w = fb.numLeds();
    int h = fb.numSlices();
    const uint8_t* data = (const uint8_t*)fb.getSlice(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g.fbTex);

    if (g.fbTexW != w || g.fbTexH != h) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        g.fbTexW = w;
        g.fbTexH = h;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                        GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
}

static void draw_line(float x0, float y0, float x1, float y1,
                      float r, float gr, float b, float a) {
    glUseProgram(g.overlayProg);
    glUniform4f(g.overlayLoc.color, r, gr, b, a);

    float verts[] = { x0, y0, x1, y1 };
    glBindVertexArray(g.lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, g.lineVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

static void draw_overlays(float hallOffsetAngle, bool hasOverruns, int numSlices) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (g.showHallMarker) {
        float cx = 0.0f, cy = 0.0f;
        float ex = cosf(hallOffsetAngle);
        float ey = sinf(hallOffsetAngle);
        draw_line(cx, cy, ex, ey, 0.0f, 1.0f, 0.0f, 0.6f);
    }

    if (g.showSliceGrid) {
        int step = numSlices > 36 ? numSlices / 36 : 1;
        float innerR = g.hubFraction;
        for (int i = 0; i < numSlices; i += step) {
            float angle = (float)i / numSlices * TAU;
            float c = cosf(angle), s = sinf(angle);
            draw_line(c * innerR, s * innerR, c, s,
                      1.0f, 1.0f, 1.0f, 0.15f);
        }
    }

    glDisable(GL_BLEND);
}

void renderer_render(const Framebuffer& fb, const Config& cfg,
                     float armAngle, float armSweep,
                     float hallOffsetAngle, bool hasOverruns,
                     int numSlices) {
    emscripten_webgl_make_context_current(g.ctx);
    glViewport(0, 0, g.width, g.height);
    upload_fb(fb);

    int phaseSlices = (int)roundf(((float)cfg.phaseOffset / 360.0f) * numSlices);

    glUseProgram(g.mainProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g.fbTex);
    glUniform1i(g.loc.fb, 0);
    glUniform1i(g.loc.numSlices, numSlices);
    glUniform1i(g.loc.numLeds, fb.numLeds());
    glUniform1f(g.loc.hubFrac, g.hubFraction);
    glUniform1f(g.loc.gapFrac, g.gapFraction);
    glUniform1i(g.loc.phaseSlices, phaseSlices);
    glUniform1f(g.loc.armAngle, armAngle);
    glUniform1f(g.loc.armSweep, armSweep);
    glUniform1i(g.loc.numArms, g.numArms);

    glBindVertexArray(g.quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    draw_overlays(hallOffsetAngle, hasOverruns, numSlices);
}
