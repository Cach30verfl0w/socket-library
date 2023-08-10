#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <kstd/utils.hpp>
#include <fmt/format.h>
#include <Windows.h>

#include "sockslib/utils.hpp"

namespace sockslib {
    [[nodiscard]] auto get_last_error() -> std::string {
        const auto error_code = ::GetLastError();

        if(error_code == 0) {
            return "";
        }

        LPWSTR buffer = nullptr;
        constexpr auto lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        const auto new_length = ::FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                error_code, lang_id, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);// NOLINT
        auto message = kstd::utils::to_mbs({buffer, new_length});
        LocalFree(buffer);

        return fmt::format("ERROR 0x{:X}: {}", error_code, message);
    }
}
#endif