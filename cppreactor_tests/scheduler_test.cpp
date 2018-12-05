#include "catch.hpp"

#include <iostream>
#include <reactor_coroutine.hpp>
#include <reactor_scheduler.hpp>

using namespace reactor;


reactor_coroutine coroutine1(scheduler& s)
{
	co_await next_frame{s};
}

TEST_CASE("Coroutine is updated", "[reactor_coroutine]") {

	std::cout << "Starting" << std::endl;

	scheduler s;
	auto c = coroutine1(s);

	s.enqueue(c);

	s.update(0.1f);

}
