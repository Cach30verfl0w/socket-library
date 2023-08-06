#pragma once

#include <cstdint>

namespace example {
	struct MyObject final {
		uint32_t x;
		uint32_t y;

		[[nodiscard]] inline auto sum() const noexcept -> uint32_t {
			return x + y;
		}
 	};
}
