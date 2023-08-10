#ifdef PLATFORM_LINUX
#include "sockslib/socket.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fmt/format.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>

namespace sockslib {
    [[nodiscard]] auto get_last_error() -> std::string {
        return fmt::format("ERROR 0x{:X}: {}", errno, strerror(errno));
    }

    Socket::Socket() :
            _socket_handle {invalid_socket_handle} {
    }

    ServerSocket::ServerSocket(const kstd::u16 port, const ProtocolType protocol_type) :
            _protocol_type {protocol_type} {
        // Create socket and validate socket
        kstd::u32 protocol = 0;
        switch(protocol_type) {
            case ProtocolType::TCP: protocol = IPPROTO_TCP; break;
            case ProtocolType::UDP: protocol = IPPROTO_UDP; break;
        }

        _socket_handle = socket(AF_INET, static_cast<int>(protocol_type), protocol);
        if(!handle_valid(_socket_handle)) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Force the socket to listen on the specific port
        if(setsockopt(_socket_handle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &port, sizeof(&port))) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Bind the socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        if(::bind(_socket_handle, (struct sockaddr*) &address, sizeof(address)) < 0) {
            throw std::runtime_error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }

        if(protocol_type != ProtocolType::UDP) {
            // Listen with the socket
            if(::listen(_socket_handle, SOMAXCONN) < 0) {
                throw std::runtime_error {fmt::format("Unable to listen with socket => {}", get_last_error())};
            }
        }
    }

    ServerSocket::ServerSocket(ServerSocket&& other) noexcept :
            _protocol_type {other._protocol_type} {
        Socket::_socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
    }

    ServerSocket::~ServerSocket() noexcept {
        if(handle_valid(_socket_handle)) {
            close(_socket_handle);
        }
    }

    auto ServerSocket::accept() const noexcept -> kstd::Result<AcceptedSocket> {
        auto accepted_socket_handle = ::accept(_socket_handle, nullptr, nullptr);
        if(!handle_valid(accepted_socket_handle)) {
            return kstd::Error {fmt::format("Unable to accept socket => {}", get_last_error())};
        }

        return AcceptedSocket {accepted_socket_handle};
    }

    auto ServerSocket::operator=(ServerSocket&& other) noexcept -> ServerSocket& {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        other._socket_handle = invalid_socket_handle;
        return *this;
    }

    ClientSocket::ClientSocket(const std::string address, const kstd::u16 port, const ProtocolType protocol_type) {
        using namespace std::string_literals;

        // Create socket and validate socket
        _socket_handle = socket(AF_INET, static_cast<int>(protocol_type), 0);
        if(!handle_valid(_socket_handle)) {
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Convert address to binary representation
        struct sockaddr_in sockaddr {};
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

    auto ClientSocket::write(void* data, kstd::usize data_size) const noexcept -> kstd::Result<kstd::usize> {
        using namespace std::string_literals;
        if(data_size > std::numeric_limits<int>::max()) {
            data_size = std::numeric_limits<int>::max();
        }

        auto bytes_sent = ::send(_socket_handle, static_cast<const char*>(data), static_cast<int>(data_size), 0);
        if(bytes_sent <= 0) {
            return kstd::Error {fmt::format("Unable to write to socket => {}", get_last_error())};
        }
        return bytes_sent;
    }

    ClientSocket::ClientSocket(ClientSocket&& other) noexcept {
        _socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
    }

    auto ClientSocket::read(kstd::u8* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize> {
        using namespace std::string_literals;
        if (!handle_valid(_socket_handle)) {
            return kstd::Error {"Unable to read from socket => Socket handle is invalid!"s};
        }

        if(size > std::numeric_limits<int>::max()) {
            size = std::numeric_limits<int>::max();
        }

        auto bytes_read = ::recv(_socket_handle, reinterpret_cast<char*>(data), static_cast<int>(size), 0);// NOLINT
        if(bytes_read < 0) {
            return kstd::Error {fmt::format("Unable to read from socket => {}", get_last_error())};
        }
        return bytes_read;
    }

#ifdef KSTD_CPP_20
    auto ClientSocket::read(std::span<kstd::u8> data) const noexcept -> kstd::Result<kstd::usize> {
        return read(data.data(), data.size());
    }
#endif

    auto ClientSocket::operator=(ClientSocket&& other) noexcept -> ClientSocket& {
        _socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
        return *this;
    }

    AcceptedSocket::AcceptedSocket(sockslib::SocketHandle socket_handle) {
        _socket_handle = socket_handle;
    }

    AcceptedSocket::AcceptedSocket(AcceptedSocket&& other) noexcept {// NOLINT
        _socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
    }

    AcceptedSocket::~AcceptedSocket() noexcept {
        if(handle_valid(_socket_handle)) {
            shutdown(_socket_handle, SHUT_RDWR);
            close(_socket_handle);
        }
    }

    auto AcceptedSocket::write(void* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize> {
        using namespace std::string_literals;
        if(!handle_valid(_socket_handle)) {
            return kstd::Error {"Unable to write with socket => Socket handle is invalid!"s};
        }

        if(size > std::numeric_limits<int>::max()) {
            size = std::numeric_limits<int>::max();
        }

        auto bytes_sent = ::send(_socket_handle, static_cast<const char*>(data), static_cast<int>(size), 0);
        if(bytes_sent < 0) {
            return kstd::Error {fmt::format("Unable to write with socket => {}", get_last_error())};
        }
        return bytes_sent;
    }

    auto AcceptedSocket::read(kstd::u8* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize> {
        using namespace std::string_literals;
        if(!handle_valid(_socket_handle)) {
            return kstd::Error {"Unable to read from socket => Socket handle is invalid!"s};
        }

        if(size > std::numeric_limits<int>::max()) {
            size = std::numeric_limits<int>::max();
        }

        auto bytes_read = ::recv(_socket_handle, reinterpret_cast<char*>(data), static_cast<int>(size), 0);// NOLINT
        if(bytes_read < 0) {
            return kstd::Error {fmt::format("Unable to read from socket => {}", get_last_error())};
        }
        return bytes_read;
    }

#ifdef KSTD_CPP_20
    auto AcceptedSocket::read(std::span<kstd::u8> data) const noexcept -> kstd::Result<kstd::usize> {
        return read(data.data(), data.size());
    }
#endif

    auto AcceptedSocket::operator=(AcceptedSocket&& other) noexcept -> AcceptedSocket& {
        _socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
        return *this;
    }
}// namespace sockslib
#endif