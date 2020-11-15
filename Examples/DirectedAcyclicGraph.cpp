// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Manager.h>
#include <Jobs/Counter.h>
#include <Jobs/Logging.h>
#include <Jobs/Profiling.h>

#include <atomic>
#include <memory>

using namespace Jobs;
using namespace std::literals::chrono_literals;

//////////////////////////////////////////////////////////////
///////////////////////// TASK GRAPH /////////////////////////
//////////////////////////////////////////////////////////////
//                       A         B     C
//                     /   \       |    /
//                    D     E      F   /
//                  / | \  / \     /  /
//                 G  H  I    \   /  /
//                  \ | /      \ /  /
//                    J         K  /
//                     \        \ /
//                      \        L
//                       \      /
//                        \    /
//                         \  /
//                           M
//////////////////////////////////////////////////////////////

// Simulates some amount of work. Each chunk of work is ~300ms.
void DoWork(int workAmount)
{
	for (int i = 0; i < workAmount; ++i)
		std::this_thread::sleep_for(300ms);
}

int main()
{
	// Task trackers.
	auto counterA = std::make_shared<Counter<>>();
	auto counterB = std::make_shared<Counter<>>();
	auto counterC = std::make_shared<Counter<>>();
	auto counterD = std::make_shared<Counter<>>();
	auto counterE = std::make_shared<Counter<>>();
	auto counterF = std::make_shared<Counter<>>();
	auto counterG = std::make_shared<Counter<>>();
	auto counterH = std::make_shared<Counter<>>();
	auto counterI = std::make_shared<Counter<>>();
	auto counterJ = std::make_shared<Counter<>>();
	auto counterK = std::make_shared<Counter<>>();
	auto counterL = std::make_shared<Counter<>>();
	auto counterM = std::make_shared<Counter<>>();

	Manager manager;  // Hosts the workers, fibers, and job queue.
	manager.Initialize();  // Default ctor creates a worker for every hardware thread.

	// Forwards optional internal logging information for debug builds. Also handles our own logging info.
	LogManager::Get().SetOutputDevice(std::cout);

	Job taskA{ [](auto) {
		JOBS_SCOPED_STAT("Task A");
		JOBS_LOG(LogLevel::Log, "A");
		DoWork(2);
	} };

	Job taskB{ [](auto) {
		JOBS_SCOPED_STAT("Task B");
		JOBS_LOG(LogLevel::Log, "B");
		DoWork(3);
	} };

	Job taskC{ [](auto) {
		JOBS_SCOPED_STAT("Task C");
		JOBS_LOG(LogLevel::Log, "C");
		DoWork(5);
	} };

	Job taskD{ [](auto) {
		JOBS_SCOPED_STAT("Task D");
		JOBS_LOG(LogLevel::Log, "D");
		DoWork(1);
	} };
	taskD.AddDependency(counterA);

	Job taskE{ [](auto) {
		JOBS_SCOPED_STAT("Task E");
		JOBS_LOG(LogLevel::Log, "E");
		DoWork(7);
	} };
	taskE.AddDependency(counterA);

	Job taskF{ [](auto) {
		JOBS_SCOPED_STAT("Task F");
		JOBS_LOG(LogLevel::Log, "F");
		DoWork(3);
	} };
	taskF.AddDependency(counterB);

	Job taskG{ [](auto) {
		JOBS_SCOPED_STAT("Task G");
		JOBS_LOG(LogLevel::Log, "G");
		DoWork(5);
	} };
	taskG.AddDependency(counterD);

	Job taskH{ [](auto) {
		JOBS_SCOPED_STAT("Task H");
		JOBS_LOG(LogLevel::Log, "H");
		DoWork(1);
	} };
	taskH.AddDependency(counterD);

	Job taskI{ [](auto) {
		JOBS_SCOPED_STAT("Task I");
		JOBS_LOG(LogLevel::Log, "I");
		DoWork(3);
	} };
	taskI.AddDependency(counterD);
	taskI.AddDependency(counterE);

	Job taskJ{ [](auto) {
		JOBS_SCOPED_STAT("Task J");
		JOBS_LOG(LogLevel::Log, "J");
		DoWork(2);
	} };
	taskJ.AddDependency(counterG);
	taskJ.AddDependency(counterH);
	taskJ.AddDependency(counterI);

	Job taskK{ [](auto) {
		JOBS_SCOPED_STAT("Task K");
		JOBS_LOG(LogLevel::Log, "K");
		DoWork(5);
	} };
	taskK.AddDependency(counterE);
	taskK.AddDependency(counterF);

	Job taskL{ [](auto) {
		JOBS_SCOPED_STAT("Task L");
		JOBS_LOG(LogLevel::Log, "L");
		DoWork(3);
	} };
	taskL.AddDependency(counterK);
	taskL.AddDependency(counterC);

	Job taskM{ [](auto) {
		JOBS_SCOPED_STAT("Task M");
		JOBS_LOG(LogLevel::Log, "M");
		DoWork(1);
	} };
	taskM.AddDependency(counterJ);
	taskM.AddDependency(counterL);

	// Full DAG is defined, time to enqueue.
	// Note that we do have to enqueue in the proper order, otherwise jobs
	// might test their dependencies and move on, before their dependent job
	// has even had a chance to be enqueued on the main thread.

	{
		JOBS_SCOPED_STAT("DAG Enqueue");

		manager.Enqueue(taskA, counterA);
		manager.Enqueue(taskB, counterB);
		manager.Enqueue(taskC, counterC);
		manager.Enqueue(taskD, counterD);
		manager.Enqueue(taskE, counterE);
		manager.Enqueue(taskF, counterF);
		manager.Enqueue(taskG, counterG);
		manager.Enqueue(taskH, counterH);
		manager.Enqueue(taskI, counterI);
		manager.Enqueue(taskJ, counterJ);
		manager.Enqueue(taskK, counterK);
		manager.Enqueue(taskL, counterL);
		manager.Enqueue(taskM, counterM);
	}

	// Wait for the last task, M, to finish.
	counterM->Wait(0);

	JOBS_LOG(LogLevel::Log, "Finished");
	system("pause");

	return 0;
}