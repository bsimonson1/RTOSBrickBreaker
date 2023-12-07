// G8RTOS_IPC.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for FIFO functions for interprocess communication

#include "../G8RTOS_IPC.h"

/************************************Includes***************************************/

#include "../G8RTOS_Semaphores.h"

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

typedef struct G8RTOS_FIFO_t {
    uint32_t bufferArray[FIFO_SIZE]; // what would be the array size be
    uint32_t *pointer_to_head; // points to head of buffer array
    uint32_t *pointer_to_tail; // points to tail of buffer array
    int32_t lostData_counter;
    semaphore_t RoomLeft;
    semaphore_t current_size;
    semaphore_t mutex;
} G8RTOS_FIFO_t;


/***********************************Externs*****************************************/

/********************************Private Variables***********************************/

static G8RTOS_FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];


/********************************Public Functions***********************************/

// G8RTOS_InitFIFO
// Initializes FIFO, points head & tai to relevant buffer
// memory addresses. Returns - 1 if FIFO full, 0 if no error
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_InitFIFO(uint32_t FIFO_index) {
    // Check if FIFO index is out of bounds
    if (FIFO_index > MAX_NUMBER_OF_FIFOS) {
        return -1;
    }
    // Init head, tail pointers
    FIFOs[FIFO_index].pointer_to_head = FIFOs[FIFO_index].bufferArray;
    FIFOs[FIFO_index].pointer_to_tail = FIFOs[FIFO_index].bufferArray;
    // Init the mutex, current size
    G8RTOS_InitSemaphore(&FIFOs[FIFO_index].current_size, 0);
    G8RTOS_InitSemaphore(&FIFOs[FIFO_index].mutex, 1);
    G8RTOS_InitSemaphore(&FIFOs[FIFO_index].RoomLeft, FIFO_SIZE);
    // Init lost data
    FIFOs[FIFO_index].lostData_counter = 0;
    return 0;
}

// G8RTOS_ReadFIFO
// Reads data from head pointer of FIFO.
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_ReadFIFO(uint32_t FIFO_index) {
    // Your code
    // Be mindful of boundary conditions!
    if (FIFO_index > MAX_NUMBER_OF_FIFOS) {
        return -1;
    }

    if (FIFOs[FIFO_index].current_size == FIFO_SIZE) {
        FIFOs[FIFO_index].lostData_counter++;
        return -2;
    }

    G8RTOS_WaitSemaphore(&FIFOs[FIFO_index].current_size);
    G8RTOS_WaitSemaphore(&FIFOs[FIFO_index].mutex);

    uint32_t data = *FIFOs[FIFO_index].pointer_to_head;
    FIFOs[FIFO_index].pointer_to_head++;
    if (FIFOs[FIFO_index].pointer_to_head == &FIFOs[FIFO_index].bufferArray[FIFO_SIZE]) {
        FIFOs[FIFO_index].pointer_to_head = &FIFOs[FIFO_index].bufferArray[0];
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFO_index].mutex);
    G8RTOS_SignalSemaphore(&FIFOs[FIFO_index].RoomLeft);
    return data;
}

// G8RTOS_WriteFIFO
// Writes data to tail of buffer.
// 0 if no error, -1 if out of bounds, -2 if full
// Param uint32_t "FIFO_index": Index of FIFO block
// Param uint32_t "data": data to be written
// Return: int32_t
int32_t G8RTOS_WriteFIFO(uint32_t FIFO_index, uint32_t data) {
    // Your code
    if (FIFO_index > MAX_NUMBER_OF_FIFOS) {
        return -1;
    }

    if (FIFOs[FIFO_index].current_size == FIFO_SIZE) {
        FIFOs[FIFO_index].lostData_counter++;
        return -2;
    }

    G8RTOS_WaitSemaphore(&FIFOs[FIFO_index].RoomLeft);
    G8RTOS_WaitSemaphore(&FIFOs[FIFO_index].mutex);

    *FIFOs[FIFO_index].pointer_to_tail = data;
    FIFOs[FIFO_index].pointer_to_tail++;
    if (FIFOs[FIFO_index].pointer_to_tail == &FIFOs[FIFO_index].bufferArray[FIFO_SIZE]) {
        FIFOs[FIFO_index].pointer_to_tail = &FIFOs[FIFO_index].bufferArray[0];
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFO_index].mutex);
    G8RTOS_SignalSemaphore(&FIFOs[FIFO_index].current_size);
    return 0;
}

