#include "sockslib/socket.hpp"
#include <gtest/gtest.h>
#include <kstd/option.hpp>
#include <kstd/safe_alloc.hpp>

TEST(sockslib_ServerSocket, test_bind_tcp_socket) {
    using namespace sockslib;
    auto socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::TCP, 512);
    socket_result.throw_if_error();
}

TEST(sockslib_ServerSocket, test_bind_udp_socket) {
    using namespace sockslib;
    auto socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::UDP, 512);
    socket_result.throw_if_error();
}

TEST(sockslib_ClientSocket, test_connect_tcp_socket) {
    using namespace sockslib;
    auto server_socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::TCP, 512);
    server_socket_result.throw_if_error();

    auto socket_result = kstd::try_construct<ClientSocket>("127.0.0.1", 1337, ProtocolType::TCP, 512);
    socket_result.throw_if_error();
    auto& socket = *socket_result;

    int data = 1337;
    socket.write(&data, sizeof(data)).throw_if_error();
    // TODO: Receive and validate data
}

TEST(sockslib_ClientSocket, test_connect_udp_socket) {
    using namespace sockslib;
    auto server_socket_result = kstd::try_construct<ServerSocket>(1337, ProtocolType::UDP, 512);
    server_socket_result.throw_if_error();

    auto socket_result = kstd::try_construct<ClientSocket>("127.0.0.1", 1337, ProtocolType::UDP, 512);
    socket_result.throw_if_error();
    auto& socket = *socket_result;

    int data = 1337;
    socket.write(&data, sizeof(data)).throw_if_error();
    // TODO: Receive and validate data
}
