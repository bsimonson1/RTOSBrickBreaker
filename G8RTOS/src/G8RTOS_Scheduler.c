// G8RTOS_Scheduler.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for scheduler functions

#include "../G8RTOS_Scheduler.h"

/************************************Includes***************************************/

#include <stdint.h>
#include <stdbool.h>

#include "../G8RTOS_CriticalSection.h"

#include <inc/hw_memmap.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

/********************************Private Variables**********************************/

// Thread Control Blocks - array to hold information for each thread
static tcb_t threadControlBlocks[MAX_THREADS];

// Thread Stacks - array of arrays for individual stacks of each thread
static uint32_t threadStacks[MAX_THREADS][STACKSIZE];

// Periodic Event Threads - array to hold pertinent information for each thread
static ptcb_t pthreadControlBlocks[MAX_PTHREADS];

// Current Number of Threads currently in the scheduler
static uint32_t NumberOfThreads;

// Current Number of Periodic Threads currently in the scheduler
static uint32_t NumberOfPThreads;

//static uint32_t threadCounter = 0;

/*******************************Private Functions***********************************/

// Occurs every 1 ms.
static void InitSysTick(void)
{
    // Replace with code from lab 3
    // hint: use SysCtlClockGet() to get the clock speed without having to hardcode it!
    // Period = sys_clk / Prescaler * Desired time
    // Set systick period
    SysTickDisable();
    SysTickPeriodSet((SysCtlClockGet() / 1) / 1000); // float value might be causing issues (also figure out the prescaler value) 16000
    // Set systick interrupt handler
    SysTickIntRegister(SysTick_Handler);
    // Set pendsv handler
    IntRegister(FAULT_PENDSV, PendSV_Handler);
    // Enable systick interrupt
    SysTickIntEnable();
    // Enable systick
    SysTickEnable();
    IntMasterEnable();
}

/********************************Public Variables***********************************/
uint32_t SystemTime = 0;

tcb_t* CurrentlyRunningThread;
/********************************Public Functions***********************************/

// SysTick_Handler
// Increments system time, sets PendSV flag to start scheduler.
// Return: void
void SysTick_Handler() {
    SystemTime += 1;
    // need to set the PendSV flag
    // Traverse the linked-list to find which threads should be awake.
    uint32_t counter = 0;
    tcb_t* temp = &threadControlBlocks[0]; //&threadControlBlocks[0]; CurrentlyRunningThread;
    while (counter < MAX_THREADS) // NumberOfThreads
    {
        if ((temp->asleep) && (SystemTime >= temp->sleepCount))
        {
            temp->sleepCount = 0;
            temp->asleep = false;
        }
        temp = temp->nextTCB;
        ++counter;
    }
    // Traverse the periodic linked list to run which functions need to be run.
    counter = 0;
    if (NumberOfPThreads >= 1)
    {
        ptcb_t* pthread = &pthreadControlBlocks[0];
        while (counter < NumberOfPThreads)
        {
            // need to check the interrupt here and if the pthread needs to be run
            if (pthread->executeTime == SystemTime)
            {
                pthread->executeTime = pthread->executeTime + pthread->period; // add the period of execution to the execute time interval
                pthread->handler(); // call the task/thread explicitly
            }
            pthread = pthread->nextPTCB;
            counter++;
        }
    }
    IntPendSet(FAULT_PENDSV);
    return;
}

// G8RTOS_Init
// Initializes the RTOS by initializing system time.
// Return: void
void G8RTOS_Init() {
    uint32_t newVTORTable = 0x20000000;
    uint32_t* newTable = (uint32_t*)newVTORTable;
    uint32_t* oldTable = (uint32_t*) 0;

    for (int i = 0; i < 155; i++)
    {
        newTable[i] = oldTable[i];
    }

    HWREG(NVIC_VTABLE) = newVTORTable;

    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPThreads = 0;
}

// G8RTOS_Launch
// Launches the RTOS.
// Return: error codes, 0 if none
int32_t G8RTOS_Launch() {
    // Replace with code from lab 3
    // Initialize system tick
    InitSysTick();
    // Set currently running thread to the first control block
    CurrentlyRunningThread = &threadControlBlocks[0];
    // Set interrupt priorities
       // Pendsv
       // Systick
    IntPrioritySet(FAULT_SYSTICK, 7);
    IntPrioritySet(FAULT_PENDSV, 7);
    // Call G8RTOS_Start()
    G8RTOS_Start();
    // Set interrupt prio
    return 0;
}

// G8RTOS_Scheduler
// Chooses next thread in the TCB. This time uses priority scheduling.
// Return: void
void G8RTOS_Scheduler() {
    // Using priority, determine the most eligible thread to run that
    // is not blocked or asleep. Set current thread to this thread's TCB.
    uint32_t max = 255; // max priority is 255
    tcb_t* current = &threadControlBlocks[0];
    tcb_t* highest = 0;
    uint32_t counter = 0;
    for (tcb_t* t = current; counter < MAX_THREADS; t = t->nextTCB)
   {
       if (t->priority <= max)
       {
           // maybe change this to || rather than &&
           if ((t->blocked == 0) && (t->asleep == false) && (t->isAlive))
           {
               max = t->priority;
               highest = t;
           }
       }
       counter++;
   }
   if (highest == 0)
   {
       highest = &threadControlBlocks[0];
   }
   CurrentlyRunningThread = highest;
}

// G8RTOS_AddThread
// Adds a thread. This is now in a critical section to support dynamic threads.
// It also now should initalize priority and account for live or dead threads.
// Param void* "threadToAdd": pointer to thread function address
// Param uint8_t "priority": priority from 0, 255.
// Param char* "name": character array containing the thread name.
// Return: sched_ErrCode_t
//sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name, threadID_t ThreadID) {
////sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name) {
//    // Your code here
//    uint32_t startCrit = StartCriticalSection();
//    //If number of threads is greater than or equal to the maximum number of threads
//    if (NumberOfThreads >= MAX_THREADS)
//    {
//        EndCriticalSection(startCrit);
//        return THREAD_LIMIT_REACHED;
//    }
//    else
//    {
//        // Initialize the new thread's control block
//        //tcb_t* newThread = &threadControlBlocks[NumberOfThreads];
//        uint32_t counter = 0;
//        tcb_t* newThread;
//        while (counter < MAX_THREADS)
//        {
//            if (!threadControlBlocks[counter].isAlive)
//            {
//                newThread = &threadControlBlocks[counter];
//                break;
//            }
//            counter++;
//        }
//        newThread->stackPointer = &threadStacks[NumberOfThreads][STACKSIZE - 16]; // Set the stack pointer to the top of the stack
//        newThread->priority = priority; // set the new threads priority
//        for (int i = 0; i < MAX_NAME_LENGTH - 1; i++)
//        { //  && name[i] != '\0'
//            newThread->threadName[i] = name[i];
//        }
//        newThread->isAlive = true;
//        // do i need to assign a threadID here?
//        newThread->ThreadID = ThreadID;
//
//        // Set up the new thread's stack frame with a "fake context"
//        // change from number of threads to counter
//        threadStacks[counter][STACKSIZE - 1] = 0x01000000; // thumbit
//        threadStacks[counter][STACKSIZE - 3] = 0x14141414; // r14
//        threadStacks[counter][STACKSIZE - 4] = 0x12121212; // r12
//        threadStacks[counter][STACKSIZE - 5] = 0x03030303; // r3
//        threadStacks[counter][STACKSIZE - 6] = 0x02020202; // r2
//        threadStacks[counter][STACKSIZE - 7] = 0x01010101; // r1
//        threadStacks[counter][STACKSIZE - 8] = 0x00000000; // r0
//        threadStacks[counter][STACKSIZE - 9] = 0x11111111; // r11
//        threadStacks[counter][STACKSIZE - 10] = 0x10101010; // r10
//        threadStacks[counter][STACKSIZE - 11] = 0x09090909; // r9
//        threadStacks[counter][STACKSIZE - 12] = 0x08080808; // r8
//        threadStacks[counter][STACKSIZE - 13] = 0x07070707; // r7
//        threadStacks[counter][STACKSIZE - 14] = 0x06060606; // r6
//        threadStacks[counter][STACKSIZE - 15] = 0x05050505; // r5
//        threadStacks[counter][STACKSIZE - 16] = 0x04040404; // r4
//
//        // Update the linked list to include the new thread
//        if (counter == 0)
//        {
//            threadControlBlocks[0].nextTCB = &threadControlBlocks[0];
//            threadControlBlocks[0].previousTCB = &threadControlBlocks[0];
//            threadStacks[counter][STACKSIZE - 2] = threadToAdd; // STACKSIZE - 2 is R15 or the link register
//        }
//        else
//        {
//            threadControlBlocks[counter].nextTCB = &threadControlBlocks[0];
//            threadControlBlocks[counter].previousTCB = &threadControlBlocks[counter-1];
//            threadControlBlocks[counter-1].nextTCB = &threadControlBlocks[counter];
//            threadControlBlocks[0].previousTCB = &threadControlBlocks[counter];
////            tcb_t* currentHead = &threadControlBlocks[0];
////            threadControlBlocks[counter].nextTCB = currentHead;
////            threadControlBlocks[counter].previousTCB = &threadControlBlocks[counter-1];
////            currentHead->previousTCB = &threadControlBlocks[counter]; // only need to update the head previous pointer to the new tail
////            threadControlBlocks[counter-1].nextTCB = &threadControlBlocks[counter]; // this updates the head next on the second insertion
//            threadStacks[counter][STACKSIZE - 2] = threadToAdd; // STACKSIZE - 2 is R15 or the link register
//        }
//        // Increment the number of threads
//        NumberOfThreads++;
//    }
//    // This should be in a critical section!
//    EndCriticalSection(startCrit);
//    return NO_ERROR;
//}

sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name, threadID_t threadCounter) {//added check for live threads
    // Your code here
    uint32_t temp = StartCriticalSection();
    uint8_t liveThread = 0;
    if(NumberOfThreads > MAX_THREADS)
        {
            EndCriticalSection(temp);
            return THREAD_LIMIT_REACHED;
        }
    else{


        if(NumberOfThreads == 0)
        {
            threadControlBlocks[0].nextTCB = &threadControlBlocks[0];
            threadControlBlocks[0].previousTCB = &threadControlBlocks[0];
        }
        else
        {
            while(liveThread < MAX_THREADS)
            {
                if(threadControlBlocks[liveThread].isAlive == false)
                {
                    break;
                }
                liveThread++;

            }//liveThread
            if(liveThread == MAX_THREADS)
            {
                EndCriticalSection(temp);
                return THREAD_DOES_NOT_EXIST;
            }

            threadControlBlocks[liveThread].nextTCB = &threadControlBlocks[0];//g
            //3. Set the current thread's nextTCB to be the new thread
            threadControlBlocks[liveThread].previousTCB = threadControlBlocks[0].previousTCB;
            //4. Set the first thread's previous thread to be the new thread, so that it goes in the right spot in the list
            threadControlBlocks[0].previousTCB = &threadControlBlocks[liveThread];
            //5. Point the previousTCB of the new thread to the current thread so that it moves in the correct order
            threadControlBlocks[liveThread].previousTCB->nextTCB = &threadControlBlocks[liveThread];
        }
        threadControlBlocks[liveThread].stackPointer = &threadStacks[liveThread][STACKSIZE - 16];
        threadStacks[liveThread][STACKSIZE - 2] = threadToAdd;
        threadStacks[liveThread][STACKSIZE - 1] = THUMBBIT;
        threadControlBlocks[liveThread].blocked = 0;
        threadControlBlocks[liveThread].sleepCount = 0;
        threadControlBlocks[liveThread].asleep = false;
        threadControlBlocks[liveThread].ThreadID = threadCounter;
        threadCounter++;
        threadControlBlocks[liveThread].priority = priority;
        threadControlBlocks[liveThread].isAlive = true;

        for(uint16_t x = 0; x < MAX_NAME_LENGTH; x++)
        {
            threadControlBlocks[liveThread].threadName[x] = name[x];
        }
        NumberOfThreads++;
    }
    EndCriticalSection(temp);
    return NO_ERROR;
}


// G8RTOS_Add_APeriodicEvent
// Param void* "AthreadToAdd": pointer to thread function address
// Param int32_t "IRQn": Interrupt request number that references the vector table. [0..155].
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_APeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn) {
    // Disable interrupts
    //IntMasterDisable();
    uint32_t startCrit = StartCriticalSection();
    // Check if IRQn is valid
    if (IRQn >  155 || IRQn <  0) //  0x20000000 + (154*4) contiguous memory of 4 bits
    {
        EndCriticalSection(startCrit);
        return IRQn_INVALID;
    }
    // Check if priority is valid
    if (priority > 6)
    {
        EndCriticalSection(startCrit);
        return HWI_PRIORITY_INVALID;
    }
    // Set corresponding index in interrupt vector table to handler. page 134
    uint32_t newVTORTable = HWREG(NVIC_VTABLE);
    uint32_t* newTable = (uint32_t*)newVTORTable;

    newTable[IRQn] = (uint32_t)(AthreadToAdd);

    // Set priority.
    IntPrioritySet(IRQn, priority);
    // Enable the interrupt.
    IntEnable(IRQn);
    // End the critical section.
    EndCriticalSection(startCrit);
    return NO_ERROR;
}

// G8RTOS_Add_PeriodicEvent
// Adds periodic threads to G8RTOS Scheduler
// Function will initialize a periodic event struct to represent event.
// The struct will be added to a linked list of periodic events
// Param void* "PThreadToAdd": void-void function for P thread handler
// Param uint32_t "period": period of P thread to add
// Param uint32_t "execution": When to execute the periodic thread
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_PeriodicEvent(void (*PThreadToAdd)(void), uint32_t period, uint32_t execution) {
    // your code
    // Make sure that the number of PThreads is not greater than max PThreads.
    uint32_t startCrit = StartCriticalSection();

    if (NumberOfPThreads > MAX_PTHREADS)
    {
        EndCriticalSection(startCrit);
        return THREAD_LIMIT_REACHED;
    }
    // Check if there is no PThread. Initialize and set the first PThread.
    if (NumberOfPThreads == 0)
    {
        pthreadControlBlocks[NumberOfPThreads].nextPTCB = &pthreadControlBlocks[NumberOfPThreads];
        pthreadControlBlocks[NumberOfPThreads].previousPTCB = &pthreadControlBlocks[NumberOfPThreads];
    }
    // Subsequent PThreads should be added, inserted similarly to a doubly-linked linked list
            // last PTCB should point to first, last PTCB should point to last.
    else
    {
        // newly added thread is set
        pthreadControlBlocks[NumberOfPThreads].nextPTCB = &pthreadControlBlocks[0]; // newly added thread node next will point to the head
        pthreadControlBlocks[NumberOfPThreads].previousPTCB = &pthreadControlBlocks[NumberOfPThreads-1]; // point to the previous index

        pthreadControlBlocks[NumberOfPThreads-1].nextPTCB = &pthreadControlBlocks[NumberOfPThreads]; // get the previous node (before the newly added node) and update it's next pointer
        pthreadControlBlocks[0].previousPTCB = &pthreadControlBlocks[NumberOfPThreads]; // set the previous pointer of the head to the new last node
    }
    // Set function
    pthreadControlBlocks[NumberOfPThreads].handler = PThreadToAdd;
    // Set period
    pthreadControlBlocks[NumberOfPThreads].period = period;
    // Set execute time
    pthreadControlBlocks[NumberOfPThreads].executeTime = execution + SystemTime;
    // Increment number of PThreads
    NumberOfPThreads++;
    EndCriticalSection(startCrit);
    return NO_ERROR;
}

// G8RTOS_KillThread
// Param uint32_t "threadID": ID of thread to kill
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillThread(threadID_t threadID) {
    // Start critical section
    uint32_t startCrit = StartCriticalSection();
    // Check if there is only one thread, return if so
    if (NumberOfThreads == 1)
    {
        if (CurrentlyRunningThread->ThreadID == threadID)
        {
            // what to do if there is only one running thread (this is the IDLE thread)
            EndCriticalSection(startCrit);
            return NO_ERROR;
        }
        else
        {
            EndCriticalSection(startCrit);
            return THREAD_DOES_NOT_EXIST;
        }
    }
    // Traverse linked list, find thread to kill
    uint32_t counter = 0;
    tcb_t* next = CurrentlyRunningThread;
    while (counter < NumberOfThreads) // at least 2 threads if here
    {
        if (next->ThreadID == threadID && next == CurrentlyRunningThread)
        {
            G8RTOS_KillSelf();
            EndCriticalSection(startCrit);
            return NO_ERROR;
        }
        else if (next->ThreadID == threadID)
        {
            // Update the next tcb and prev tcb pointers if found
            // mark as not alive, release the semaphore it is blocked on
            next->isAlive = false; // thread is no longer alive
            G8RTOS_SignalSemaphore(next->blocked);
            next->nextTCB->previousTCB = next->previousTCB;
            next->previousTCB->nextTCB = next->nextTCB;
            NumberOfThreads--;
            EndCriticalSection(startCrit);
            return NO_ERROR;
        }
        counter++;
        next = next->nextTCB;
    }
    EndCriticalSection(startCrit);
    return THREAD_DOES_NOT_EXIST;
}

// G8RTOS_KillSelf
// Kills currently running thread.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillSelf() {
    // your code
    uint32_t startCrit = StartCriticalSection();
    // Check if there is only one thread
    tcb_t* next = CurrentlyRunningThread;
    if (NumberOfThreads > 1)
    {
        next->isAlive = false; // thread is no longer alive
        G8RTOS_SignalSemaphore(next->blocked);
        next->nextTCB->previousTCB = next->previousTCB;
        next->previousTCB->nextTCB = next->nextTCB;
        NumberOfThreads--;
    }
    EndCriticalSection(startCrit);
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    return NO_ERROR;
}

// sleep
// Puts current thread to sleep
// Param uint32_t "durationMS": how many systicks to sleep for
void sleep(uint32_t durationMS) {
    // Update time to sleep to
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;
    //CurrentlyRunningThread->sleepCount = durationMS;
    // Set thread as asleep
    CurrentlyRunningThread->asleep = true;
    //IntPendSet(FAULT_PENDSV);
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

// G8RTOS_GetThreadID
// Gets current thread ID.
// Return: threadID_t
threadID_t G8RTOS_GetThreadID(void) {
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

// G8RTOS_GetNumberOfThreads
// Gets number of threads.
// Return: uint32_t
uint32_t G8RTOS_GetNumberOfThreads(void) {
    return NumberOfThreads;         //Returns the number of threads
}
