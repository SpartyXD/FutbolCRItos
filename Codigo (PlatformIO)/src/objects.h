#pragma once

#include <misc.h>

struct Joystick{
    //Pins
    int vrx_pin, vry_pin;
    int sw_pin, sw_state;
    float sensitivity = 1.0;

    //Joysticks
    int deadzone = 2048;
    int joystick_threshold = 300;
    int max_reading = 4095;

    //Debounce
    unsigned long debounce_delay = 50;
    unsigned long last_sw_check = 0;
    unsigned long time_now = 0;
    bool last_sw_state = HIGH;

    int _get_power(int measurement){
        int relative_pos = (measurement-deadzone);

        if(abs(relative_pos) < joystick_threshold)
            return 0;

        int power = map(abs(relative_pos), joystick_threshold, (max_reading-deadzone), 0, 100);
        power *= sensitivity;

        return (relative_pos<0) ? -power : power;
    }

    Joystick(){}

    void init(int vrx_pin, int vry_pin, int sw_pin, float sensitivity=1, int deadzone=2048){
        pinMode(vrx_pin, INPUT);
        pinMode(vry_pin, INPUT);
        pinMode(sw_pin, INPUT_PULLUP);

        this->vrx_pin = vrx_pin;
        this->vry_pin = vry_pin;
        this->sw_pin = sw_pin;
    }

    bool is_pressed(){
        time_now = get_time();

        if(time_now-last_sw_check <= debounce_delay)
            return false;

        last_sw_check = time_now;
        sw_state = digitalRead(sw_pin);

        if(sw_state == LOW && last_sw_state == HIGH){
            last_sw_state = LOW;
            return true;
        }

        last_sw_state = sw_state;
        return false;
    }

    int get_axis_power(int axis){
        if(axis == X_AXIS)
            return _get_power(analogRead(vrx_pin));
        else if(axis == Y_AXIS)
            return _get_power(analogRead(vry_pin));
        else
            return 0;
    }

};

struct MotorShield{
    int left_pwm_pin, left_a_pin, left_b_pin;
    int right_pwm_pin, right_a_pin, right_b_pin;
    int MAX_SPEED = 255;

    MotorShield(){}

    void init(int pwm_A, int a_1, int a_2, int pwm_B, int b_1, int b_2, int max_speed=255){
        left_pwm_pin = pwm_A;
        left_a_pin = a_1;
        left_b_pin = a_2;
        
        right_pwm_pin = pwm_B;
        right_a_pin = b_1;
        right_b_pin = b_2;
        
        MAX_SPEED = max_speed;
        
        pinMode(left_pwm_pin, OUTPUT); pinMode(left_a_pin, OUTPUT); pinMode(left_b_pin, OUTPUT);
        pinMode(right_pwm_pin, OUTPUT); pinMode(right_a_pin, OUTPUT); pinMode(right_b_pin, OUTPUT);
        stopMotors();
    }

    void stopMotors(){
        controlMotors(0, 0);
    }

    void setMotorSpeed(int motor, int speed){
        bool reverse = speed<0;
        speed = constrain(abs(speed), 0, MAX_SPEED);

        if(motor == 0){
            //Left
            analogWrite(left_pwm_pin, speed);
            digitalWrite(left_a_pin, !reverse);
            digitalWrite(left_b_pin, reverse);
        }
        else{
            //Right
            analogWrite(right_pwm_pin, speed);
            digitalWrite(right_a_pin, !reverse);
            digitalWrite(right_b_pin, reverse);
        }
    }

    void controlMotors(int left_speed, int right_speed){
        setMotorSpeed(0, left_speed);
        setMotorSpeed(1, right_speed);
    }

};



