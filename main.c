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

    uint32_t time;  // local copy of gTime
    uint32_t buttons; //local copy of gButtons
    uint32_t hour, min, sec;
    char str[50], str1[20], str2[20], str3[20], str4[20], str5[20], str6[20], str7[20];   // string buffer
    // full-screen rectangle
    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};

    while (true) {
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black
        time = gTime; // read shared global only once
        buttons = gButtons; // read shared global only once
        sec = (time / 100) % 60 ;
        min = time /6000;
        hour = min /60;
        snprintf(str, sizeof(str), "Time = %02u:%02u:%02u", hour, min, sec); // convert time to string//
        snprintf(str1, sizeof(str1), "B1:%01u", (buttons & 0x04)); // print button1 state
        snprintf(str2, sizeof(str2), "B2:%01u", (buttons & 0x08)); // print button2 state

//        snprintf(str3, sizeof(str3), "B2:%32u", (buttons)); // print button2 state

        snprintf(str3, sizeof(str3), "JSelect:%01u", (buttons & 0x10)); // print button2 state
        snprintf(str4, sizeof(str4), "JRight:%01u", (buttons & 0x20)); // print button2 state
        snprintf(str5, sizeof(str5), "JLeft:%01u", (buttons & 0x40)); // print button2 state
        snprintf(str6, sizeof(str6), "JUp:%01u", (buttons & 0x80)); // print button2 state
        snprintf(str7, sizeof(str7), "JDown:%01u", (buttons & 0x100)); // print button2 state

        GrContextForegroundSet(&sContext, ClrYellow); // yellow text
        GrStringDraw(&sContext, str, /*length*/ -1, /*x*/ 0, /*y*/ 0, /*opaque*/ false);

        GrStringDraw(&sContext, str1, /*length*/ -1, /*x*/ 0, /*y*/ 10, /*opaque*/ false);
        GrStringDraw(&sContext, str2, /*length*/ -1, /*x*/ 0, /*y*/ 20, /*opaque*/ false);

        GrStringDraw(&sContext, str3, /*length*/ -1, /*x*/ 0, /*y*/ 30, /*opaque*/ false);
        GrStringDraw(&sContext, str4, /*length*/ -1, /*x*/ 0, /*y*/ 40, /*opaque*/ false);
        GrStringDraw(&sContext, str5, /*length*/ -1, /*x*/ 0, /*y*/ 50, /*opaque*/ false);
        GrStringDraw(&sContext, str6, /*length*/ -1, /*x*/ 0, /*y*/ 60, /*opaque*/ false);
        GrStringDraw(&sContext, str7, /*length*/ -1, /*x*/ 0, /*y*/ 70, /*opaque*/ false);

        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}
