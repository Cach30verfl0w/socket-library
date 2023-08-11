// Copyright 2023 Karma Krafts & associates
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef PLATFORM_WINDOWS
#include "sockslib/socket.hpp"

#include "fmt/format.h"
#include <numeric>
#include <stdexcept>
#include <iostream>

#include <WS2tcpip.h>

namespace sockslib {
    Socket::Socket() :
            _socket_handle {invalid_socket_handle} {
        init_wsa().throw_if_error();
    }

    ServerSocket::ServerSocket(const kstd::u16 port, const ProtocolType protocol_type) :
            _protocol_type {protocol_type} {
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
            cleanup_wsa();
            throw std::runtime_error(
                    fmt::format("Unable to initialize server (Address info resolve failed) => ", get_last_error()));
        }

        // Create the socket and do validation check
        _socket_handle = socket(_addr_info->ai_family, _addr_info->ai_socktype, _addr_info->ai_protocol);
        if(!handle_valid(_socket_handle)) {
            cleanup_wsa();

            FreeAddrInfoW(_addr_info);
            _addr_info = nullptr;

            throw std::runtime_error(
                    fmt::format("Unable to initialize server (Socket creation failed) => {}", get_last_error()));
        }

        // Bind the socket
        if(FAILED(::bind(_socket_handle, _addr_info->ai_addr, static_cast<int>(_addr_info->ai_addrlen)))) {
            cleanup_wsa();

            FreeAddrInfoW(_addr_info);
            closesocket(_socket_handle);
            _addr_info = nullptr;

            throw std::runtime_error(
                    fmt::format("Unable to initialize server (Socket binding failed) => {}", get_last_error()));
        }

        // Call the listen function if the socket is using TCP
        if(protocol_type == ProtocolType::TCP) {
            if(FAILED(listen(_socket_handle, SOMAXCONN))) {
                cleanup_wsa();

                FreeAddrInfoW(_addr_info);
                closesocket(_socket_handle);
                _addr_info = nullptr;

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
            _addr_info {other._addr_info},
            _protocol_type {other._protocol_type} {
        _socket_handle = other._socket_handle;
        other._socket_handle = invalid_socket_handle;
        other._addr_info = nullptr;
        ++_wsa_user_count;
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

        // Cleanup WSA if socket count is 1 and decrement socket count
        cleanup_wsa();
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
        _addr_info = other._addr_info;

        other._socket_handle = invalid_socket_handle;
        other._addr_info = nullptr;
        ++_wsa_user_count;
        return *this;
    }

    ClientSocket::ClientSocket(std::string address, const kstd::u16 port, const ProtocolType protocol_type) :
            _protocol_type {protocol_type} {
        using namespace std::string_literals;

        // Specify protocol
        int protocol = 0;
        switch(protocol_type) {
            case ProtocolType::TCP: protocol = IPPROTO_TCP; break;
            case ProtocolType::UDP: protocol = IPPROTO_UDP; break;
        }

#ifndef SOCKSLIB_NO_DNS_RESOLVE
        // Resolve address if domain
        if(is_domain(address)) {
            address = resolve_address(address).get_or_throw();
        }
#endif
        const auto address_type_result = recognize_address_type(address);
        if(!address_type_result) {
            throw std::runtime_error {
                    fmt::format("Unable to initialize socket => Unable to recognize address protocol")};
        }
        const auto address_type = static_cast<int>(address_type_result.get());

        // Create socket handle and validate it
        _socket_handle = socket(address_type, static_cast<int>(protocol_type), protocol);
        if(!handle_valid(_socket_handle)) {
            cleanup_wsa();
            throw std::runtime_error {fmt::format("Unable to initialize socket => {}", get_last_error())};
        }

        // Generate SOCKADDR
        SOCKADDR_IN addr {};
        if(FAILED(InetPton(address_type, address.c_str(),// NOLINT
                           &addr.sin_addr.s_addr))) {
            throw std::runtime_error {
                    "Unable to connect with socket => Failed conversion of literal address to binary address!"s};
        }
        addr.sin_family = address_type;
        addr.sin_port = htons(port);

        // Establish connection

        if(FAILED(connect(_socket_handle, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)))) {// NOLINT
            // Free address info and close socket
            closesocket(_socket_handle);

            // Cleanup WSA and decrement socket count
            cleanup_wsa();

            auto last_error = get_last_error();
            if(last_error.empty()) {
                throw std::runtime_error {
                        "Unable to connect with socket => Establishment of connection with server failed! Maybe the server isn't reachable?"s};
            }
            throw std::runtime_error {fmt::format("Unable to connect with socket => {}", last_error)};
        }
    }

    ClientSocket::ClientSocket(sockslib::ClientSocket&& other) noexcept :// NOLINT
            _protocol_type {other._protocol_type} {
        _socket_handle = other._socket_handle;
        other.Socket::_socket_handle = invalid_socket_handle;
        ++_wsa_user_count;
    }

    ClientSocket::~ClientSocket() noexcept {
        // Close the socket if valid
        if(handle_valid(_socket_handle)) {
            shutdown(_socket_handle, SD_SEND);
            closesocket(_socket_handle);
        }

        // Cleanup WSA if socket count is 1 and decrement socket count
        cleanup_wsa();
    }

    auto ClientSocket::write(void* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize> {
        using namespace std::string_literals;
        if(!handle_valid(_socket_handle)) {
            return kstd::Error {"Unable to write with socket => Socket handle is invalid!"s};
        }

        using namespace std::string_literals;
        if(size > std::numeric_limits<int>::max()) {
            size = std::numeric_limits<int>::max();
        }

        auto bytes_sent = ::send(_socket_handle, static_cast<const char*>(data), static_cast<int>(size), 0);
        if(bytes_sent <= 0) {
            return kstd::Error {fmt::format("Unable to write to socket => {}", get_last_error())};
        }
        return bytes_sent;
    }

    auto ClientSocket::read(kstd::u8* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize> {
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
    auto ClientSocket::read(std::span<kstd::u8> data) const noexcept -> kstd::Result<kstd::usize> {
        return read(data.data(), data.size());
    }
#endif

    auto ClientSocket::operator=(ClientSocket&& other) noexcept -> ClientSocket& {
        _socket_handle = other._socket_handle;
        _protocol_type = other._protocol_type;
        other._socket_handle = invalid_socket_handle;
        ++_wsa_user_count;
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
            shutdown(_socket_handle, SD_SEND);
            closesocket(_socket_handle);
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