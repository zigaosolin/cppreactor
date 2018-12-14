#include "catch.hpp"
#include <iostream>
#include <chrono>
#include <reactor_coroutine.hpp>

using namespace reactor;


reactor_coroutine<> single_co_await(int& iteration)
{
	iteration = 0;
	float dt = co_await next_frame{};
	iteration = 1;
	REQUIRE(dt == 0.1f);

}


TEST_CASE("Coroutine is updated", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	int iteration = -1;

	// Call or push do not start it yet
	auto c = single_co_await(iteration);
	REQUIRE(iteration == -1);
	s.push(c);
	REQUIRE(iteration == -1);

	// First update starts it
	s.update_next_frame(0.05f);
	REQUIRE(iteration == 0);

	s.update_next_frame(0.1f);
	REQUIRE(iteration == 1);
}

reactor_coroutine<> infinite_frames()
{
	for (;;)
		co_await next_frame{};
}

TEST_CASE("Coroutine speed", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	auto c = infinite_frames();
	s.push(c);

	auto start = std::chrono::high_resolution_clock::now();

	const int loops = 100000;
	for (int i = 0; i < loops; i++)
	{
		s.update_next_frame(0.01f);
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> duration = end - start;

#ifdef _DEBUG
	const int expectedMinUpdates = 10000; // Very low requirements for debug
#else
	const int expectedMinUpdates = 1000000; // The number is actually 80M on my comp
#endif

	REQUIRE( (loops / duration.count()) > expectedMinUpdates);


}
