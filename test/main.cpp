#include <gtest/gtest.h>

auto main(int num_args, char** args) -> int {
	testing::InitGoogleTest(&num_args, args);
	return RUN_ALL_TESTS();
}
