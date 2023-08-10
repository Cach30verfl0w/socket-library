#ifdef PLATFORM_APPLE
#include "sockslib/resolve.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace sockslib {
    auto resolve_address(std::string domain) -> kstd::Result<std::string> {
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
            return std::string(inet_ntoa(*(struct in_addr*)hostent->h_addr));
        }
    }
}
#endif