// Copyright (c) 2019-2020 Andrew Depke

#pragma once

#define JOBS_PLATFORM_WINDOWS 0
#define JOBS_PLATFORM_POSIX 0

#ifdef _WIN32
  #define JOBS_PLATFORM_WINDOWS 1
#else
  #define JOBS_PLATFORM_POSIX 1
#endif