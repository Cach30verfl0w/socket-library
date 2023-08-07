#include "sockslib/socket.hpp"
#include <gtest/gtest.h>
#include <kstd/option.hpp>
#include <kstd/safe_alloc.hpp>

TEST(sockslib_Socket, test_bind_tcp_socket_init_without_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(kstd::Option<kstd::u16> {}, sockslib::SocketType::TCP, 512);
    ASSERT_TRUE(socket_result);

    auto bind_result = socket_result.get().bind();
    ASSERT_FALSE(bind_result);

    using namespace std::string_literals;
    ASSERT_EQ(bind_result.get_error(), "Unable to bind socket => The port is not specified!"s);
}

TEST(sockslib_Socket, test_bind_tcp_socket_init_with_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(1337, sockslib::SocketType::TCP, 512);
    ASSERT_TRUE(socket_result);

    auto bind_result = socket_result.get().bind();
    ASSERT_TRUE(bind_result);
}

TEST(sockslib_Socket, test_bind_udp_socket_init_without_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(kstd::Option<kstd::u16> {}, sockslib::SocketType::UDP, 512);
    ASSERT_TRUE(socket_result);

    auto bind_result = socket_result.get().bind();
    ASSERT_FALSE(bind_result);

    using namespace std::string_literals;
    ASSERT_EQ(bind_result.get_error(), "Unable to bind socket => The port is not specified!"s);
}

TEST(sockslib_Socket, test_bind_udp_socket_init_with_port) {
    auto socket_result = kstd::try_construct<sockslib::Socket>(1337, sockslib::SocketType::UDP, 512);
    ASSERT_TRUE(socket_result);

    auto bind_result = socket_result.get().bind();
    ASSERT_TRUE(bind_result);
}