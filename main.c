/**
 * main.c
 *
 * ECE 3849 Lab 0 Starter Project
 * Gene Bogdanov    10/18/2017
 *
 * This version is using the new hardware for B2017: the EK-TM4C1294XL LaunchPad with BOOSTXL-EDUMKII BoosterPack.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "buttons.h"

uint32_t gSystemClock; // [Hz] system clock frequency
volatile uint32_t gTime = 8345; // time in hundredths of a second

// temporary buffer storage variables
volatile int32_t temp0, temp1;
volatile int32_t gTriggerIndex;
volatile int i, j, k, l;

// data to display conversion variables
uint16_t sample;
volatile float fVoltsPerDiv = 1.0;
float fScale;
int y, y_old ;
char buttonRead;

//for interpreting gButtons and putting into FIFO
uint32_t buttons; //local copy of gButtons

//triggerslope flag
int triggerSlope = 0;

char buttonChar(uint32_t buttons);

int main(void)
{
    IntMasterDisable();

    // Enable the Floating Point Unit, and permit ISRs to use it
    FPUEnable();
    FPULazyStackingEnable();

    // Initialize the system clock to 120 MHz
    gSystemClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);

    Crystalfontz128x128_Init(); // Initialize the LCD display driver
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP); // set screen orientation

    tContext sContext;
    GrContextInit(&sContext, &g_sCrystalfontz128x128); // Initialize the grlib graphics context
    GrContextFontSet(&sContext, &g_sFontFixed6x8); // select font
    ButtonInit();
    IntMasterEnable();

    j = 0;
    i = 0;

    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};

//    GrContextForegroundSet(&sContext, ClrBlack);
//    GrRectFill(&sContext, &rectFullScreen); // fill screen with black

    while(1){

        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black
        GrContextForegroundSet(&sContext, ClrYellow); // yellow text

        buttons = gButtons;
        buttonRead = buttonChar(buttons);

        fifo_put(buttonRead);
//        //draw grid
//        k = 0;
//        l = 0;
//        for(k = 1; k+=20; k<128)

        /* Joystick configuration for scaling   */

        if (buttons&0x20)               // if right joystick selected
            fVoltsPerDiv = 0.1;
        if (buttons&0x40)               // if left joystick is selected
            fVoltsPerDiv = 0.2;
        if (buttons&0x80)               // if up joystick is selected
            fVoltsPerDiv = 0.5;
        if (buttons&0x100)              // if down joystick is selected
            fVoltsPerDiv = 1.0;

        fScale = (VIN_RANGE * PIXELS_PER_DIV)/((1 << ADC_BITS) * fVoltsPerDiv);

        //USR_SW1 switches trigger slope
        if(buttons&0x01){    //USR_SW1
            if(triggerSlope == 0)
                triggerSlope = 1;
            else
                triggerSlope = 0;
        }


        gTriggerIndex = gADCBufferIndex + 512;
        while(j<gTriggerIndex+1){
                temp0 = triggerBuffer[1];
                temp1 = triggerBuffer[2];
                triggerBuffer[0] = temp0;
                triggerBuffer[1] = temp1;
                triggerBuffer[2] = gADCBuffer[gTriggerIndex-j];
                j++;
        }

        j = 0;

/*      Trigger slope settings for Selina's board. DO NOT DESTROY!   */
/*
        if(triggerBuffer[0] > 445 && triggerBuffer[0] < ADC_OFFSET && triggerBuffer[1] > ADC_OFFSET && triggerBuffer[1] < 1935){
          for(i=0; i<1024; i++){
              gWaveformBuffer[i] = gADCBuffer[(gTriggerIndex - (512+i))];
              sample = gWaveformBuffer[i];
              y = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));
              if (i != 0){
                  GrLineDraw(&sContext,i-1, y_old, i, y); // print dots at the height of y
*/

        if(triggerSlope == 0){ //rising
            if(triggerBuffer[0] > 2044 && triggerBuffer[2] < 2045){
              for(i=0; i<1024; i++){
                  gWaveformBuffer[i] = gADCBuffer[(gTriggerIndex - (512+i))];
                  sample = gWaveformBuffer[i];
                  y = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));
                  if (i != 0){

                      GrLineDraw(&sContext,i-1, y_old, i, y); // print dots at the height of y
                  }
                  y_old = y;
               }
              gTriggerIndex = gADCBufferIndex - 512;
            }
        }
        else{ //falling
            if(triggerBuffer[0]< 2044 && triggerBuffer[2] > 2045){
              for(i=0; i<1024; i++){
                  gWaveformBuffer[i] = gADCBuffer[(gTriggerIndex - (512+i))];
                  sample = gWaveformBuffer[i];
                  y = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));
                  if (i != 0){

                      GrLineDraw(&sContext,i-1, y_old, i, y); // print dots at the height of y
                  }
                  y_old = y;
              }
              gTriggerIndex = gADCBufferIndex - 512;
            }
        }

        GrFlush(&sContext); // flush the frame buffer to the LCD


    }


//    uint32_t time;  // local copy of gTime
//    uint32_t buttons; //local copy of gButtons
//    uint32_t hour, min, sec;
//    char str[50], str1[20], str2[20], str3[20], str4[20], str5[20], str6[20], str7[20];   // string buffer
//    full-screen rectangle



//    while (true) {
//        GrContextForegroundSet(&sContext, ClrBlack);
//        GrRectFill(&sContext, &rectFullScreen); // fill screen with black
//        time = gTime; // read shared global only once
//        buttons = gButtons; // read shared global only once
//        sec = (time / 100) % 60 ;
//        min = time /6000;
//        hour = min /60;
//        snprintf(str, sizeof(str), "Time = %02u:%02u:%02u", hour, min, sec); // convert time to string//
//        snprintf(str1, sizeof(str1), "B1:%01u", (buttons & 0x04)); // print button1 state
//        snprintf(str2, sizeof(str2), "B2:%01u", (buttons & 0x08)); // print button2 state
//        snprintf(str3, sizeof(str3), "JSelect:%01u", (buttons & 0x10)); // print button2 state
//        snprintf(str4, sizeof(str4), "JRight:%01u", (buttons & 0x20)); // print button2 state
//        snprintf(str5, sizeof(str5), "JLeft:%01u", (buttons & 0x40)); // print button2 state
//        snprintf(str6, sizeof(str6), "JUp:%01u", (buttons & 0x80)); // print button2 state
//        snprintf(str7, sizeof(str7), "JDown:%01u", (buttons & 0x100)); // print button2 state
//
//        GrContextForegroundSet(&sContext, ClrYellow); // yellow text
//        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 0, /*opaque*/ false);
//
//        GrStringDraw(&sContext, str1, /*length*/ -1, /*x*/ 0, /*y*/ 10, /*opaque*/ false);
//        GrStringDraw(&sContext, str2, /*length*/ -1, /*x*/ 0, /*y*/ 20, /*opaque*/ false);
//
//        GrStringDraw(&sContext, str3, /*length*/ -1, /*x*/ 0, /*y*/ 30, /*opaque*/ false);
//        GrStringDraw(&sContext, str4, /*length*/ -1, /*x*/ 0, /*y*/ 40, /*opaque*/ false);
//        GrStringDraw(&sContext, str5, /*length*/ -1, /*x*/ 0, /*y*/ 50, /*opaque*/ false);
//        GrStringDraw(&sContext, str6, /*length*/ -1, /*x*/ 0, /*y*/ 60, /*opaque*/ false);
//        GrStringDraw(&sContext, str7, /*length*/ -1, /*x*/ 0, /*y*/ 70, /*opaque*/ false);
//
//    }
}


char buttonChar(uint32_t buttons){
    if(buttons&0x01)    //USR_SW1
        return '1';
    if(buttons&0x02)    //USR_SW2
        return '2';
    if(buttons&0x04)    //S1
        return 'A';
    if(buttons&0x08)    //S2
        return 'B';
    if(buttons&0x10)    //Select
        return 'S';
    if(buttons&0x20)    //Right
        return 'R';
    if(buttons&0x40)    //Left
        return 'L';
    if(buttons&0x80)    //Up
        return 'U';
    if(buttons&0x100)   //Down
        return 'D';
    else
        return '0';
}





