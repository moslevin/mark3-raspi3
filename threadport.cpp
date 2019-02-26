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
===========================================================================*/
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
 */
/**

    @file   threadport.cpp

    @brief  ARM Cortex-A53 Multithreading

*/

#include "kerneltypes.h"
#include "mark3cfg.h"
#include "thread.h"
#include "threadport.h"
#include "kernelswi.h"
#include "kerneltimer.h"
#include "timerlist.h"
#include "quantum.h"
#include "kernel.h"

using namespace Mark3;

volatile K_WORD g_uCriticalCount;  // Critical section nesting counter
volatile K_WORD g_uExNestingCount; // Exception nesting counter
volatile K_WORD g_uThreadUsesFPU;  // Current thread uses FPU - set to 1 to enable lazy stacking in current thread

namespace Mark3
{
//---------------------------------------------------------------------------
static void ThreadPort_StartFirstThread(void);

//---------------------------------------------------------------------------
void ThreadPort::InitStack(Thread* pclThread_)
{
    uint64_t* pu64Stack;
    uint16_t  i;

    // Get the top-of-stack pointer for the thread
    pu64Stack = (uint64_t*)pclThread_->m_pwStackTop;

#if KERNEL_STACK_CHECK
    // Initialize the stack to all FF's to aid in stack depth checking
    uint64_t* pu64Temp = (uint64_t*)pclThread_->m_pwStack;
    for (i = 0; i < pclThread_->m_u16StackSize / sizeof(uint64_t); i++) { pu64Temp[i] = 0xFFFFFFFFFFFFFFFF; }
#endif // #if KERNEL_STACK_CHECK

    // R0-R30 + XZR -- default stack context
    PUSH_TO_STACK(pu64Stack, 0x0101010101010101);
    PUSH_TO_STACK(pu64Stack, (uint64_t)pclThread_->m_pvArg);
    PUSH_TO_STACK(pu64Stack, 0x0303030303030303);
    PUSH_TO_STACK(pu64Stack, 0x0202020202020202);
    PUSH_TO_STACK(pu64Stack, 0x0505050505050505);
    PUSH_TO_STACK(pu64Stack, 0x0404040404040404);
    PUSH_TO_STACK(pu64Stack, 0x0707070707070707);
    PUSH_TO_STACK(pu64Stack, 0x0606060606060606);
    PUSH_TO_STACK(pu64Stack, 0x0909090909090909);
    PUSH_TO_STACK(pu64Stack, 0x0808080808080808);
    PUSH_TO_STACK(pu64Stack, 0x1111111111111111);
    PUSH_TO_STACK(pu64Stack, 0x1010101010101010);
    PUSH_TO_STACK(pu64Stack, 0x1313131313131313);
    PUSH_TO_STACK(pu64Stack, 0x1212121212121212);
    PUSH_TO_STACK(pu64Stack, 0x1515151515151515);
    PUSH_TO_STACK(pu64Stack, 0x1414141414141414);
    PUSH_TO_STACK(pu64Stack, 0x1717171717171717);
    PUSH_TO_STACK(pu64Stack, 0x1616161616161616);
    PUSH_TO_STACK(pu64Stack, 0x1919191919191919);
    PUSH_TO_STACK(pu64Stack, 0x1818181818181818);
    PUSH_TO_STACK(pu64Stack, 0x2121212121212121);
    PUSH_TO_STACK(pu64Stack, 0x2020202020202020);
    PUSH_TO_STACK(pu64Stack, 0x2323232323232323);
    PUSH_TO_STACK(pu64Stack, 0x2222222222222222);
    PUSH_TO_STACK(pu64Stack, 0x2525252525252525);
    PUSH_TO_STACK(pu64Stack, 0x2424242424242424);
    PUSH_TO_STACK(pu64Stack, 0x2727272727272727);
    PUSH_TO_STACK(pu64Stack, 0x2626262626262626);
    PUSH_TO_STACK(pu64Stack, 0x2929292929292929);
    PUSH_TO_STACK(pu64Stack, 0x2828282828282828);
    PUSH_TO_STACK(pu64Stack, 0); // XZR
    PUSH_TO_STACK(pu64Stack, 0); // R30

    PUSH_TO_STACK(pu64Stack, 0x4);                                  // Initial processor state (EL1)
    PUSH_TO_STACK(pu64Stack, (uint64_t)pclThread_->m_pfEntryPoint); // Exception return address

    PUSH_TO_STACK(pu64Stack, 0); // Thread uses floats - set to 0 by default to enable lazy stacking
    PUSH_TO_STACK(pu64Stack, 0); // Reserved -- for 16-byte alignment

    pu64Stack++;

    pclThread_->m_pwStackTop = pu64Stack;
}

//---------------------------------------------------------------------------
void Thread_Switch(void)
{
    g_pclCurrent = (Thread*)g_pclNext;
}

//---------------------------------------------------------------------------
void ThreadPort::StartThreads()
{
    DISABLE_IRQS();

    g_uCriticalCount = 1;

    KernelSWI::Config();   // configure the task switch SWI
    KernelTimer::Config(); // configure the kernel timer

    // Tell the kernel that we're ready to start scheduling threads
    // for the first time.
    Kernel::CompleteStart();

    Scheduler::SetScheduler(1); // enable the scheduler
    Scheduler::Schedule();      // run the scheduler - determine the first thread to run

    Thread_Switch(); // Set the next scheduled thread to the current thread

    //!! Note: Kernel timers are started from the timer thread in this port
    KernelSWI::Start(); // enable the task switch SWI

#if KERNEL_ROUND_ROBIN
    Quantum::Update(g_pclCurrent);
#endif // #if KERNEL_ROUND_ROBIN

    g_uCriticalCount = 0;

    ThreadPort_StartFirstThread(); // Jump to the first thread (does not return)
}

//---------------------------------------------------------------------------
void ThreadPort_StartFirstThread(void)
{
    ASM(" MSR 	SPSEL, #0  \n"

        /* Swap in next thread */
        " MOV		X1, %[CURRENT_THREAD]\n "

        /* Load Stack pointer  */
        " ADD       X2, X1, #0x10 \n "
        " LDR       X0, [X2] \n"
        " MOV		SP, X0 \n"

        /* Load FPU Context word (ignore -- defaults to zero on start) */
        " LDP       X2, X3, [SP], #0x10 \n " /* padding and FPU context. */

        /* Restore context */
        /* SPSR and ELR. */
        " LDP       X2, X3, [SP], #0x10 \n "

        /* Restore the SPSR + ELR. */
        " MSR		SPSR_EL1, X3 \n "
        " MSR		ELR_EL1, X2 \n "

        /* Restore the remaining registers */
        " LDP 	X30, XZR, [SP], #0x10 \n"
        " LDP 	X28, X29, [SP], #0x10 \n"
        " LDP 	X26, X27, [SP], #0x10 \n"
        " LDP 	X24, X25, [SP], #0x10 \n"
        " LDP 	X22, X23, [SP], #0x10 \n"
        " LDP 	X20, X21, [SP], #0x10 \n"
        " LDP 	X18, X19, [SP], #0x10 \n"
        " LDP 	X16, X17, [SP], #0x10 \n"
        " LDP 	X14, X15, [SP], #0x10 \n"
        " LDP 	X12, X13, [SP], #0x10 \n"
        " LDP 	X10, X11, [SP], #0x10 \n"
        " LDP 	X8, X9, [SP], #0x10 \n"
        " LDP 	X6, X7, [SP], #0x10 \n"
        " LDP 	X4, X5, [SP], #0x10 \n"
        " LDP 	X2, X3, [SP], #0x10 \n"
        " LDP 	X0, X1, [SP], #0x10 \n"

        " MSR 	SPSEL, #1 \n"
        " ERET \n"
        :
        : [CURRENT_THREAD] "r"(g_pclCurrent));
}

} // namespace Mark3
