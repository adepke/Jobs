// Copyright (c) 2019-2020 Andrew Depke

#pragma once

namespace Jobs
{
    struct FiberTransfer
    {
        void* context;
        void* data;
    };

    // Routines implemented in assembly.
    extern "C" void* make_fcontext(void* stack, size_t stackSize, void (*entry)(FiberTransfer transfer));
    extern "C" FiberTransfer jump_fcontext(void* to, void* data);
}