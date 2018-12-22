//      TITLE("Interval and Profile Clock Interrupts")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    clock.s
//
// Abstract:
//    This module implements the code necessary to field and process the interval and profile clock interrupts.
//
// Author:
//    David N. Cutler (davec) 27-Mar-1990
//    Joe Notarangelo 06-Apr-1992
//
// Environment:
//    Kernel mode only.
//
// Revision History:


#include "ksalpha.h"
#if DBG
// KiDpcTimeout - This is the number of clock ticks that a single DPC can
//      consume. When a DPC crosses this threshold, a BreakPoint is issued

        .globl  KiDpcTimeout
KiDpcTimeout:
        .long   110

//
// KiDpcTimeoutMsg - This is the message that gets displayed if the DPC
//      has exceeded KiDpcTimeout
//
        .globl KiDpcTimeoutMsg
KiDpcTimeoutMsg:
        .ascii "\n*** DPC routine > 1 sec --- This is not a break in KeUpdateRunTime\n"

//
// KiDpcTimeoutMsgLength - This is the length of the timeout message,
//      including the trailing NULL
//
        .globl KiDpcTimeoutMsgLength
KiDpcTimeoutMsgLength:
        .long   69

#endif


// VOID KeUpdateSystemTime (IN ULONG TimeIncrement)
//
// Routine Description:
//    This routine is entered as the result of an interrupt generated by the interval timer.
//    Its function is to update the system time and check to determine if a timer has expired.
//
//    N.B. This routine is executed on a single processor in a multiprocess system.
//       The remainder of the processors only execute the quantum end and runtime update code.
//
// Arguments:
//    Time Increment (a0) - Supplies the time increment in 100ns units.
//
// Return Value:
//    None.

        LEAF_ENTRY(KeUpdateSystemTime)

//
// Update the interrupt time.
//

        zap     a0, 0xf0, a0            // zero extend time increment
        lda     a2, KiTickOffset        // get tick offset value
        ldl     a3, 0(a2)               //
        LDIP    t8, SharedUserData      // get shared user data address
        ldq     t9, UsInterruptTime(t8) //
        addq    a0, t9, t9              // add time increment value
        stq     t9, UsInterruptTime(t8) // store interrupt time value
#if _AXP64_
        sra     t9, 32, t4
        stl     t4, UsInterruptHigh2Time(t8)    // store the high dword again
#endif
        subq    a3, a0, a3              // subtract time increment
        lda     v0, KeTickCount         // get tick count value
        ldq     t6, 0(v0)               //
        lda     t0, KiTimerTableListHead // get base address of timer table
        stl     a3, 0(a2)               // store tick offset value
        bgt     a3, 10f                 // if gt, tick not completed
        ldl     a4, KeMaximumIncrement  // get maximum increment value

//
// Update system time.
//

        lda     t1, KeTimeAdjustment    // get time adjustment value
        ldl     t1, 0(t1)               //
#if _AXP64_
        ldl     t3, UsSystemHigh1Time(t8) // load high dword
        sll     t3, 32, t3
        ldl     t4, UsSystemLowTime(t8)  // load low dword
        extll   t4, zero, t4             // zero-extend low dword
        bis     t4, t3, t3               // get the system time value
        addq    t1, t3, t3               // add time increment value
        sra     t3, 32, t4              // get the high dword
        .set noreorder
        stl     t4, UsSystemHigh1Time(t8) // store the first high dword
        stl     t3, UsSystemLowTime(t8)  // store the low dword
        stl     t4, UsSystemHigh2Time(t8) // store the second high dowrd
        .set reorder
#else
        ldq     t3, UsSystemTime(t8)    // get system time value
        addq    t1, t3, t3              // add time increment value
        stq     t3, UsSystemTime(t8)    // store system time value
#endif

//
// Update the tick count.
//

        addq    t6, 1, t1               // increment tick count value
        stq     t1, 0(v0)               // store tick count value
        stl     t1, UsTickCountLow(t8)  // store low tick count value

//
// Compute next tick offset value.
//

        addq    a3, a4, a4              // add maximum increment to residue
        stl     a4, 0(a2)               // store tick offset value

//
// Check to determine if a timer has expired at the current hand value.
//

        and     t6, TIMER_TABLE_SIZE - 1, v0  // reduce to table table index

#if defined(_AXP64_)

        sll     v0, 4, t2               // compute timer table listhead address
        addq    t2, t0, t2              //

#else

        s8addl  v0, t0, t2              // compute timer table listhead address

#endif

        LDP     t3, LsFlink(t2)         // get address of first timer in list
        cmpeq   t2, t3, t4              // compare fist with listhead address
        bne     t4, 5f                  // if ne, no timer active

//
// Get the expiration time from the timer object.
//
// N.B. The offset to the timer list entry must be subtracted out of the
//      displacement calculation.
//

        ldq     t4,TiDueTime - TiTimerListEntry(t3) // get due time
        cmpule  t4, t9, t5              // is expiration time <= system time
        bne     t5, 20f                 // if ne, timer has expired

//
// Check to determine if a timer has expired at the next hand value.
//

5:      addq    t6, 1, t6               // advance hand value to next entry
10:     and     t6, TIMER_TABLE_SIZE - 1, v0 // reduce to table table index

#if defined(_AXP64_)

        sll     v0, 4, t2               // compute timer table listhead address
        addq    t2, t0, t2              //

#else

        s8addl  v0, t0, t2              // compute timer table listhead address

#endif

        LDP     t3, LsFlink(t2)         // get address of first timer in list
        cmpeq   t2, t3, t4              // compare fist with listhead address
        bne     t4, 40f                 // if ne, no timer active

//
// Get the expiration time from the timer object.
//

        ldq     t4, TiDueTime - TiTimerListEntry(t3) // get due time
        cmpule  t4, t9, t5              // is expiration time <= system time
        beq     t5, 40f                 // if eq, timer has not expired

//
// Put timer expiration DPC in the system DPC list and initiate a dispatch
// interrupt on the current processor.
//

20:     lda     t2, KiTimerExpireDpc    // get expiration DPC address

        DISABLE_INTERRUPTS              // disable interrupts

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        lda     t3, PbDpcListHead(v0)   // get DPC listhead address
        lda     t1, PbDpcLock(v0)       // get address of spin lock

#if !defined(NT_UP)

30:     LDP_L   t4, 0(t1)               // get current lock value
        bis     t1, zero, t5            // set ownership value
        bne     t4, 50f                 // if ne, spin lock owned
        STP_C   t5, 0(t1)               // set spin lock owned
        beq     t5, 50f                 // if eq, store conditional failed
        mb                              // synchronize memory access

#endif

        LDP     t4, DpLock(t2)          // get DPC inserted state
        bne     t4, 35f                 // if ne, DPC entry already inserted
        LDP     t4, LsBlink(t3)         // get address of last entry in list
        STP     t1, DpLock(t2)          // set DPC inserted state
        STP     t6, DpSystemArgument1(t2) // set timer table hand value
        ADDP    t2, DpDpcListEntry, t2  // compute address of DPC list entry
        STP     t2, LsBlink(t3)         // set address of new last entry
        STP     t2, LsFlink(t4)         // set next link in old last entry
        STP     t3, LsFlink(t2)         // set address of next entry
        STP     t4, LsBlink(t2)         // set address of previous entry
        ldl     t5, PbDpcQueueDepth(v0) // get current DPC queue depth
        addl    t5, 1, t7               // increment DPC queue depth
        stl     t7, PbDpcQueueDepth(v0) // set updated DPC queue depth

//
// N.B. Since an interrupt must be active, simply set the software interrupt
//      request bit in the PRCB to request a dispatch interrupt directly from
//      the interrupt exception handler.
//

        ldil    t11, DISPATCH_INTERRUPT  // get interrupt request level
        stl     t11, PbSoftwareInterrupts(v0) // set interrupt request level
35:                                     //

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(t1)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // enable interrupts

//
// Check to determine is a full tick has expired.
//

40:     ble      a3, KeUpdateRunTime    // if le, full tick expiration
        ret     zero, (ra)              // return

//
// Attempt to acquire the dpc lock failed.
//

#if !defined(NT_UP)

50:     LDP     t4, 0(t1)               // get lock value
        beq     t4, 30b                 // if eq, lock available
        br      zero, 50b               // retry

#endif

        .end    KeUpdateSystemTime

//++
//
// VOID
// KeUpdateRunTime (
//    VOID
//    )
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt generated by the
//    interval timer. Its function is to update the runtime of the current
//    thread, update the runtime of the current thread's process, and decrement
//    the current thread's quantum.
//
//    N.B. This routine is executed on all processors in a multiprocess system.
//
// Arguments:
//
//    None
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeUpdateRunTime)

        GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, t0            // save current thread address

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        bis     v0, zero, t5            // save current prcb address
        LDP     a0, PbInterruptTrapFrame(v0) // get trap frame address

//
// Update the current DPC rate.
//
// A running average of the DPC rate is used. The number of DPCs requested
// in the previous tick is added to the current DPC rate and divided by two.
// This becomes the new DPC rate.
//

        ldl     t1, PbDpcCount(t5)      // get current DPC count
        ldl     t6, PbLastDpcCount(t5)  // get last DPC count
        subl    t1, t6, t7              // compute difference
        ldl     t2, PbDpcRequestRate(t5) // get old DPC request rate
        addl    t7, t2, t3              // compute average
        srl     t3, 1, t4               //
        stl     t4, PbDpcRequestRate(t5) // store new DPC request rate
        stl     t1, PbLastDpcCount(t5)  // update last DPC count
        LDP     t2, ThApcState + AsProcess(t0) // get current process address
        ldl     t3, TrPsr(a0)           // get saved processor status
        and     t3, PSR_MODE_MASK, t6   // isolate previous mode
        bne     t6, 30f                 // if ne, previous mode was user

//
// If a DPC is active, then increment the time spent executing DPC routines.
// Otherwise, if the old IRQL is greater than DPC level, then increment the
// time spent executing interrupt service routines.  Otherwise, increment
// the time spent in kernel mode for the current thread.
//

        srl     t3, PSR_IRQL, t6        // isolate previous IRQL
        ldl     v0, PbDpcRoutineActive(t5) // get DPC active flag
        subl    t6, DISPATCH_LEVEL, t6  // previous Irql - DPC level
        blt     t6, 20f                 // if lt, charge against thread
        lda     t8, PbInterruptTime(t5) // compute interrupt time address
        bgt     t6, 10f                 // if gt, increment interrupt time
        lda     t8, PbDpcTime(t5)       // compute DPC time address
        beq     v0, 20f                 // if eq, not executing DPC

#if DBG
//
// On a checked build, increment the DebugDpcTime count and see if this
// has exceeded the value of KiDpcTimeout. If it has, then we need to
// print a message and issue a breakpoint (if possible)
//
        ldl     t9, PbDebugDpcTime(t5)  // load current time in DPC
        addl    t9, 1, t9               // another tick occured
        ldl     t10, KiDpcTimeout       // What is the timeout value?
        cmpule  t9, t10, t11            // T11=1 if tick <= timeout
        bne     t11, 5f                 // if ne, then time is okay
        lda     a0, KiDpcTimeoutMsg     // load the timeout message address
        ldl     a1, KiDpcTimeoutMsgLength // load the timeout message length
        BREAK_DEBUG_PRINT               // Print the message
        BREAK_DEBUG_STOP                // Enter the debugger
        bis     zero, zero, t9          // Clear the time in DPC

5:      stl     t9, PbDebugDpcTime(t5)  // store current time in DPC
#endif

//
// Update the time spent executing DPC or interrupt level
//
// t8 = address of time to increment
//

10:     ldl     t11, 0(t8)              // get processor time
        addl    t11, 1, t11             // increment processor time
        stl     t11, 0(t8)              // update processor time
        lda     t6, PbKernelTime(t5)    // compute address of kernel time
        br      zero, 45f               // update kernel time

//
// Update the time spent in kernel mode for the current thread and the current
// thread's process.
//

20:     ldl     t11, ThKernelTime(t0)   // get kernel time
        addl    t11, 1, t11             // increment kernel time
        stl     t11, ThKernelTime(t0)   // store updated kernel time
        lda     t2, PrKernelTime(t2)    // compute process kernel time address
        lda     t6, PbKernelTime(t5)    // compute processor kernel time addr
        br      zero, 40f               // join comon code

//
// Update the time spend in user mode for the current thread and the current
// thread's process.
//

30:     ldl     t11, ThUserTime(t0)     // get user time
        addl    t11, 1, t11             // increment user time
        stl     t11, ThUserTime(t0)     // store updated user time
        lda     t2, PrUserTime(t2)      // compute process user time address
        lda     t6, PbUserTime(t5)      // compute processor user time address

//
// Update the time spent in kernel/user mode for the current thread's process.
//

40:                                     //

#if !defined(NT_UP)

        ldl_l   t11, 0(t2)              // get process time
        addl    t11, 1, t11             // increment process time
        stl_c   t11, 0(t2)              // store updated process time
        beq     t11, 41f                // if eq, store conditional failed
        mb                              // synchronize subsequent reads

#else

        ldl     t11,0(t2)               // get process time
        addl    t11, 1, t11             // increment process time
        stl     t11,0(t2)               // store updated process time

#endif

//
// A DPC is not active.  If there are DPCs in the DPC queue and a  DPC
// interrupt has not  been requested, request  a dispatch interrupt  in
// order to initiate the batch  processing of the  pending DPCs in  the
// DPC queue.
//
// N.B. Since an interrupt must be active, the software interrupt request
//      bit in the PRCB can be set to request a dispatch interrupt directly from
//      the interrupt exception handler.
//
// Pushing DPCs from the clock interrupt indicates that the current maximum
// DPC queue depth is too high. If the DPC rate does not exceed the ideal
// rate, decrement the maximum DPC queue depth and
// reset the threshold to its original value.
//

        ldl     t1, PbDpcQueueDepth(t5) // get current queue depth
        beq     t1, 45f                 // skip if queue is empty
        ldl     t2, PbDpcInterruptRequested(t5) // get dpc interrupt request flag
        bne     t2, 45f                 // skip if flag is set
        ldil    a0, DISPATCH_INTERRUPT  // set software interrupt request
        stl     a0, PbSoftwareInterrupts(t5) //
        ldl     t3, PbMaximumDpcQueueDepth(t5) // get current DPC queue depth
        subl    t3, 1, t4                // decrement
        ldl     t2, PbDpcRequestRate(t5) // get old DPC request rate
        ldl     t1, KiIdealDpcRate       // get ideal DPC rate
        cmpult  t2, t1, t2               // compare current with ideal
        ldl     t1, KiAdjustDpcThreshold // get system threshold default
        stl     t1, PbAdjustDpcThreshold(t5) // reset processor threshold default
        beq     t4, 50f                  // if queue depth==0, skip decrement
        beq     t2, 50f                  // if rate not lt ideal rate, skip decrement
        stl     t4, PbMaximumDpcQueueDepth(t5) // set current DPC queue depth
        br      zero, 50f                // rejoin common code

//
// There is no need to push a DPC from the clock interrupt. This indicates that
// the current maximum DPC queue depth may be too low. Decrement the threshold
// indicator, and if the new threshold is zero, and the current maximum queue
// depth is less than the maximum, increment the maximum DPC queue
// depth.
//

45:     ldl     t1, PbAdjustDpcThreshold(t5) // get current threshold
        subl    t1, 1, t2               // decrement threshold
        stl     t2, PbAdjustDpcThreshold(t5) // update current threshold
        bne     t2, 50f                 // if threshold nez, skip
        ldl     t1, KiAdjustDpcThreshold // get system threshold default
        stl     t1, PbAdjustDpcThreshold(t5) // reset processor threshold default
        ldl     t3, PbMaximumDpcQueueDepth(t5) // get current DPC queue depth
        ldl     t1, KiMaximumDpcQueueDepth // get maximum DPC queue depth
        cmpult  t3, t1, t2              // compare
        beq     t2, 50f                 // if current not lt maximum, skip
        addl    t3, 1, t4               // increment queue depth
        stl     t4, PbMaximumDpcQueueDepth(t5) // update current DPC queue depth

//
// Update the time spent in kernel/user mode for the current processor.
//
// t5 = pointer to processor time to increment
//

50:     ldl     t11, 0(t6)              // get processor time
        addl    t11, 1, t11             // increment processor time
        stl     t11, 0(t6)              // store updated processor time

//
// If the current thread is not the idle thread, decrement its
// quantum and check to determine if a quantum end has occurred.
//

        LDP     t6, PbIdleThread(t5)    // get address of idle thread
        cmpeq   t6, t0, t7              // check if idle thread running
        bne     t7, 60f                 // if ne, idle thread running
        LoadByte(t7, ThQuantum(t0))     // get current thread quantum
        sll     t7, 56, t9              //
        sra     t9, 56, t7              //
        subl    t7, CLOCK_QUANTUM_DECREMENT, t7 // decrement quantum
        StoreByte(t7, ThQuantum(t0))    // store thread quantum
        bgt     t7, 60f                 // if gtz, quantum remaining

//
// Put processor specific quantum end DPC in the system DPC list and initiate
// a dispatch interrupt on the current processor.
//
// N.B. Since an interrupt must be active, simply set the software interrupt
//      request bit in the PRCB to request a dispatch interrupt directly from
//      the interrupt exception handler.
//

        ldil    a0, DISPATCH_INTERRUPT  // set interrupt request mask
        stl     a0, PbSoftwareInterrupts(t5) // request software interrupt
        stl     a0, PbQuantumEnd(t5)    // set quantum end indicator
60:     ret     zero, (ra)              // return

//
// Atomic increment of user/kernel time failed.
//

#if !defined(NT_UP)

41:     br      zero, 40b               // retry atomic increment

#endif

        .end    KeUpdateRunTime

//++
//
// VOID
// KeProfileInterrupt (
//    VOID
//    )
//
// VOID
// KeProfileInterruptWithSource (
//    IN KPROFILE_SOURCE ProfileSource
//    )
//
// VOID
// KiProfileInterrupt(
//    IN KPROFILE_SOURCE ProfileSource,
//    IN PKTRAP_FRAME TrapFrame
//    )
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt generated by the
//    profile timer. Its function is to update the profile information for
//    the currently active profile objects.
//
//    N.B. This routine is executed on all processors in a multiprocess system.
//
// Arguments:
//
//    ProfileSource (a0) - Supplies the source of the profile interrupt
//              KeProfileInterrupt is an alternate entry for backwards
//              compatibility that sets the source to zero (ProfileTime)
//
// Return Value:
//
//    None.
//
//--

        .struct 0
PfS0:   .space  8                       // saved integer register s0
PfRa:   .space  8                       // return address
        .space  2 * 8                   // profile frame length
ProfileFrameLength:

        NESTED_ENTRY(KeProfileInterrupt, ProfileFrameLength, zero)

        bis     zero, zero, a0          // set profile source to ProfileTime

        ALTERNATE_ENTRY(KeProfileInterruptWithSource)

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        LDP     a1, PbInterruptTrapFrame(v0) // get trap frame address

        ALTERNATE_ENTRY(KiProfileInterrupt)

        lda     sp, -ProfileFrameLength(sp) // allocate stack frame
        stq     ra, PfRa(sp)            // save return address

#if !defined(NT_UP)

        stq     s0, PfS0(sp)            // save integer register s0

#endif

        PROLOGUE_END

//
// Acquire profile lock.
//

#if !defined(NT_UP)

        lda     s0, KiProfileLock       // get address of profile lock
10:     LDP_L   t0, 0(s0)               // get current lock value
        bis     s0, zero, t1            // set ownership value
        bne     t0, 15f                 // if ne, spin lock owned
        STP_C   t1, 0(s0)               // set spin lock owned
        beq     t1, 15f                 // if eq, store conditional failed
        mb                              // synchronize memory access

#endif

        GET_CURRENT_THREAD              // get current thread address

        LDP     a2, ThApcState + AsProcess(v0) // get current process address
        ADDP    a2, PrProfileListHead, a2 // compute profile listhead address
        bsr     ra, KiProcessProfileList // process profile list
        lda     a2, KiProfileListHead   // get system profile listhead address
        bsr     ra, KiProcessProfileList // process profile list

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(s0)             // set spin lock not owned
        ldq     s0, PfS0(sp)            // restore s0

#endif

        ldq     ra, PfRa(sp)            // restore return address
        lda     sp, ProfileFrameLength(sp)  // deallocate stack frame
        ret     zero, (ra)              // return

//
// Acquire profile lock failed.
//

#if !defined(NT_UP)

15:     LDP     t0, 0(s0)               // get current lock value
        beq     t0, 10b                 // if eq, lock available
        br      zero, 15b               // spin in cache until lock ready

#endif

        .end    KeProfileInterrupt

//++
//
// VOID
// KiProcessProfileList (
//    IN KPROFILE_SOURCE Source,
//    IN PKTRAP_FRAME TrapFrame,
//    IN PLIST_ENTRY ListHead
//    )
//
// Routine Description:
//
//    This routine is called to process a profile list.
//
// Arguments:
//
//    Source (a1) - Supplies profile source to match
//
//    TrapFrame (a0) - Supplies a pointer to a trap frame.
//
//    ListHead (a2) - Supplies a pointer to a profile list.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiProcessProfileList)

        LDP     a3, LsFlink(a2)         // get address of next entry
        cmpeq   a2, a3, t0              // end of list ?
        bne     t0, 30f                 // if ne, end of list
        LDP     t0, TrFir(a1)           // get interrupt PC address

        GET_PROCESSOR_CONTROL_REGION_BASE // get current pcr address

        ldl     t6, PcSetMember(v0)     // get processor member

//
// Scan profile list and increment profile buckets as appropriate.
//

10:     LDP     t1, PfRangeBase - PfProfileListEntry(a3) // get base of range
        LDP     t2, PfRangeLimit - PfProfileListEntry(a3) // get limit of range
        ldl     t4, PfSource - PfProfileListEntry(a3) // get source
        ldl     t7, PfAffinity - PfProfileListEntry(a3) // get affinity
        zapnot  t4, 3, t4               // source is a SHORT
        cmpeq   t4, a0, t5              // check against profile source
        and     t7, t6, v0              // check against processor
        beq     t5, 20f                 // if ne, profile source doesn't match
        beq     v0, 20f                 // if ne, processor doesn't match
        cmpult  t0, t1, v0              // check against range base
        cmpult  t0, t2, t3              // check against range limit
        bne     v0, 20f                 // if ne, less than range base
        beq     t3, 20f                 // if eq, not less than range limit
        SUBP    t0, t1, t1              // compute offset in range
        ldl     t2, PfBucketShift - PfProfileListEntry(a3) // get shift count
        LDP     v0, PfBuffer - PfProfileListEntry(a3) // profile buffer address
        zap     t1, 0xf0, t1            // force bucket offset to 32bit unit
        srl     t1, t2, t3              // compute bucket offset
        bic     t3, 0x3, t3             // clear low order offset bits
        ADDP    v0, t3, t3              // compute bucket address
        ldl     v0, 0(t3)               // increment profile bucket
        addl    v0, 1, v0               //
        stl     v0, 0(t3)               //
20:     LDP     a3, LsFlink(a3)         // get address of next entry
        cmpeq   a2, a3, t1              // end of list?
        beq     t1, 10b                 // if eq[false], more entries
30:     ret     zero, (ra)              // return

        .end    KiProcessProfileList
