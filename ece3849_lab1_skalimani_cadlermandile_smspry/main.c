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
int16_t y[1024];

//
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

        if (fifo_get(&p)) {

            /* Joystick configuration for scaling   */

            switch (p) {
            case 'R':               // if right joystick selected
                fVoltsPerDiv = 0.1;
                break;
            case 'L':              // if left joystick is selected
                fVoltsPerDiv = 0.2;
                break;
            case 'U':              // if up joystick is selected
                fVoltsPerDiv = 0.5;
                break;
            case 'D':              // if down joystick is selected
                fVoltsPerDiv = 1.0;
                break;
            case '1':
                triggerSlope =! triggerSlope;
                break;
            }
        }

        fScale = (VIN_RANGE * PIXELS_PER_DIV)/((1 << ADC_BITS) * fVoltsPerDiv);

        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black


        //draw grid
        GrContextForegroundSet(&sContext, ClrBlue); // blue lines
        for(m=-16; m<LCD_HORIZONTAL_MAX; m+=PIXELS_PER_DIV){
            for(q=-16; q<LCD_VERTICAL_MAX; q+=PIXELS_PER_DIV){
                GrLineDraw(&sContext, m, q, m+PIXELS_PER_DIV, q);
                GrLineDraw(&sContext, m, q, m, q+PIXELS_PER_DIV);
            }
        }


        // store values from gADCBuffer to local variables and check for slope
        gTriggerIndex = gADCBufferIndex - LCD_HORIZONTAL_MAX/2;
        int cur_sample, last_sample;
        last_sample = gADCBuffer[ADC_BUFFER_WRAP(gTriggerIndex)];

        if (triggerSlope) {
            for (j = 0; j < ADC_BUFFER_SIZE/2; j++) {
                cur_sample = gADCBuffer[ADC_BUFFER_WRAP(gTriggerIndex--)];
                if (cur_sample <= ADC_OFFSET && last_sample >= ADC_OFFSET) {
                    // found falling trigger
                    break;
                }
                last_sample = cur_sample;
            }
        }
        else {

            for (j = 0; j < ADC_BUFFER_SIZE/2; j++) {
                cur_sample = gADCBuffer[ADC_BUFFER_WRAP(gTriggerIndex--)];
                if (cur_sample >= ADC_OFFSET && last_sample <= ADC_OFFSET) {
                    // found rising trigger
                    break;
                }
                last_sample = cur_sample;
            }

        }

        if (j >= ADC_BUFFER_SIZE/2) {
            gTriggerIndex = gADCBufferIndex - LCD_HORIZONTAL_MAX/2;
        }

        for (j = 0; j < LCD_HORIZONTAL_MAX; j++) {
            gWaveformBuffer[j] = gADCBuffer[ADC_BUFFER_WRAP(gTriggerIndex + j - LCD_HORIZONTAL_MAX/2)];
        }

        j = 0;

        // Print time scale
        snprintf(str, sizeof(str), "20 us");
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 3, /*opaque*/ false);

        // Print volts per division
        snprintf(str, sizeof(str), "%.1f V", fVoltsPerDiv);
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 50, /*y*/ 3, /*opaque*/ false);

        if(triggerSlope){ //falling edge
            GrContextForegroundSet(&sContext, ClrWhite);
            GrLineDraw(&sContext,115, 3, 120, 3);
            GrLineDraw(&sContext,115, 3, 115, 8);
            GrLineDraw(&sContext,110, 8, 115, 8);
        }
        else{ //rising edge
            GrContextForegroundSet(&sContext, ClrWhite);
            GrLineDraw(&sContext,110, 3, 115, 3);
            GrLineDraw(&sContext,115, 3, 115, 8);
            GrLineDraw(&sContext,115, 8, 120, 8);
        }

        //draw waveform
        GrContextForegroundSet(&sContext, ClrYellow);

        for(n = 0; n<LCD_HORIZONTAL_MAX;n++){
            sample = gWaveformBuffer[n];
            y[n] = LCD_VERTICAL_MAX/2 -(int)(fScale * ((int)sample - ADC_OFFSET));

            if (n != 0){
                GrLineDraw(&sContext, n-1, y[n-1], n, y[n]);
            }
        }

        count_loaded = cpu_load_count();
        cpu_load = 1.0f - (float)count_loaded/count_unloaded; // compute CPU load
        snprintf(str, sizeof(str), "CPU Load = %.2f %%", cpu_load*(100));
        GrContextForegroundSet(&sContext, ClrWhite);
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 118, /*opaque*/ false);

        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}

/*
 * CPU load count
 * Uses Timer3
 */
uint32_t cpu_load_count(void)
{
    uint32_t i = 0;
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
    while (!(TimerIntStatus(TIMER3_BASE, false) & TIMER_TIMA_TIMEOUT))
        i++;
    return i;
}




