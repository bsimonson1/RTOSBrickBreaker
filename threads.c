// G8RTOS_Threads.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"
#include "./BrickBreaker_Structures.h"

#include "./MultimodDrivers/multimod.h"
//#include "./MultimodDrivers/ST7789.h"
//#include "./MultimodDrivers/GFX_Library.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// Change this to change the number of points that make up each line of a cube.
// Note that if you set this too high you will have a stack overflow!
#define PLATFORM_W 50
#define PLATFORM_H 20
#define PLATFORM_Y 30

#define ST7789_WHITE                0xFFFF

#define BLOCK_H 20
#define BLOCK_W 50

#define PLAYER_SPEED 10
#define BALL_SPEED 5

#define DEADZONE_LEFT 3000
#define DEADZONE_RIGHT 2020

#define NUMROWS 5
#define NUMCOLS 6

#define BALL_R 5
#define BALL_W 5
#define BALL_H 5

/*********************************Global Variables**********************************/

bool ball_spawned = false;

bool game_on = false;

bool game_lost = false;

ball_t ball; // global ball

uint16_t score = 0;

blocks_t blocks[NUMROWS][NUMCOLS]; // global blocks

platform_t platform;

uint8_t paused = 0;

// dead blocks counter (20 spawned in)
uint8_t dead_blocks = 0;

uint8_t BALL_Y_SPEED = 5;
uint8_t BALL_X_SPEED = 5;

uint8_t NUM_BALLS = 0;

/*********************************Global Variables**********************************/

/*********************************Game Init*****************************************/
void game_init(void)
{
    // leave space at the top for the score board
    for (int i = 0; i <= NUMROWS; i++)
    {
        for (int j = 0; j < NUMCOLS; j++)
        {
            blocks[i-1][j].x = BLOCK_W * j;
            blocks[i-1][j].y = Y_MAX - BLOCK_H * i;
            blocks[i-1][j].isAlive = true;
            ST7789_DrawRectangle(blocks[i-1][j].x, blocks[i-1][j].y, BLOCK_W, BLOCK_H, rand());
        }
    }
}
/*********************************Game Init*****************************************/

/*************************************Threads***************************************/

void Idle_Thread(void) {
    time_t t;
    srand((unsigned) time(&t));
    while(1);
}

void PlatformMove_Thread(void)
{
    platform.x = 240 / 2;
    while(1)
    {
        // wait for the new and old platform to be drawn and undrawn
        //G8RTOS_WaitSemaphore(&sem_PlatformUpdate);
        if (paused == 1)
        {
            sleep(UINT32_MAX);
            G8RTOS_SignalSemaphore(&sem_PlatformUpdate);
        }
        else
        {
            //G8RTOS_WaitSemaphore(&sem_PlatformUpdate);
            // Get result from joystick
            uint32_t joy_val = G8RTOS_ReadFIFO(JOYSTICK_FIFO);
            uint16_t x_val = joy_val >> 16;
            // set the old position
            platform.old_x = platform.x;
            // if greater than this val, this means we are moving left
            if (x_val > DEADZONE_LEFT)
            {
                platform.x -= PLAYER_SPEED;
            }
            // if less than this val, this means we are moving right
            else if (x_val < DEADZONE_RIGHT)
            {
                platform.x += PLAYER_SPEED;
            }
            if (platform.x > X_MAX - 50)
            {
                platform.x = platform.old_x;
            }
            //G8RTOS_SignalSemaphore(&sem_PlatformUpdate);
            // erase the old position
            G8RTOS_WaitSemaphore(&sem_SPIA);
            ST7789_DrawRectangle(platform.old_x, PLATFORM_Y, PLATFORM_W, PLATFORM_H, ST7789_BLACK);
            G8RTOS_SignalSemaphore(&sem_SPIA);
            // draw the new position
            G8RTOS_WaitSemaphore(&sem_SPIA);
            ST7789_DrawRectangle(platform.x, PLATFORM_Y, PLATFORM_W, PLATFORM_H, ST7789_WHITE);
            G8RTOS_SignalSemaphore(&sem_SPIA);
            // update the position FIFO for collision detection
            //G8RTOS_WriteFIFO(PLATFORM_FIFO, platform.x);
            // signal the semaphore since the new platform has been drawn
            //G8RTOS_SignalSemaphore(&sem_PlatformUpdate);
            sleep(1);
        }
    }
}

void Ball_Thread(void)
{
    //uint32_t ballXY = 0;
    uint8_t y_boundary_hit = 0;
    uint8_t x_boundary_hit = 0;
    uint16_t old_ball_x = 0;
    uint16_t old_ball_y = 0;
    while(1)
    {
        // wait for the ball to update
        //G8RTOS_WaitSemaphore(&sem_BallUpdate);
        if (paused == 1)
        {
            G8RTOS_SignalSemaphore(&sem_BallUpdate);
            sleep(UINT32_MAX);
        }
        else
        {
            if (NUM_BALLS == 0)
            {
                NUM_BALLS = 1;
                ball.x = (platform.x + 50);
                ball.y = (platform.x + 50);
                ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_RED);
            }
            old_ball_x = ball.x;
            old_ball_y = ball.y;
            if (!y_boundary_hit)
            {
                ball.y += BALL_Y_SPEED;
            }
            else
            {
                ball.y -= BALL_Y_SPEED;
            }
            if (!x_boundary_hit)
            {
                ball.x += BALL_X_SPEED;
            }
            else
            {
                ball.x -= BALL_X_SPEED;
            }
            if (ball.x >= 230)
            {
                x_boundary_hit = 1;
            }
            else if (ball.x <= 10)
            {
                x_boundary_hit = 0;
            }
            if (ball.y >= 310)
            {
                y_boundary_hit = 1;
            }
            else if (ball.y <= 30)
            {
                game_lost = true;
                NUM_BALLS = 0;
                UARTprintf("Game Lost!\n");
            }
            if (((ball.x >= platform.x) && (ball.x <= platform.x + 50)) && ball.y == 45)
            {
                y_boundary_hit = 0;
                // we want a variable ball x speed depending on where it hits the platform
                if (((ball.x >= platform.x) && (ball.x <= platform.x + 10)) || ((ball.x >= (platform.x + 40)) && (ball.x <= platform.x + 50)))
                {
                    BALL_X_SPEED = 1;
                }
                else if (((ball.x >= platform.x + 11) && (ball.x <= platform.x + 20)) || ((ball.x >= (platform.x + 30)) && (ball.x <= platform.x + 39)))
                {
                    BALL_X_SPEED = 3;
                }
                else if (((ball.x >= platform.x + 21) && (ball.x <= platform.x + 29)))
                {
                    BALL_X_SPEED = 5;
                }
            }
            for (int i = 0; i < NUMROWS; i++)
            {
                for (int j = 0; j < NUMCOLS; j++)
                {
                    if (blocks[i][j].isAlive && ball.x + BALL_W >= blocks[i][j].x &&
                            ball.x <= blocks[i][j].x + BLOCK_W && ball.y + BALL_H >= blocks[i][j].y && ball.y <= blocks[i][j].y + BLOCK_H)
                    {
                        dead_blocks++;
                        blocks[i][j].isAlive = false;
                        ST7789_DrawRectangle(blocks[i][j].x, blocks[i][j].y, BLOCK_W, BLOCK_H, ST7789_BLACK);
                        // Calculate overlaps on both axes
                        float overlapX, overlapY;
                        if (ball.speedX > 0)
                        {
                            // Ball is moving right
                            overlapX = (blocks[i][j].x + BLOCK_W) - ball.x;
                        }
                        else
                        {
                            // Ball is moving left
                            overlapX = (ball.x + BALL_W) - blocks[i][j].x;
                        }

                        if (ball.speedY > 0)
                        {
                            // Ball is moving down
                            overlapY = (blocks[i][j].y + BLOCK_H) - ball.y;
                        }
                        else
                        {
                            // Ball is moving up
                            overlapY = (ball.y + BALL_H) - blocks[i][j].y;
                        }

                        // Determine collision face based on smaller overlap
                        if (overlapX < overlapY)
                        {
                            // Collision is on the X face
    //                        ball.speedX *= -1;
                            x_boundary_hit = !x_boundary_hit;
                        }
                        else
                        {
                            // Collision is on the Y face
    //                        ball.speedY *= -1;
                            y_boundary_hit = !y_boundary_hit;
                        }
                    }
                }
            }
            ST7789_DrawRectangle(old_ball_x, old_ball_y, BALL_W, BALL_H, ST7789_BLACK);
            ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_RED);
            ballXY = (((ball.x) << 16) && (ball.y));
            // update the balls position in the FIFO
            //G8RTOS_WriteFIFO(BALL_FIFO, ballXY);
            // ball has finished updating signal the semaphore
            //G8RTOS_SignalSemaphore(&sem_BallUpdate);
            sleep(100);
        }
    }
}

void Read_Buttons()
{
    // Initialize / declare any variables here
    while(1)
    {
        // Wait for a signal to read the buttons on the Multimod board.
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);
        // Sleep to debounce
        sleep(10);
        // Read the buttons status on the Multimod board.
        uint8_t buttonVal = MultimodButtons_Get();
        // Process the buttons and determine what actions need to be performed.
        // SW1 = 253 SW2 = 251 SW3 = 247 SW4 = 239
        if (buttonVal == 253) //sw1 spawn a ball and start game
        {
            UARTprintf("This is sw1\n");
            if (!ball_spawned && !game_on)
            {
                ball_spawned = true;
                game_on = true;
                // initialize the game
                game_init();
                G8RTOS_AddThread(PlatformMove_Thread, 253, "platform\0", 1);
                G8RTOS_AddThread(Ball_Thread, 253, "ball_thread\0", 3);
                G8RTOS_Add_PeriodicEvent(Get_Joystick, 50, 1); // x max is left
                //G8RTOS_Add_PeriodicEvent(Score_Update, 25, 1);
            }
        }
        // if lose/win condition is met use this to restart
        else if (buttonVal == 251)
        {
            UARTprintf("This is sw2, restarting the game!\n");
            if (!ball_spawned && !game_on)
            {
                ST7789_Fill(ST7789_BLACK);
                ball_spawned = true;
                game_on = true;
                game_lost = false;
                platform.x = 240 / 2;
                // re-initialize the game
                game_init();
                G8RTOS_AddThread(PlatformMove_Thread, 253, "platform\0", 1);
                G8RTOS_AddThread(Ball_Thread, 253, "ball_thread\0", 3);
            }
        }
        // pause feature, need to fix for joystick fifo (messes up when paused)
        else if (buttonVal == 247)
        {
            UARTprintf("This is sw3, pausing the game!\n");
            if (paused == 0)
            {
                paused = 1;
                //G8RTOS_AddThread(Pause_Thread, 1, "pause\0", 5);
            }
            else
            {
                paused = 0;
            }
        }
        // Clear the interrupt
        GPIOIntClear(BUTTONS_INT_GPIO_BASE, BUTTONS_INT_PIN);
        // Re-enable the interrupt so it can occur again.
        GPIOIntEnable(BUTTONS_INT_GPIO_BASE, BUTTONS_INT_PIN);
        sleep(1);
    }
}

/********************************Periodic Threads***********************************/

void Get_Joystick(void)
{
    if (paused)
    {
        sleep(UINT32_MAX);
        //UINT32_MAX
    }
    else if (!ball_spawned && !game_on)
    {
        sleep(UINT32_MAX);
    }
    else
    {
        // Read the joystick
        uint32_t TotalXY = JOYSTICK_GetXY();
        // Send through FIFO.
        G8RTOS_WriteFIFO(JOYSTICK_FIFO, TotalXY); // upper 16 is x
    }
}

void Score_Update(void)
{
//    score += 1;
//    display_drawChar(120, 100, 83, ST7789_WHITE, ST7789_WHITE, 50);
//    display_drawChar(50, 28, 67, ST7789_WHITE, ST7789_BLACK, 10);
//    display_drawChar(60, 28, 79, ST7789_WHITE, ST7789_BLACK, 10);
//    display_drawChar(70, 28, 82, ST7789_WHITE, ST7789_BLACK, 10);
//    display_drawChar(80, 28, 69, ST7789_WHITE, ST7789_BLACK, 10);
//    char s = score;
//    display_drawChar(90, 28, s, ST7789_WHITE, ST7789_BLACK, 10);
}

void Win_Condition(void)
{
    if (dead_blocks >= 24)
    {
        UARTprintf("Game Won!\n");
        G8RTOS_KillThread(1);
        G8RTOS_KillThread(3);
        dead_blocks = 0;
        ball_spawned = false;
        game_on = false;
        NUM_BALLS = 0;
    }
    if (game_lost)
    {
        G8RTOS_KillThread(1);
        G8RTOS_KillThread(3);
        ball_spawned = false;
        game_on = false;
        NUM_BALLS = 0;
        dead_blocks = 0;
    }
}

/*******************************Aperiodic Threads***********************************/

void GPIOE_Handler()
{
    // Disable interrupt
    GPIOIntDisable(BUTTONS_INT_GPIO_BASE, BUTTONS_INT_PIN);
    // Signal relevant semaphore
    G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
}

