#pragma once
#include <cstring>
#include <map>
#include <string>

// In-memory NVS stub. Uses global stores keyed by namespace so multiple
// Preferences instances within the same namespace share state (mirrors real NVS).
namespace pref_store {
    inline std::map<std::string, std::map<std::string, int32_t>>& ints() {
        static std::map<std::string, std::map<std::string, int32_t>> s;
        return s;
    }
    inline std::map<std::string, std::map<std::string, std::string>>& strs() {
        static std::map<std::string, std::map<std::string, std::string>> s;
        return s;
    }
}

class Preferences {
    std::string ns_;
    bool ro_ = false;
public:
    bool begin(const char* ns, bool readonly = false) { ns_ = ns; ro_ = readonly; return true; }
    void end() {}

    int32_t getInt(const char* k, int32_t def = 0) const {
        auto& m = pref_store::ints()[ns_];
        auto it = m.find(k);
        return it != m.end() ? it->second : def;
    }
    void putInt(const char* k, int32_t v) { if (!ro_) pref_store::ints()[ns_][k] = v; }

    size_t getString(const char* k, char* buf, size_t sz) const {
        auto& m = pref_store::strs()[ns_];
        auto it = m.find(k);
        if (it == m.end()) return 0;
        const std::string& s = it->second;
        size_t n = s.size() < sz - 1 ? s.size() : sz - 1;
        memcpy(buf, s.c_str(), n);
        buf[n] = '\0';
        return n;
    }
    void putString(const char* k, const char* v) { if (!ro_) pref_store::strs()[ns_][k] = v; }

    bool clear() {
        if (ro_) return false;
        pref_store::ints()[ns_].clear();
        pref_store::strs()[ns_].clear();
        return true;
    }
};
