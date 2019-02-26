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
/**

    @file   kernelswi.cpp

    @brief  Kernel Software interrupt implementation for ARM Cortex-M4

*/

#include "kerneltypes.h"
#include "kernelswi.h"

#include "mark3.h"

volatile K_WORD g_uSWIQueued = 0;

namespace Mark3
{
//---------------------------------------------------------------------------
void KernelSWI::Config(void)
{
    // Nothing to do...
}

//---------------------------------------------------------------------------
void KernelSWI::Start(void)
{
    // Nothing to do...
}

//---------------------------------------------------------------------------
void KernelSWI::Trigger(void)
{
    if (!Kernel::IsStarted()) {
        return;
    }

    // Use SVC handler to trigger context switches -- But don't trigger
    // context switches while in a critical section...
    if (!g_uCriticalCount) {
        asm volatile(" svc 0 \n ");
    } else {
        g_uSWIQueued = 1;
    }
}

} // namespace Mark3
