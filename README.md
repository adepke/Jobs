# Jobs
### Robust Fiber-Driven Task-Based Parallelism Library

## Features
- Fast scalable performance
- Lightweight dependencies with minimal switching overhead
- Managed dependency memory
- Unlimited dependencies per job, allowing for complex graphs
- High level algorithms to abstract individual job creation and management

## Motivation
With the modern trend of increasing CPU cores, a highly scalable automated work distribution system is fundamental as a backend for many real-time applications. While many of these already exist, few benefit from the power of user mode scheduling, which allows for minimal cost switching between jobs. The only other fiber backed job library at the time of conception (that I was aware of) was Richie Sam's Fiber Tasking Lib (FTL), which sadly lacked many crucial core and quality of life features which this project was created to satisfy.

## Performance
*Coming soon, [Tracy](https://github.com/wolfpld/tracy) profiler fiber support is a work-in-progress*

## Building
Jobs uses [premake](https://premake.github.io/) as a project file generator.
To create your solution files, run the included `premake5.exe` executable with the target project type as the first commandline argument, eg. `vs2019` for Visual Studio 2019 project files.

### Options
When running the build script, the behavior can be modified via the options below. The syntax of using an option is as following: `./premake5.exe vs2019 --logging`
> logging

Used to retrieve internal logs for debugging purposes, this will have signficant performance impacts.
> profiling

Creates additional projects for compiling Tracy and the included examples, enables the emission of Tracy zones for internal profiling. You must include a build of Tracy (including it's dependencies) in the project root directory, with it's premake build scripts.

## Samples
Full examples of using this library can be found in the `Examples/` directory.