// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Manager.h>
#include <Jobs/Counter.h>
#include <Jobs/FiberMutex.h>
#include <Jobs/Logging.h>
#include <Jobs/Profiling.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace Jobs;
using namespace std::literals::chrono_literals;

int main()
{
	std::vector<int> items;

	auto counter = std::make_shared<Counter<>>();

	Manager manager;  // Hosts the workers, fibers, and job queue.
	manager.Initialize();  // Default ctor creates a worker for every hardware thread.

	// Forwards optional internal logging information for debug builds. Also handles our own logging info.
	LogManager::Get().SetOutputDevice(std::cout);

	FiberMutex mutex{ &manager };

	// Create our payload to pass to the jobs. The lifetime of this payload must exceed that of the jobs which use it.

	struct Payload
	{
		FiberMutex* mutex;
		std::vector<int>* destination;
	} jobPayload;

	// Configure the payload.

	jobPayload.mutex = &mutex;
	jobPayload.destination = &items;

	Job producerTask{ [](auto, void* payload) {

		JOBS_SCOPED_STAT("Producer Task");

		auto* typedPayload = reinterpret_cast<Payload*>(payload);

		std::lock_guard guard{ *typedPayload->mutex };  // Acquire the mutex.

		// We can either use a lock_guard, or manually lock and unlock the mutex.
		//typedPayload->mutex->lock();

		constexpr auto count = 10000;
		constexpr auto value = 5;

		std::this_thread::sleep_for(3s);  // Simulate some large chunk of work that takes ~3 seconds.

		std::fill_n(std::back_inserter(*typedPayload->destination), count, value);

		JOBS_LOG(LogLevel::Log, "Producer finished, generated %i values, expected sum: %i", count, count * value);

		//typedPayload->mutex->unlock();

	}, &jobPayload };

	Job consumerTask{ [](auto, void* payload) {

		JOBS_SCOPED_STAT("Consumer Task");

		auto* typedPayload = reinterpret_cast<Payload*>(payload);

		// Do some other tasks before needing the mutex. This is to prevent the consumer from acquiring the mutex before the producer does.
		std::this_thread::sleep_for(250ms);
		
		std::lock_guard guard{ *typedPayload->mutex };  // Acquire the mutex.

		// We can either use a lock_guard, or manually lock and unlock the mutex.
		//typedPayload->mutex->lock();

		const auto sum = std::accumulate(typedPayload->destination->begin(), typedPayload->destination->end(), 0);

		JOBS_LOG(LogLevel::Log, "Consumer finished, found %i values, sum: %i", typedPayload->destination->size(), sum);

		//typedPayload->mutex->unlock();

	}, &jobPayload };

	manager.Enqueue(producerTask, counter);
	manager.Enqueue(consumerTask, counter);

	counter->Wait(0);

	JOBS_LOG(LogLevel::Log, "Finished");
	system("pause");

	return 0;
}