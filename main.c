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
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"

uint32_t gSystemClock; // [Hz] system clock frequency
volatile uint32_t gTime = 8345; // time in hundredths of a second

// temporary buffer storage variables
volatile int32_t temp0, temp1;
volatile int32_t gTriggerIndex;
volatile int i, j, k, l, n, m, q;

// data to display conversion variables
uint16_t sample;
volatile float fVoltsPerDiv = 1.0;
float fScale;
int y[1024];
int y_old[1024];

char buttonRead;
char getTest;

char p;
char str[50];

uint32_t count_unloaded = 0;
uint32_t count_loaded = 0;
float cpu_load = 0.0;


//for interpreting gButtons and putting into FIFO
uint32_t buttons; //local copy of gButtons

//triggerslope flag
int triggerSlope =  0;

void buttonChar(uint32_t buttons);
uint32_t cpu_load_count(void);

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

    count_unloaded = cpu_load_count();

    IntMasterEnable();

    j = 0;
    i = 0;

    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &rectFullScreen); // fill screen with black

    while(1){

        fifo_get(&p);

        /* Joystick configuration for scaling   */

        if (p == 'R')               // if right joystick selected
            fVoltsPerDiv = 0.1;
        if (p == 'L')               // if left joystick is selected
            fVoltsPerDiv = 0.2;
        if (p == 'U')               // if up joystick is selected
            fVoltsPerDiv = 0.5;
        if (p == 'D')              // if down joystick is selected
            fVoltsPerDiv = 1.0;

        fScale = (VIN_RANGE * PIXELS_PER_DIV)/((1 << ADC_BITS) * fVoltsPerDiv);

        buttons = gButtons;

        //USR_SW1 switches trigger slope
        if(buttons & 1){    //USR_SW1
            triggerSlope =! triggerSlope;
        }
        else{
            triggerSlope = triggerSlope;
        }

        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black


        //draw grid
        for(m=-20; m<128; m+=20){
            for(q=-20; q<128; q+=20){
                GrLineDraw(&sContext, m, q, m+20, q);
                GrLineDraw(&sContext, m, q, m, q+20);
                GrContextForegroundSet(&sContext, ClrBlue); // blue text
            }
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

        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black

        snprintf(str, sizeof(str), "20 us");
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 3, /*opaque*/ false);

        snprintf(str, sizeof(str), "%.1f mV", fVoltsPerDiv);
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 50, /*y*/ 3, /*opaque*/ false);



        if(triggerSlope == 0){ //falling

            GrContextForegroundSet(&sContext, ClrWhite);
            GrLineDraw(&sContext,115, 3, 120, 3);
            GrLineDraw(&sContext,115, 3, 115, 8);
            GrLineDraw(&sContext,110, 8, 115, 8);

            if(triggerBuffer[0] < (ADC_OFFSET - 1) && triggerBuffer[2] > ADC_OFFSET){
              for(i=0; i<1024; i++){
                  gWaveformBuffer[i] = gADCBuffer[(gTriggerIndex - (512+i))];
                  sample = gWaveformBuffer[i];
                  y[i] = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));
               }
              gTriggerIndex = gADCBufferIndex - 512;
            }
        }
        else{ //rising

            GrContextForegroundSet(&sContext, ClrWhite);
            GrLineDraw(&sContext,110, 3, 115, 3);
            GrLineDraw(&sContext,115, 3, 115, 8);
            GrLineDraw(&sContext,115, 8, 120, 8);

            if(triggerBuffer[0] > (ADC_OFFSET) && triggerBuffer[2] < (ADC_OFFSET-1)){
              for(i=0; i<1024; i++){
                  gWaveformBuffer[i] = gADCBuffer[(gTriggerIndex - (512+i))];
                  sample = gWaveformBuffer[i];
                  y[i] = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));
              }
              gTriggerIndex = gADCBufferIndex - 512;
            }
        }

        for(n = 0; n<128;n++){
              if (n != 0){

                  GrLineDraw(&sContext,n-1, y_old[n], n, y[n]); // print dots at the height of y
                  GrContextForegroundSet(&sContext, ClrYellow); // yellow text
              }
              y_old[n] = y[n];
        }

        count_loaded = cpu_load_count();
        cpu_load = 1.0f - (float)count_loaded/count_unloaded; // compute CPU load
        snprintf(str, sizeof(str), "CPU Load = %.2f %%", cpu_load*(100));
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 118, /*opaque*/ false);

        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}

uint32_t cpu_load_count(void)
{
    uint32_t i = 0;
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
    while (!(TimerIntStatus(TIMER3_BASE, false) & TIMER_TIMA_TIMEOUT))
        i++;
    return i;
}




