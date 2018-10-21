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

    .extern g_pclCurrent
    .extern g_pclNext
    .extern g_uCriticalCount
    .extern g_uExNestingCount
    .extern g_uSWIQueued
    .extern g_uThreadUsesFPU

    .section ".text"

    .globl _ThreadPort_ContextSwitchASM
    .globl _IRQ_Handler

.macro _ThreadPort_SaveContext
    MSR 	SPSEL, #0

    /* Save the entire context. */
    STP 	X0, X1, [SP, #-0x10]!
    STP 	X2, X3, [SP, #-0x10]!
    STP 	X4, X5, [SP, #-0x10]!
    STP 	X6, X7, [SP, #-0x10]!
    STP 	X8, X9, [SP, #-0x10]!
    STP 	X10, X11, [SP, #-0x10]!
    STP 	X12, X13, [SP, #-0x10]!
    STP 	X14, X15, [SP, #-0x10]!
    STP 	X16, X17, [SP, #-0x10]!
    STP 	X18, X19, [SP, #-0x10]!
    STP 	X20, X21, [SP, #-0x10]!
    STP 	X22, X23, [SP, #-0x10]!
    STP 	X24, X25, [SP, #-0x10]!
    STP 	X26, X27, [SP, #-0x10]!
    STP 	X28, X29, [SP, #-0x10]!
    STP 	X30, XZR, [SP, #-0x10]!

    /* SPSR and ELR. */
    MRS		X3, SPSR_EL1
    MRS		X2, ELR_EL1

    /* Save them both. */
    STP 	X2, X3, [SP, #-0x10]!

    /* FPU indicator */
    LDR         X0, FPU_ENABLE_
    LDR         X2, [X0]

    /* Save the FPU context, if any (32 128-bit registers). */
    CMP		X2, #0
    B.EQ	1f
    STP		Q0, Q1, [SP,#-0x20]!
    STP		Q2, Q3, [SP,#-0x20]!
    STP		Q4, Q5, [SP,#-0x20]!
    STP		Q6, Q7, [SP,#-0x20]!
    STP		Q8, Q9, [SP,#-0x20]!
    STP		Q10, Q11, [SP,#-0x20]!
    STP		Q12, Q13, [SP,#-0x20]!
    STP		Q14, Q15, [SP,#-0x20]!
    STP		Q16, Q17, [SP,#-0x20]!
    STP		Q18, Q19, [SP,#-0x20]!
    STP		Q20, Q21, [SP,#-0x20]!
    STP		Q22, Q23, [SP,#-0x20]!
    STP		Q24, Q25, [SP,#-0x20]!
    STP		Q26, Q27, [SP,#-0x20]!
    STP		Q28, Q29, [SP,#-0x20]!
    STP		Q30, Q31, [SP,#-0x20]!

1:
    /* Reserved word -- unused */
    MOV         X3, #0
    /* Store FPU indicator and padding */
    STP         X2, X3, [SP, #-0x10]!

    /* Store stack pointer */
    LDR 	X0, CURR_
    LDR 	X1, [X0]
    ADD         X1, X1, #0x10
    MOV 	X0, SP
    STR 	X0, [X1]

    MSR 	SPSEL, #1
    .endm

.macro _ThreadPort_RestoreContext
    /* Restore context */
    /* Switch to use the EL0 stack pointer. */
    MSR 	SPSEL, #0

    /* Load Stack pointer  */
    LDR 	X0, CURR_
    LDR 	X1, [X0]
    ADD         X1, X1, #0x10
    LDR         X0, [X1]
    MOV		SP, X0

    LDP 	X2, X3, [SP], #0x10  /* padding and FPU context. */

    /* Ignore X3 - it's only used for alignment */

    /* Restore the FPU context indicator based on value in thread's context. */
    LDR		X0, FPU_ENABLE_
    STR		X2, [X0]

    /* Restore the FPU context, if any. */
    CMP		X2, #0
    B.EQ	1f
    LDP		Q30, Q31, [SP], #0x20
    LDP		Q28, Q29, [SP], #0x20
    LDP		Q26, Q27, [SP], #0x20
    LDP		Q24, Q25, [SP], #0x20
    LDP		Q22, Q23, [SP], #0x20
    LDP		Q20, Q21, [SP], #0x20
    LDP		Q18, Q19, [SP], #0x20
    LDP		Q16, Q17, [SP], #0x20
    LDP		Q14, Q15, [SP], #0x20
    LDP		Q12, Q13, [SP], #0x20
    LDP		Q10, Q11, [SP], #0x20
    LDP		Q8, Q9, [SP], #0x20
    LDP		Q6, Q7, [SP], #0x20
    LDP		Q4, Q5, [SP], #0x20
    LDP		Q2, Q3, [SP], #0x20
    LDP         Q0, Q1, [SP], #0x20

1:
    /* SPSR and ELR. */
    LDP 	X2, X3, [SP], #0x10

    /* Restore the SPSR. */
    MSR		SPSR_EL1, X3
    MSR		ELR_EL1, X2

    /* Restore the remaining registers */
    LDP 	X30, XZR, [SP], #0x10
    LDP 	X28, X29, [SP], #0x10
    LDP 	X26, X27, [SP], #0x10
    LDP 	X24, X25, [SP], #0x10
    LDP 	X22, X23, [SP], #0x10
    LDP 	X20, X21, [SP], #0x10
    LDP 	X18, X19, [SP], #0x10
    LDP 	X16, X17, [SP], #0x10
    LDP 	X14, X15, [SP], #0x10
    LDP 	X12, X13, [SP], #0x10
    LDP 	X10, X11, [SP], #0x10
    LDP 	X8, X9, [SP], #0x10
    LDP 	X6, X7, [SP], #0x10
    LDP 	X4, X5, [SP], #0x10
    LDP 	X2, X3, [SP], #0x10
    LDP 	X0, X1, [SP], #0x10

    MSR 	SPSEL, #1
    .endm

.align 8
.type _ThreadPort_ContextSwitchASM %function
_ThreadPort_ContextSwitchASM:

    _ThreadPort_SaveContext

    /* Current = Next thread */
    LDR         X1, CURR_
    LDR         X0, NEXT_
    LDR         X0, [X0]
    STR         X0, [X1]

    _ThreadPort_RestoreContext
    ERET

.align 8
.type _IRQ_Handler, %function
_IRQ_Handler:
    /* Save volatile registers. */
    STP		X0, X1, [SP, #-0x10]!
    STP		X2, X3, [SP, #-0x10]!
    STP		X4, X5, [SP, #-0x10]!
    STP		X6, X7, [SP, #-0x10]!
    STP		X8, X9, [SP, #-0x10]!
    STP		X10, X11, [SP, #-0x10]!
    STP		X12, X13, [SP, #-0x10]!
    STP		X14, X15, [SP, #-0x10]!
    STP		X16, X17, [SP, #-0x10]!
    STP		X18, X19, [SP, #-0x10]!
    STP		X29, X30, [SP, #-0x10]!

    /* Save the SPSR and ELR. */
    MRS		X3, SPSR_EL1
    MRS		X2, ELR_EL1
    STP 	X2, X3, [SP, #-0x10]!

    /* Disable interrupts. */
    MSR 	DAIFSET, #2
    DSB		SY
    ISB		SY

    /* Set CS count to prevent context switches while in the interrupt context */
    LDR         X0, CS_COUNT_
    LDR         X1, [X0]
    ADD         X1, X1, #1
    STR         X1, [X0]

    /* Increment the exception nesting count -- Although with ^^^, this should never be > 1 */
    LDR         X0, EX_COUNT_
    LDR         X1, [X0]
    ADD         X1, X1, #1
    STR         X1, [X0]

    /* Call the C/C++ exception handler entrypoint */
    MOV         X0, #1
    MRS         X1, esr_el1
    MRS         X2, elr_el1
    MRS         X3, spsr_el1
    MRS         X4, far_el1

    /** Note -- this would be replaced with a non-trivial example in real-world use */
    BL          TimerTick_Handler

    /* Decrement the exception nesting count */
    LDR         X0, EX_COUNT_
    LDR         X1, [X0]
    SUB         X1, X1, #1
    STR         X1, [X0]

    /* Exit CS */
    LDR         X0, CS_COUNT_
    LDR         X1, [X0]
    SUB         X1, X1, #1
    STR         X1, [X0]

    /* Check if a SWI was queued */
    LDR         X0, SWI_QUEUED_
    LDR         X1, [X0]
    CMP         X1, #0
    B.EQ        Exit_NoCS

    /* Clear the SWI_QUEUED_ flag */
    MOV         X2, #0
    STR         X2, [X0]

    /* SWI was queued -- Perform a context switch on exit */

    /* Restore volatile registers. */
    LDP 	X4, X5, [SP], #0x10  /* SPSR and ELR. */
    MSR		SPSR_EL1, X5
    MSR		ELR_EL1, X4
    DSB		SY
    ISB		SY

    LDP		X29, X30, [SP], #0x10
    LDP		X18, X19, [SP], #0x10
    LDP		X16, X17, [SP], #0x10
    LDP		X14, X15, [SP], #0x10
    LDP		X12, X13, [SP], #0x10
    LDP		X10, X11, [SP], #0x10
    LDP		X8, X9, [SP], #0x10
    LDP		X6, X7, [SP], #0x10
    LDP		X4, X5, [SP], #0x10
    LDP		X2, X3, [SP], #0x10
    LDP		X0, X1, [SP], #0x10

    _ThreadPort_SaveContext

    /* Current = Next thread */
    LDR         X1, CURR_
    LDR         X0, NEXT_
    LDR         X0, [X0]
    STR         X0, [X1]

    _ThreadPort_RestoreContext
    ERET

Exit_NoCS:
    /* Restore volatile registers. */
    LDP 	X4, X5, [SP], #0x10  /* SPSR and ELR. */
    MSR		SPSR_EL1, X5
    MSR		ELR_EL1, X4
    DSB		SY
    ISB		SY

    LDP		X29, X30, [SP], #0x10
    LDP		X18, X19, [SP], #0x10
    LDP		X16, X17, [SP], #0x10
    LDP		X14, X15, [SP], #0x10
    LDP		X12, X13, [SP], #0x10
    LDP		X10, X11, [SP], #0x10
    LDP		X8, X9, [SP], #0x10
    LDP		X6, X7, [SP], #0x10
    LDP		X4, X5, [SP], #0x10
    LDP		X2, X3, [SP], #0x10
    LDP		X0, X1, [SP], #0x10

    ERET

NEXT_: .dword g_pclNext
CURR_: .dword g_pclCurrent
CS_COUNT_: .dword g_uCriticalCount
SWI_QUEUED_: .dword g_uSWIQueued
EX_COUNT_ : .dword g_uExNestingCount
FPU_ENABLE_ : .dword g_uThreadUsesFPU
