/*
 * BrickBreaker_Structures.h
 *
 *  Created on: Nov 15, 2023
 *      Author: bengs
 */

#ifndef BRICKBREAKER_STRUCTURES_H_
#define BRICKBREAKER_STRUCTURES_H_

/************************************Includes***************************************/

#include <stdbool.h>
#include <stdint.h>

/************************************Includes***************************************/

/*************************************Defines***************************************/


/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

// Thread Control Block
typedef struct blocks_t {
    uint16_t x;
    uint16_t y;
    bool isAlive;
} blocks_t;

typedef struct ball_t {
    int16_t x;
    int16_t y;
    int16_t speedX;
    int16_t speedY;
} ball_t;

typedef struct platform_t {
    uint16_t x;
    uint16_t old_x;
} platform_t;

/****************************Data Structure Definitions*****************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/

/********************************Public Functions***********************************/
/********************************Public Functions***********************************/

/*******************************Private Variables***********************************/
/*******************************Private Variables***********************************/

/*******************************Private Functions***********************************/
/*******************************Private Functions***********************************/

#endif /* BRICKBREAKER_STRUCTURES_H_ */
