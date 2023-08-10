#ifdef PLATFORM_LINUX
#include "sockslib/resolve.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>

namespace sockslib {
    auto resolve_address(std::string domain, kstd::usize timeout) -> kstd::Result<std::string> {
        using namespace std::string_literals;
        struct hostent *hostent = gethostbyname(domain.data());
        if (hostent == nullptr) {
            switch(h_errno) {
                case HOST_NOT_FOUND: return kstd::Error { "The hsot was not found"s };
                case NO_ADDRESS: return kstd::Error { "No address found with the domain"s };
                case NO_RECOVERY: return kstd::Error { "A non-recoverable name server error occurred"s };
                case TRY_AGAIN: return kstd::Error { "The name server is temporarily unavailable"s };
            }
        } else {
            unsigned char *addr = reinterpret_cast<unsigned char *>(hostent->h_addr_list[0]);

            std::stringstream stringstream;
            std::copy(addr, addr + 4, std::ostream_iterator<unsigned int>(stringstream, "."));
            return std::string(stringstream.str());
        }
    }
}
#endif