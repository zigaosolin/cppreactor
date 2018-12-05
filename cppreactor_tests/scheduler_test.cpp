#include "catch.hpp"

#include <iostream>
#include <reactor_coroutine.hpp>

using namespace reactor;

reactor_coroutine<float> coroutine1()
{
	co_yield 1.0f;
	co_yield 3.0f;
	co_yield 5.0f;
}

TEST_CASE("Coroutine is updated", "[reactor_coroutine]") {

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
