// Brick Breaker, uP2 Fall 2023
// Created: 2023-11-15

/************************************Includes***************************************/

#include "G8RTOS/G8RTOS.h"
#include "./MultimodDrivers/multimod.h"
#include <driverlib/timer.h>

#include "./threads.h"

/************************************MAIN*******************************************/
int main(void)
{
    // Sets clock speed to 80 MHz. You'll need it!
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    G8RTOS_Init();
    multimod_init();

    //G8RTOS_InitFIFO(PLATFORM_FIFO);
    G8RTOS_InitFIFO(JOYSTICK_FIFO);
    //G8RTOS_InitFIFO(BALL_FIFO);

    G8RTOS_AddThread(Idle_Thread, 255, "IDLE", 0);
    G8RTOS_AddThread(Read_Buttons, 254, "buttons\0", 2);
    G8RTOS_Add_APeriodicEvent(GPIOE_Handler, 5, 20);
    G8RTOS_Add_PeriodicEvent(Win_Condition, 10, 3);
    //G8RTOS_Add_PeriodicEvent(Lose_Condition, 25, 2);

    G8RTOS_InitSemaphore(&sem_UART, 1);
    G8RTOS_InitSemaphore(&sem_PCA9555_Debounce, 1);
    G8RTOS_InitSemaphore(&sem_Joystick_Debounce, 1);
    G8RTOS_InitSemaphore(&sem_SPIA, 1);
    G8RTOS_InitSemaphore(&sem_I2CA, 1);
    G8RTOS_InitSemaphore(&sem_BallUpdate, 1);
    G8RTOS_InitSemaphore(&sem_PlatformUpdate, 1);

    G8RTOS_Launch();
    while (1);
}

/************************************MAIN*******************************************/
