#include "sockslib/resolve.hpp"

#include <gtest/gtest.h>

TEST(sockslib_Resolver, resolve_domain) {
    auto resolver = sockslib::resolve_address("localhost", 10000);
    if (!resolver) {
        resolver.throw_if_error();
    }
}