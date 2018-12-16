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

reactor_coroutine<> nested_coroutine(int& data)
{
	co_await next_frame{};

	if (data == 3)
		co_return;
	data++;
	co_await nested_coroutine(data);
}

TEST_CASE("Coroutine nesting", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	int data = 0;
	auto c = nested_coroutine(data);
	s.push(c);

	REQUIRE(data == 0);
	s.update_next_frame(reactor_default_frame_data{ 0.01f });
	REQUIRE(data == 0);
	s.update_next_frame(reactor_default_frame_data{ 0.01f });
	REQUIRE(data == 1);
	s.update_next_frame(reactor_default_frame_data{ 0.01f });
	REQUIRE(data == 2);
	s.update_next_frame(reactor_default_frame_data{ 0.01f });
	REQUIRE(data == 3);
}

reactor_coroutine<> coroutine_inner(int id, int id2)
{
	co_await next_frame{};
}

reactor_coroutine<> coroutine_outer(int id1)
{
	for (int i = 0; i < 3; i++)
	{
		co_await coroutine_inner(id1, i);
	}
}

reactor_coroutine<> coroutine_control(bool& finished)
{
	for (int i = 0; i < 3; i++)
	{
		co_await coroutine_outer(i);
	}
	finished = true;
}


TEST_CASE("Coroutine multiple levels", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	double data = 0;

	bool finished = false;
	auto c = coroutine_control(finished);
	s.push(c);

	for (int i = 0; i < 10; i++)
	{
		s.update_next_frame(reactor_default_frame_data{ 0.01f });
	}
	REQUIRE(finished == true);
}


reactor_coroutine_return<double> increment_value_immediate(double value, double max)
{
	value += 1.0;
	if (value >= max)
	{
		co_return value;
	}

	auto new_value = co_await increment_value_immediate(value, max);

	co_return new_value;
}

reactor_coroutine<> increment_until(double& value, double max)
{
	value = co_await increment_value_immediate(value, max);
}

TEST_CASE("Coroutine return value", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	double data = 0;

	auto c = increment_until(data, 100);
	s.push(c);

	// Whole updat ein one frame since no frame suspensions
	s.update_next_frame(reactor_default_frame_data{ 0.01f });
	REQUIRE(data >= 100.0);
}

reactor_coroutine_return<double> increment_value_suspend(double value, double max)
{
	value += 1.0;
	if (value >= max)
	{
		co_return value;
	}

	co_await next_frame{};

	auto new_value = co_await increment_value_suspend(value, max);

	co_return new_value;
}

reactor_coroutine<> increment_until_suspend(double& value, double max)
{
	value = co_await increment_value_suspend(value, max);
}

TEST_CASE("Coroutine return value suspended return", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	double data = 0;

	auto c = increment_until_suspend(data, 100);
	s.push(c);

	// Whole updat ein one frame since no frame suspensions
	for (int i = 0; i < 101; i++)
	{
		s.update_next_frame(reactor_default_frame_data{ 0.01f });
	}
	REQUIRE(data >= 100.0);
}


reactor_coroutine<> exception_coroutine()
{
	throw std::exception();

	co_await next_frame{};
}

TEST_CASE("Coroutine throws exception", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	double data = 0;

	auto c = exception_coroutine();
	s.push(c);

	REQUIRE_THROWS(s.update_next_frame(reactor_default_frame_data{ 0.01f }), "exception");
}

reactor_coroutine<> exception_coroutine1()
{
	throw std::exception();

	co_await next_frame{};
}

reactor_coroutine<> exception_coroutine_outer(bool& caught)
{
	try {
		co_await exception_coroutine1();
	}
	catch (std::exception ex)
	{
		caught = true;
	}
}

TEST_CASE("Coroutine throws exception inside", "[reactor_coroutine]") {

	reactor_scheduler<> s;
	double data = 0;

	bool caught = false;
	auto c = exception_coroutine_outer(caught);
	s.push(c);

	s.update_next_frame(reactor_default_frame_data{ 0.01f });

	REQUIRE(caught == true);
}

// TODO: exceptions