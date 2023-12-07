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

int16_t BALL_X_SPEED = 4;
int16_t BALL_Y_SPEED = 4;

int killed_blocks[3][2];

/*********************************Global Variables**********************************/

/*********************************Game Init*****************************************/
void game_init(void)
{
    //G8RTOS_WaitSemaphore(&sem_SPIA);
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
    //G8RTOS_SignalSemaphore(&sem_SPIA);
}
/*********************************Game Init*****************************************/

//check collisions

void Collision_Thread(int16_t *reverse_x, int16_t *reverse_y)
{
    // remember that the ball FIFO has x pos as the upper 16 bits and the y pos as the lower 16 bits
//    ballXY = G8RTOS_ReadFIFO(BALL_FIFO);
//    ball_x = ballXY >> 16;
//    ball_y = ballXY;
//
//    platform.x_pos = G8RTOS_ReadFIFO(PLATFORM_FIFO);

    int i = 0;
    int j = 0;
    int ki = 0;
    int num_of_isAlive_blocks = 0;
    for(i = 0 ;i < 3;i++ )
    {
        for(j =0;j< 2; j++)
        {
            killed_blocks[i][j] = -1;
        }
    }
    for(i =0;i<NUMROWS;i++)
    {
        for(j=0;j<NUMCOLS;j++)
        {
            if(blocks[i][j].isAlive)
            {
                num_of_isAlive_blocks++;
            }
            //detect collision with block
            if(blocks[i][j].isAlive && (ball.x >= blocks[i][j].x - BALL_W && ball.x + BALL_W <= blocks[i][j].x + BLOCK_W + BALL_W) && (ball.y + BALL_H  >= blocks[i][j].y && ball.y + BALL_H <= blocks[i][j].y + BLOCK_H || ball.y <= blocks[i][j].y + BLOCK_H && ball.y >= blocks[i][j].y))
            {
                //detect if ball collidied with the side edge of the block
                if(ball.y + BALL_H > blocks[i][j].y && ball.y < blocks[i][j].y + BLOCK_H)
                {
                    killed_blocks[ki][0] = i;
                    killed_blocks[ki][1] = j;
                    ki++;
                    blocks[i][j].isAlive = 0;
                    *reverse_x = 1;
                }
                //detect if ball collided with bottom or top of block
                if((ball.y + BALL_H > blocks[i][j].y || ball.y < blocks[i][j].y + BLOCK_H) && ball.x + BALL_W > blocks[i][j].x && ball.x < blocks[i][j].x + BLOCK_W)
                {
                    killed_blocks[ki][0] = i;
                    killed_blocks[ki][1] = j;
                    ki++;
                    blocks[i][j].isAlive = 0;
                    *reverse_y = 1;
                }
            }
        }
    }
    if(!num_of_isAlive_blocks)
    {
        //win = 1;
    }
    //sleep(1);
}

// check collisions

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
        ST7789_DrawRectangle(platform.old_x, PLATFORM_Y, PLATFORM_W, PLATFORM_H, ST7789_BLACK);
        // draw the new position
        ST7789_DrawRectangle(platform.x, PLATFORM_Y, PLATFORM_W, PLATFORM_H, ST7789_WHITE);
        // update the position FIFO for collision detection
        G8RTOS_WriteFIFO(PLATFORM_FIFO, platform.x);
        sleep(1);
    }
}

//void Ball_Thread(void)
//{
//    // add vars here
//    uint8_t NUM_BALLS = 0;
//    uint32_t ballXY = 0;
//
//    int16_t reverse_x = 0;
//    int16_t reverse_y = 0;
//
//    while (1)
//    {
//        if (NUM_BALLS == 0)
//        {
//            // spawn a ball here
//            NUM_BALLS++;
//            // will draw a rectangle for now
//            ball.x = (platform.x);
//            ball.y = (platform.x);
//            ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_RED);
//            //display_drawCircle(ball.x, ball.y, BALL_R, ST7789_RED);
//        }
////        old_ball_x = ball.x;
////        old_ball_y = ball.y;
//        // start the ball at the platform heading in the pos x and pos y direction
//        Collision_Thread(&reverse_x, &reverse_y);
//        if (ball.x + BALL_W >= X_MAX || ball.x <= 0 || reverse_x)
//        {
//                ball.speedX *= -1;
//        }
//        if((ball.x + BALL_W >= platform.x && ball.x <= platform.x + PLATFORM_W && ball.y <= PLATFORM_Y + PLATFORM_H)|| reverse_y || ball.y + BALL_H >= Y_MAX) {
//                ball.speedY *= -1;
//        }
//        //erase after images of ball
//        if (ball.speedY > 0)
//        {
//            ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_BLACK);
//        }
//        else
//        {
//            ST7789_DrawRectangle(ball.x + BALL_W-1, ball.y, BALL_W, BALL_H,  ST7789_BLACK);
//        }
//        if (ball.speedX > 0)
//        {
//            ST7789_DrawRectangle(ball.x, ball.y, BALL_W, 1, ST7789_BLACK);
//        }
//        else
//        {
//            ST7789_DrawRectangle(ball.x, ball.y + BALL_W - 1, BALL_W, BALL_H, ST7789_BLACK);
//        }
//        ball.x += ball.speedX;
//        ball.y += ball.speedY;
//        reverse_x = 0;
//        reverse_y = 0;
//        //display_drawCircle(ball.x, ball.y, BALL_R, ST7789_RED);
//        // erase the balls old position
////        ST7789_DrawRectangle(old_ball_x, old_ball_y, BALL_R, BALL_R, ST7789_BLACK);
////        // draw the balls new position
//        ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_RED);
//        // concatenate the ball x and y positions
//        ballXY = (((ball.x) << 16) && (ball.y));
//        // update the balls position in the FIFO
//        G8RTOS_WriteFIFO(BALL_FIFO, ballXY);
//        sleep(1);
//    }
//}

void Ball_Thread(void)
{
    uint8_t NUM_BALLS = 0;
    uint32_t ballXY = 0;
    uint8_t y_boundary_hit = 0;
    uint8_t x_boundary_hit = 0;
    uint16_t old_ball_x = 0;
    uint16_t old_ball_y = 0;
    while(1)
    {
        if (NUM_BALLS == 0)
        {
            NUM_BALLS = 1;
            ball.x = (platform.x);
            ball.y = (platform.x);
            ST7789_DrawRectangle(ball.x, ball.y, BALL_W, BALL_H, ST7789_RED);
        }
        old_ball_x = ball.x;
        old_ball_y = ball.y;
        if (!y_boundary_hit)
        {
            ball.y += BALL_SPEED;
        }
        else
        {
            ball.y -= BALL_SPEED;
        }
        if (!x_boundary_hit)
        {
            ball.x += BALL_SPEED;
        }
        else
        {
            ball.x -= BALL_SPEED;
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
        else if (ball.y <= 10)
        {
            y_boundary_hit = 0;
        }
        if (((ball.x >= platform.x) && (ball.x <= platform.x + 50)) && ball.y == 45)
        {
            y_boundary_hit = 0;
        }
//        for(int i = 0; i < NUMROWS; i++)
//        {
//            for(int j = 0; j < NUMCOLS; j++)
//            {
//                // Check if the ball is within the horizontal and vertical boundaries of the brick
//                if (blocks[i][j].isAlive && ball.x + BALL_W >= blocks[i][j].x && ball.x <= blocks[i][j].x + BLOCK_W && ball.y + BALL_H >= blocks[i][j].y && ball.y <= blocks[i][j].y + BLOCK_H)
//                {
//                    // Ball has collided with the brick, mark the brick as not alive
//                    blocks[i][j].isAlive = false;
//
//                    // Erase the brick from the screen
//                    ST7789_DrawRectangle(blocks[i][j].x, blocks[i][j].y, BLOCK_W, BLOCK_H, ST7789_BLACK);
//
//                }
//            }
//        }
        for (int i = 0; i < NUMROWS; i++)
        {
            for (int j = 0; j < NUMCOLS; j++)
            {
                if (blocks[i][j].isAlive && ball.x + BALL_W >= blocks[i][j].x &&
                        ball.x <= blocks[i][j].x + BLOCK_W && ball.y + BALL_H >= blocks[i][j].y && ball.y <= blocks[i][j].y + BLOCK_H)
                {
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
        G8RTOS_WriteFIFO(BALL_FIFO, ballXY);
        sleep(100);
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
        if (buttonVal == 253) //sw1 spawn a ball
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
        // if lose condition is met use this to restart
        else if (buttonVal == 251)
        {
            UARTprintf("This is sw2\n");
            if (game_on && game_lost)
            {
                game_on = false;
                game_lost = false;
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
    // Read the joystick
    uint32_t TotalXY = JOYSTICK_GetXY();
    // Send through FIFO.
    G8RTOS_WriteFIFO(JOYSTICK_FIFO, TotalXY); // upper 16 is x
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

/*******************************Aperiodic Threads***********************************/

void GPIOE_Handler()
{
    // Disable interrupt
    GPIOIntDisable(BUTTONS_INT_GPIO_BASE, BUTTONS_INT_PIN);
    // Signal relevant semaphore
    G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
}

