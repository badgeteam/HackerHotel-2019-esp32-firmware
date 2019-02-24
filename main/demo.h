/**
 * @file demo.h
 *
 */

#ifndef DEMO_H
#define DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"


#define USE_LV_TESTS   0

/*******************
 * TUTORIAL USAGE    
 *******************/
#define USE_LV_TUTORIALS   0


/*********************
 * APPLICATION USAGE    
 *********************/

/* Test the graphical performance of your MCU
 * with different settings*/
#define USE_LV_BENCHMARK   0

/*A demo application with Keyboard, Text area, List and Chart
 * placed on Tab view */
#define USE_LV_DEMO        1
#if USE_LV_DEMO
#define LV_DEMO_WALLPAPER  0    /*Create a wallpaper too*/
#define LV_DEMO_SLIDE_SHOW 1	/*Automatically switch between tabs*/
#endif

/*MCU and memory usage monitoring*/
#define USE_LV_SYSMON      0

/*A terminal to display received characters*/
#define USE_LV_TERMINAL    0

/*Touch pad calibration with 4 points*/
#define USE_LV_TPCAL 0


#if USE_LV_DEMO

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a demo application
 */
void demo_create(void);

/**********************
 *      MACROS
 **********************/

#endif /*USE_LV_DEMO*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DEMO_H*/
