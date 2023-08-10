#include "sockslib/resolve.hpp"

#include <gtest/gtest.h>

TEST(sockslib_Resolver, resolve_domain) {
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