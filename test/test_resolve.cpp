#include "sockslib/resolve.hpp"

#include <gtest/gtest.h>

TEST(sockslib_Resolver, test_resolve_domain) {
    auto resolver = sockslib::resolve_address("localhost");
    if (!resolver) {
        resolver.throw_if_error();
    }

    if (resolver.get() == "127.0.0.1") {
        SUCCEED();
        return;
    }

    ASSERT_EQ(resolver.get(), "::1");
}

TEST(sockslib_Resolver, test_recognize_address_type_ipv4) {
    std::string address = "127.0.0.1";
    auto type = sockslib::recognize_address_type(address);
    ASSERT_FALSE(type.is_empty());
    ASSERT_EQ(type.get(), sockslib::AddressType::IPV4);
}

TEST(sockslib_Resolver, test_recognize_address_type_ipv6) {
    std::string address = "::1";
    auto type = sockslib::recognize_address_type(address);
    ASSERT_FALSE(type.is_empty());
    ASSERT_EQ(type.get(), sockslib::AddressType::IPV6);
}

TEST(sockslib_Resolver, test_recognize_address_type_invalid) {
    std::string address = "example.com";
    auto type = sockslib::recognize_address_type(address);
    ASSERT_TRUE(type.is_empty());
}

TEST(sockslib_Resolver, test_recognize_address_type_domain) {
    auto address = sockslib::resolve_address("example.com").get_or_throw();
    auto type = sockslib::recognize_address_type(address);
    ASSERT_FALSE(type.is_empty());
}