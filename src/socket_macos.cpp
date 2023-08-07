#ifdef PLATFORM_APPLE
#include "sockslib/socket.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fmt/format.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace sockslib {
    [[nodiscard]] auto get_last_error() -> std::string {
        return fmt::format("ERROR 0x{:X}: {}", errno, strerror(errno));
    }

    Socket::Socket(ProtocolType protocol_type, kstd::u16 buffer_size) :// NOLINT
            _protocol_type {protocol_type},
            _buffer_size {buffer_size} {
    }

    ServerSocket::ServerSocket(kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size) :
            Socket {protocol_type, buffer_size} {
        // Create socket and validate socket
        _socket_handle = socket(PF_INET, static_cast<int>(protocol_type), 0);
        if(handle_invalid(_socket_handle)) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Force the socket to listen on the specific port
        if(setsockopt(_socket_handle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &port, sizeof(&port))) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Bind the socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htons(INADDR_ANY);
        address.sin_port = htons(port);
        if(::bind(_socket_handle, (struct sockaddr*) &address, sizeof(address)) < 0) {
            throw std::runtime_error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }
    }

    ServerSocket::ServerSocket(ServerSocket&& other) noexcept :
            Socket {} {
        Socket::_socket_handle = other._socket_handle;
        other._socket_handle = 0;
    }

    ServerSocket::~ServerSocket() noexcept {
        if(handle_valid(_socket_handle)) {
            close(_socket_handle);
        }
    }

    auto ServerSocket::operator=(ServerSocket&& other) noexcept -> ServerSocket& {
        Socket::_socket_handle = other._socket_handle;
        Socket::_protocol_type = other._protocol_type;
        other.Socket::_socket_handle = 0;
        return *this;
    }

    ClientSocket::ClientSocket(std::string address, kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size) :
            Socket {protocol_type, buffer_size} {
        using namespace std::string_literals;

        // Create socket and validate socket
        _socket_handle = socket(AF_INET, static_cast<int>(protocol_type), 0);
        if(handle_invalid(_socket_handle)) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Convert address to binary representation
        struct sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);

        if(inet_pton(AF_INET, address.c_str(), &sockaddr.sin_addr) <= 0) {
            // Free address info and close socket
            close(_socket_handle);

            throw std::runtime_error {
                    "Unable to connect with socket => Failed conversion of literal address to binary address!"s};
        }

        if(::connect(_socket_handle, reinterpret_cast<struct sockaddr*>(&sockaddr), sizeof(sockaddr)) < 0) {
            // Free address info and close socket
            close(_socket_handle);

            auto last_error = get_last_error();
            if(last_error.empty()) {
                throw std::runtime_error {
                        "Unable to connect with socket => Establishment of connection with server failed! Maybe the server isn't reachable?"s};
            }
            throw std::runtime_error {fmt::format("Unable to connect with socket => {}", last_error)};
        }
    }

    ClientSocket::~ClientSocket() noexcept {
        if(handle_valid(_socket_handle)) {
            shutdown(_socket_handle, SHUT_RDWR);
            close(_socket_handle);
        }
    }

    ClientSocket::ClientSocket(ClientSocket&& other) noexcept :
            Socket {} {
        Socket::_socket_handle = other._socket_handle;
        other._socket_handle = 0;
    }

    auto ClientSocket::operator=(ClientSocket&& other) noexcept -> ClientSocket& {
        Socket::_socket_handle = other._socket_handle;
        Socket::_protocol_type = other._protocol_type;
        other.Socket::_socket_handle = 0;
        return *this;
    }
}// namespace sockslib
#endif