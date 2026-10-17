// Minimal shim for fmt::format and FMT_STRING to forward to std::format
#pragma once

#include <string>
#include <format>

namespace fmt {
    template <typename... Args>
    inline std::string format(const std::string_view fmt, Args&&... args)
    {
        return std::format(fmt, std::forward<Args>(args)...);
    }
}

#define FMT_STRING(x) x
