#include "sockslib/socket.hpp"

#include <gtest/gtest.h>
#include <kstd/safe_alloc.hpp>
#include <thread>
#include <iostream>
#include <chrono>

TEST(sockslib_ServerSocket, test_bind_tcp_socket) {
    using namespace sockslib;
    auto socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::TCP);
    socket_result.throw_if_error();
}

TEST(sockslib_ServerSocket, test_bind_udp_socket) {
    using namespace sockslib;
    auto socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::UDP);
    socket_result.throw_if_error();
}

TEST(sockslib_ClientSocket, test_connect_tcp_socket) {
    using namespace sockslib;
    auto server_socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::TCP);
    server_socket_result.throw_if_error();

    auto socket_result = kstd::try_construct<ClientSocket>("127.0.0.1", 1337, ProtocolType::TCP);
    socket_result.throw_if_error();
}

TEST(sockslib_ClientSocket, test_connect_udp_socket) {
    using namespace sockslib;
    auto server_socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::UDP);
    server_socket_result.throw_if_error();

    auto socket_result = kstd::try_construct<ClientSocket>("127.0.0.1", 1337, ProtocolType::UDP);
    socket_result.throw_if_error();
}

TEST(sockslib_ClientSocket, test_tcp_socket_write_read) {
    using namespace sockslib;
    kstd::atomic_bool server = false;

    // Initialize server socket side
    auto thread = std::thread { [&server] {// NOLINT
        // Initialize server socket
        auto server_socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::TCP);
        auto& server_socket = server_socket_result.get_or_throw();
        server = true;

        // Await socket and write 1
        auto socket = std::move(server_socket.accept().get_or_throw());
        kstd::u8 data = 1;
        socket.write(&data, sizeof(data)).throw_if_error();
    }};

    while (!server) {
        std::this_thread::yield();
    }

    // Establish connection
    auto socket_result = kstd::try_construct<ClientSocket>("127.0.0.1", 1337, ProtocolType::TCP);
    if (!socket_result) {
        if (thread.joinable()) {
            thread.join();
        }
        socket_result.throw_if_error();
    }
    auto& socket = socket_result.get();

    kstd::u8 data = 0;
    auto read_result = socket.read(&data, sizeof(data));
    if (!read_result) {
        if (thread.joinable()) {
            thread.join();
        }
        read_result.throw_if_error();
    }

    if (read_result.get() == 0) {
        if (thread.joinable()) {
            thread.join();
        }
        throw std::runtime_error("No bytes are readable!");
    }

    if (thread.joinable()) {
        thread.join();
    }

    ASSERT_EQ(data, 1);
}
