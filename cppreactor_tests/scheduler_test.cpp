#include "catch.hpp"
#include <iostream>
#include <chrono>
#include <reactor_coroutine.hpp>

using namespace cppcoro;

reactor_coroutine<> single_co_await(int& iteration)
{
	iteration = 0;
	auto frame_data = co_await next_frame{};
	iteration = 1;
	REQUIRE(frame_data.delta_time == 0.1f);
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
	s.update_next_frame(reactor_default_frame_data{ 0.05f });
	REQUIRE(iteration == 0);

	s.update_next_frame(reactor_default_frame_data{ 0.1f });
	REQUIRE(iteration == 1);
}

reactor_coroutine<float> single_co_await_float(int& iteration)
{
	iteration = 0;
	auto frame_data = co_await next_frame<float>{};
	iteration = 1;
	REQUIRE(frame_data == 0.1f);
}

TEST_CASE("Coroutine non-default data", "[reactor_coroutine]") {

	reactor_scheduler<float> s;
	int iteration = -1;

	// Call or push do not start it yet
	auto c = single_co_await_float(iteration);
	REQUIRE(iteration == -1);
	s.push(c);
	REQUIRE(iteration == -1);

	// First update starts it
	s.update_next_frame( 0.05f );
	REQUIRE(iteration == 0);

	s.update_next_frame( 0.1f );
	REQUIRE(iteration == 1);
}

struct big_frame_struct
{
	float data1;
	int data[100];
};

reactor_coroutine<big_frame_struct*> single_co_await_big(int& iteration)
{
	iteration = 0;
	auto frame_data = co_await next_frame<big_frame_struct*>{};
	iteration = 1;
	REQUIRE(frame_data->data1 == 0.1f);
}

TEST_CASE("Coroutine frame data by ref", "[reactor_coroutine]") {

	big_frame_struct frame_data{ 0.05f };

	reactor_scheduler<big_frame_struct*> s;
	int iteration = -1;

	// Call or push do not start it yet
	auto c = single_co_await_big(iteration);
	REQUIRE(iteration == -1);
	s.push(c);
	REQUIRE(iteration == -1);

	// First update starts it
	s.update_next_frame(&frame_data);
	REQUIRE(iteration == 0);

	frame_data.data1 = 0.1f;
	s.update_next_frame(&frame_data);
	REQUIRE(iteration == 1);
}

reactor_coroutine<float> infinite_frames()
{
	for (;;)
		co_await next_frame<float>{};
}

TEST_CASE("Coroutine speed", "[reactor_coroutine]") {

	reactor_scheduler<float> s;
	auto c = infinite_frames();
	s.push(c);

	auto start = std::chrono::high_resolution_clock::now();

	const int loops = 500000;
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

	auto updates_per_second = loops / duration.count();

	std::cout << "Coroutines updates " << updates_per_second / 1000000 << "M/s" << std::endl;
	REQUIRE(updates_per_second > expectedMinUpdates);
}
