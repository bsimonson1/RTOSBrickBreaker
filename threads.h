// threads.h
// Date Created: 2023-07-26
// Date Updated: 2023-07-26
// Threads

#ifndef THREADS_H_
#define THREADS_H_

/************************************Includes***************************************/

#include "./G8RTOS/G8RTOS.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/

#define PLATFORM_FIFO       0
#define JOYSTICK_FIFO       1
#define BALL_FIFO           2

/*************************************Defines***************************************/

/***********************************Semaphores**************************************/

semaphore_t sem_I2CA;
semaphore_t sem_SPIA;
semaphore_t sem_PCA9555_Debounce;
semaphore_t sem_Joystick_Debounce;;
semaphore_t sem_UART;
semaphore_t sem_PlatformUpdate;
semaphore_t sem_BallUpdate;

/***********************************Semaphores**************************************/

/***********************************Structures**************************************/
/***********************************Structures**************************************/

void game_init(void);
void Collision_Thread(int16_t *reverse_x, int16_t *reverse_y);

/*******************************Background Threads**********************************/

void Idle_Thread(void);
void Cube_Thread(void);
void PlatformMove_Thread(void);
void Ball_Thread(void);
void Read_Buttons(void);

/*******************************Background Threads**********************************/

/********************************Periodic Threads***********************************/

void Get_Joystick(void);
void Score_Update(void);
void Win_Condition(void);
void Lose_Condition(void);

/********************************Periodic Threads***********************************/

/*******************************Aperiodic Threads***********************************/

void GPIOE_Handler(void);

/*******************************Aperiodic Threads***********************************/


#endif /* THREADS_H_ */

