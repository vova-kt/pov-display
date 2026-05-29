#include <unity.h>
#include <cstring>
#include "web/web_ui.h"

void setUp() {}
void tearDown() {}

void test_embedded_ui_has_reboot_control() {
    TEST_ASSERT_NOT_NULL(strstr(INDEX_HTML, "Reboot Device"));
    TEST_ASSERT_NOT_NULL(strstr(INDEX_HTML, "rebootDevice()"));
    TEST_ASSERT_NOT_NULL(strstr(INDEX_HTML, "/api/reboot"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_embedded_ui_has_reboot_control);
    return UNITY_END();
}
