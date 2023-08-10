#pragma once
#include <string>

namespace sockslib {
    [[nodiscard]] auto get_last_error() -> std::string;
}