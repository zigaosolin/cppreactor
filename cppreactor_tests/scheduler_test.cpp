#include "catch.hpp"

#include <iostream>
#include <reactor_coroutine.hpp>
#include <reactor_scheduler.hpp>

using namespace reactor;

reactor_coroutine<float> coroutine2()
{
	co_yield 2.0f;
}

reactor_coroutine<float> coroutine1()
{
	co_await next_frame{};
	co_yield 3.0f;
	//co_yield 5.0f;
}

TEST_CASE("Coroutine is updated", "[reactor_coroutine]") {

	std::cout << "Starting" << std::endl;
	auto coroutine = coroutine1();

	bool notDone = coroutine.update(0.1f);
	REQUIRE(notDone == true);
	notDone = coroutine.update(0.1f);
	REQUIRE(notDone == true);
	notDone = coroutine.update(0.1f);
	REQUIRE(notDone == true);
	notDone = coroutine.update(0.1f);
	REQUIRE(notDone == false);
}
