#include <gtest/gtest.h>
#include "sockslib/utils.hpp"

TEST(sockslib_Utils, test_domains) {
    ASSERT_TRUE(sockslib::is_domain("cach30verfl0w.de"));
    ASSERT_TRUE(sockslib::is_domain("subdomain.cach30verfl0w.de"));
    ASSERT_TRUE(sockslib::is_domain("subdomain.subdomain.cach30verfl0w.de"));
}

TEST(sockslib_Utils, test_ipv4_addresses) {
    ASSERT_TRUE(sockslib::is_ipv4_address("137.211.231.252"));
    ASSERT_TRUE(sockslib::is_ipv4_address("233.234.201.205"));
    ASSERT_TRUE(sockslib::is_ipv4_address("106.202.7.239"));
    ASSERT_TRUE(sockslib::is_ipv4_address("253.27.111.16"));
    ASSERT_TRUE(sockslib::is_ipv4_address("73.174.233.160"));
    ASSERT_TRUE(sockslib::is_ipv4_address("215.120.157.87"));
    ASSERT_TRUE(sockslib::is_ipv4_address("202.211.169.51"));
    ASSERT_TRUE(sockslib::is_ipv4_address("145.117.212.254"));
    ASSERT_TRUE(sockslib::is_ipv4_address("177.1.100.5"));
    ASSERT_TRUE(sockslib::is_ipv4_address("5.100.94.131"));
}

TEST(sockslib_Utils, test_ipv6_addresses) {
    ASSERT_TRUE(sockslib::is_ipv6_address("1950:98bb:33fe:cc7a:e605:304c:070a:28c2"));
    ASSERT_TRUE(sockslib::is_ipv6_address("dab9:74c5:d2f8:d8dd:62ec:adea:eb1e:7cc9"));
    ASSERT_TRUE(sockslib::is_ipv6_address("2458:fa28:bcb2:114b:46ff:000f:a4b5:2b96"));
    ASSERT_TRUE(sockslib::is_ipv6_address("0f73:7ff3:c32a:c1e7:70d4:2018:f7c8:55f1"));
    ASSERT_TRUE(sockslib::is_ipv6_address("8787:4aa1:338c:a58e:17b9:9954:e814:e439"));
    ASSERT_TRUE(sockslib::is_ipv6_address("8af2:9b43:80d2:8023:f9b3:84f4:e472:5f89"));
    ASSERT_TRUE(sockslib::is_ipv6_address("6777:2d94:f2d6:83b9:271a:e955:451f:227a"));
    ASSERT_TRUE(sockslib::is_ipv6_address("fe80::5b3f:c05e:de1a:9df7%17"));
    ASSERT_TRUE(sockslib::is_ipv6_address("fe80::1%17"));
    ASSERT_TRUE(sockslib::is_ipv6_address("::1"));
}
