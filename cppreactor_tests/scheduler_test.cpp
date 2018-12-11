#include "catch.hpp"

#include <iostream>
#include <reactor_coroutine.hpp>
#include <reactor_scheduler.hpp>

using namespace reactor;


reactor_coroutine coroutine1()
{
	std::cout << "1" << std::endl;
	float dt = co_await next_frame{};
	std::cout << "2" << std::endl;
	dt = co_await next_frame{};
	std::cout << "3" << std::endl;
}

TEST_CASE("Coroutine is updated", "[reactor_coroutine]") {

	std::cout << "Starting" << std::endl;

	reactor_scheduler s;
	auto c = coroutine1();

	std::cout << "a" << std::endl;
	s.enqueue(c);
	std::cout << "b" << std::endl;
	s.update(0.1f);
	std::cout << "c" << std::endl;
	s.update(0.1f);
	std::cout << "d" << std::endl;
	s.update(0.1f);
	std::cout << "e" << std::endl;
	s.update(0.1f);

}
