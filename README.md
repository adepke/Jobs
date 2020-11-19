# Jobs
### Robust Fiber-Driven Task-Based Parallelism Library

## Features
- Fast scalable performance
- Lightweight dependencies with minimal switching overhead
- Managed dependency memory
- Unlimited dependencies per job, allowing for complex graphs
- Fiber-aware mutexes that allow mid-execution interruption
- High level algorithms to abstract individual job creation and management

## Motivation
With the modern trend of increasing CPU cores, a highly scalable and low overhead automated work distribution system is a critical component of the backend for many real-time applications. While many of these already exist, few benefit from the power of user mode scheduling, which allows for minimal cost switching between jobs and full control over the task scheduling. This project was primarily created as a testbed to learn how to effectively operate with fibers to create a performance-focused scheduler.

## Performance
*Coming soon, [Tracy](https://github.com/wolfpld/tracy) profiler fiber support is a work-in-progress*

## Building
Jobs uses [premake](https://premake.github.io/) as a project file generator.
### Windows
To create your solution files, run the included `premake5.exe` executable with the target project type as the first commandline argument, eg. `vs2019` for Visual Studio 2019 project files. Open the generated solution/project files in the specified IDE and build the default project.
### Linux
To create the makefiles, run the included `premake5` executable with the target action being `gmake2`. Then, run `make` to compile the project.

### Options
When running the build script, the behavior can be modified via the options below.
> --logging

Used to retrieve internal logs for debugging purposes, this will have signficant performance impacts.
> --profiling

Creates additional projects for compiling Tracy and the included examples, enables the emission of Tracy zones for internal profiling. You must include a build of Tracy (including it's dependencies) in the project root directory, with it's premake build scripts.

## Samples
Full examples of using this library can be found in the `Examples/` directory.
