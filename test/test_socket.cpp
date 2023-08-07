#include "sockslib/socket.hpp"
#include <gtest/gtest.h>
#include <kstd/option.hpp>
#include <kstd/safe_alloc.hpp>

TEST(sockslib_Socket, test_bind_tcp_socket_init_with_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(1337, sockslib::SocketType::TCP, 512);
    socket_result.throw_if_error();
}

TEST(sockslib_Socket, test_bind_udp_socket_init_with_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(1337, sockslib::SocketType::UDP, 512);
    socket_result.throw_if_error();
}