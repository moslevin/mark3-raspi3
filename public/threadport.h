/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 - 2018 m0slevin, all rights reserved.
See license.txt for more information
=========================================================================== */
/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 */
/**

    @file   threadport.h

    @brief  Cortex M4  Multithreading support.
 */
#pragma once

#include "kerneltypes.h"
#include "thread.h"

// clang-format off
//---------------------------------------------------------------------------
//! ASM Macro - simplify the use of ASM directive in C
#define ASM      asm volatile

//---------------------------------------------------------------------------
//! Macro to find the top of a stack given its size and top address
#define TOP_OF_STACK(x, y)        (K_WORD*) ( ((uint64_t)x) + (y - sizeof(K_WORD)) )
//! Push a value y to the stack pointer x and decrement the stack pointer
#define PUSH_TO_STACK(x, y)        *x = y; x--;
#define STACK_GROWS_DOWN           (1)

extern volatile K_WORD g_uSWIQueued;
extern volatile K_WORD g_uCriticalCount;
extern volatile K_WORD g_uExNestingCount;
extern volatile K_WORD g_uThreadUsesFPU;

//------------------------------------------------------------------------
// Use hardware accelerated count-leading zero
#define HW_CLZ (0)
//extern unsigned char __clz(unsigned int x);
#define CLZ(x)      __builtin_clz((x))

#define ENABLE_IRQS()										        \
	asm volatile ( "MSR DAIFCLR, #2" ::: "memory" );				\
	asm volatile ( "DSB SY" );									\
	asm volatile ( "ISB SY" );

#define DISABLE_IRQS()									            \
	asm volatile ( "MSR DAIFSET, #2" ::: "memory" );				\
	asm volatile ( "DSB SY" );									\
	asm volatile ( "ISB SY" );

//------------------------------------------------------------------------
#define CS_ENTER()  \
    do { \
        DISABLE_IRQS(); \
        g_uCriticalCount++; \
    } while(0);

//------------------------------------------------------------------------
#define CS_EXIT()   \
    do { \
        g_uCriticalCount--;    \
        if (!g_uCriticalCount) { \
            if (g_uSWIQueued) { \
                g_uSWIQueued = 0; \
                KernelSWI::Trigger(); \
            } \
            if (!g_uExNestingCount) { \
                ENABLE_IRQS(); \
            } \
        } \
    } while(0);


namespace Mark3 {
//------------------------------------------------------------------------
class Thread;
/**
 *  Class defining the architecture specific functions required by the 
 *  kernel.  
 *  
 *  This is limited (at this point) to a function to start the scheduler,
 *  and a function to initialize the default stack-frame for a thread.
 */
class ThreadPort
{
public:
    /**
     * @brief Init
     *
     * Function to perform early init of the target environment prior to
     * using OS primatives.
     */
    static void Init() {}

    /**        
     *  @brief StartThreads
     *
     *  Function to start the scheduler, initial threads, etc.
     */
    static void StartThreads();
    friend class Thread;
private:

    /**
     *  @brief InitStack
     *
     *  Initialize the thread's stack.
     *  
     *  @param pstThread_ Pointer to the thread to initialize
     */
    static void InitStack(Thread *pstThread_);
};
} // namespace Mark3
