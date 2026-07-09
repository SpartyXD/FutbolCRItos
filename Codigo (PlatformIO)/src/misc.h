#pragma once

#include <Arduino.h>

//=====================================
//Bot

//Left motor
#define PWM_A_PIN 3 
#define A1_PIN 4  
#define A2_PIN 5  

//Right motor
#define PWM_B_PIN 0
#define B1_PIN 1
#define B2_PIN 2 

//Controller
#define VRX_PIN 0
#define VRY_PIN 1
#define SW_PIN 2

//=====================================
//MACROS / PARAMETERS / AUX FUNCTIONS
#define rep(i, n) for(int i=0; i<n; i++)
#define MAX_ARDUINO_TIME 3294967295

unsigned long get_time(){
    return millis();
}

#define X_AXIS 0
#define Y_AXIS 1
#define BOT_PROGRAM 0
#define CONTROLLER_PROGRAM 1

//=====================================