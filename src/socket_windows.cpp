#ifdef PLATFORM_WINDOWS
#include "sockslib/socket.hpp"

#include <fmt/format.h>
#include <stdexcept>

#include <WS2tcpip.h>

namespace sockslib {
    kstd::atomic_usize Socket::_socket_count = 0;// NOLINT

    [[nodiscard]] auto get_last_error() -> std::string {
        const auto error_code = ::GetLastError();

        if(error_code == 0) {
            return "";
        }

        LPWSTR buffer = nullptr;
        constexpr auto lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        const auto new_length = ::FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                error_code, lang_id, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
        auto message = kstd::utils::to_mbs({buffer, new_length});
        LocalFree(buffer);

        return fmt::format("ERROR 0x{:X}: {}", error_code, message);
    }

    Socket::Socket(ProtocolType protocol_type, kstd::u16 buffer_size) :// NOLINT
            _protocol_type {protocol_type},
            _buffer_size {buffer_size} {
        if(_socket_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                throw std::runtime_error(fmt::format("Unable to initialize Socket => {}", get_last_error()));
            }
        }
        _socket_count++;
    }

    ServerSocket::ServerSocket(kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size) :
            Socket {protocol_type, buffer_size} {
        using namespace std::string_literals;

        // Configure address information hints
        ADDRINFOW hints {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = static_cast<int>(protocol_type);
        switch(protocol_type) {
            case ProtocolType::TCP: hints.ai_protocol = IPPROTO_TCP; break;
            case ProtocolType::UDP: hints.ai_protocol = IPPROTO_UDP; break;
        }
        hints.ai_flags = AI_PASSIVE;

        // Request address information
        if(FAILED(GetAddrInfoW(nullptr, std::to_wstring(port).c_str(), &hints, &_addr_info))) {
            if(Socket::_socket_count == 1) {
                WSACleanup();
                --Socket::_socket_count;

                throw std::runtime_error(
                        fmt::format("Unable to initialize server (Address info resolve failed) => ", get_last_error()));
            }
        }

        // Create the socket and do validation check
        _socket_handle = socket(_addr_info->ai_family, _addr_info->ai_socktype, _addr_info->ai_protocol);
        if(handle_invalid(_socket_handle)) {
            if(Socket::_socket_count == 1) {
                WSACleanup();
            }

            FreeAddrInfoW(_addr_info);
            _addr_info = nullptr;
            --Socket::_socket_count;

            throw std::runtime_error(
                    fmt::format("Unable to initialize server (Socket creation failed) => {}", get_last_error()));
        }

        // Bind the socket
        if(FAILED(::bind(_socket_handle, _addr_info->ai_addr, static_cast<int>(_addr_info->ai_addrlen)))) {
            if(Socket::_socket_count == 1) {
                WSACleanup();
            }

            FreeAddrInfoW(_addr_info);
            closesocket(_socket_handle);
            _addr_info = nullptr;
            --Socket::_socket_count;

            throw std::runtime_error(
                    fmt::format("Unable to initialize server (Socket binding failed) => {}", get_last_error()));
        }

        // Call the listen function if the socket is using TCP
        if(protocol_type == ProtocolType::TCP) {
            if(FAILED(listen(_socket_handle, SOMAXCONN))) {
                if(Socket::_socket_count == 1) {
                    WSACleanup();
                }

                FreeAddrInfoW(_addr_info);
                closesocket(_socket_handle);
                _addr_info = nullptr;
                --Socket::_socket_count;

                auto last_error = get_last_error();
                if(last_error.empty()) {
                    throw std::runtime_error(
                            "Unable to initialize server (Socket binding failed) => Listen call with socket failed!"s);
                }
                throw std::runtime_error(
                        fmt::format("Unable to initialize server (Socket binding failed) => {}", get_last_error()));
            }
        }
    }

    ServerSocket::ServerSocket(ServerSocket&& other) noexcept :// NOLINT
            Socket {},
            _addr_info {other._addr_info} {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        _buffer_size = other._buffer_size;
        other._socket_handle = 0;
        other._addr_info = nullptr;
        ++Socket::_socket_count;
    }

    ServerSocket::~ServerSocket() noexcept {
        // Free the address information if valid
        if(_addr_info != nullptr) {
            FreeAddrInfoW(_addr_info);
        }

        // Close the socket if valid
        if(handle_valid(_socket_handle)) {
            shutdown(_socket_handle, SD_SEND);
            closesocket(_socket_handle);
        }

        // Cleanup WSA if socket count is 1
        if(_socket_count == 1) {
            WSACleanup();
        }

        // Decrement socket count if socket count is higher than 0
        if(Socket::_socket_count > 0) {
            --Socket::_socket_count;
        }
    }

    auto ServerSocket::operator=(ServerSocket&& other) noexcept -> ServerSocket& {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        _buffer_size = other._buffer_size;
        _addr_info = other._addr_info;

        other._socket_handle = 0;
        other._addr_info = nullptr;
        ++Socket::_socket_count;
        return *this;
    }

    ClientSocket::ClientSocket(std::string address, kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size) :
            Socket {protocol_type, buffer_size} {
        using namespace std::string_literals;

        // Specify protocol
        int protocol = 0;
        switch(protocol_type) {
            case ProtocolType::TCP: protocol = IPPROTO_TCP; break;
            case ProtocolType::UDP: protocol = IPPROTO_UDP; break;
        }

        // Create socket handle and validate it
        _socket_handle = socket(AF_INET, static_cast<int>(protocol_type), protocol);
        if(handle_invalid(_socket_handle)) {
            if(Socket::_socket_count == 1) {
                WSACleanup();
            }
            --Socket::_socket_count;
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Generate SOCKADDR
        SOCKADDR_IN addr {};
        if(FAILED(InetPton(AF_INET, address.c_str(), &addr.sin_addr.s_addr))) {
            throw std::runtime_error {
                    "Unable to connect with socket => Failed conversion of literal address to binary address!"s};
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        // Establish connection
        if(FAILED(connect(_socket_handle, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)))) {
            // Free address info and close socket
            closesocket(_socket_handle);

            // Cleanup WSA and decrement socket count
            if(_socket_count == 1) {
                WSACleanup();
            }

            if(_socket_count > 0) {
                --_socket_count;
            }

            auto last_error = get_last_error();
            if(last_error.empty()) {
                throw std::runtime_error {
                        "Unable to connect with socket => Establishment of connection with server failed! Maybe the server isn't reachable?"s};
            }
            throw std::runtime_error {fmt::format("Unable to connect with socket => {}", last_error)};
        }
    }

    ClientSocket::ClientSocket(sockslib::ClientSocket&& other) noexcept :
            Socket {} {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        _buffer_size = other._buffer_size;
        other.Socket::_socket_handle = 0;
        ++Socket::_socket_count;
    }

    ClientSocket::~ClientSocket() noexcept {
        // Close the socket if valid
        if(handle_valid(_socket_handle)) {
            shutdown(_socket_handle, SD_SEND);
            closesocket(_socket_handle);
        }

        // Cleanup WSA if socket count is 1
        if(_socket_count == 1) {
            WSACleanup();
        }

        // Decrement socket count if socket count is higher than 0
        if(Socket::_socket_count > 0) {
            --Socket::_socket_count;
        }
    }

    auto ClientSocket::operator=(ClientSocket&& other) noexcept -> ClientSocket& {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        _buffer_size = other._buffer_size;
        other.Socket::_socket_handle = 0;
        ++Socket::_socket_count;
        return *this;
    }
}// namespace sockslib
#endif