/* The Arduino Nano program of PinWheels@LAB.
 *
 * File: main.cpp
 * Author: Guillaume RIVIERE
 * Date: February 2021, May 2023, August 2026
 */ 

#include <Arduino.h>
#include <pt.h>
#include <digitalWriteFast.h>

//== Arduino pinout

#define STEP_OUTPIN_M1 A0
#define STEP_OUTPIN_M2  5
#define STEP_OUTPIN_M3  3
#define STEP_OUTPIN_M4 A2

#define DIR_OUTPIN_M1  13
#define DIR_OUTPIN_M2   4
#define DIR_OUTPIN_M3   2
#define DIR_OUTPIN_M4  A1

#define ENBL_OUTPIN_M1 12
#define ENBL_OUTPIN_M2 11
#define ENBL_OUTPIN_M3 10
#define ENBL_OUTPIN_M4  9

#define MS1_OUTPIN 8
#define MS2_OUTPIN 7
#define MS3_OUTPIN 6

//== Parameters

#define DELAY_MIN_US     6000
#define DELAY_MAX_US       20
#define DELAY_DEFAULT_US 5000

//== Data structures

struct pt_param {
  struct pt proto_thread;
  uint8_t motor_index;
};

struct motor {
  uint8_t step_outpin;
  uint8_t dir_outpin;
  uint8_t enbl_outpin;
  uint16_t period_us;
  bool enabled;
  struct pt_param thread;
};

//== Macro-functions

#define PT_PARAM_INIT(_pt_param, _motor_index)           \
  PT_INIT(&_pt_param.proto_thread) ;                     \
  _pt_param.motor_index = _motor_index ;

//== Variables

struct motor motors[4];
uint8_t is_reading_frame;

//== Functions

static int proto_thread_execution(struct pt_param *thread)
{
  static uint32_t lastTimeMove[4];
  
  PT_BEGIN(&thread->proto_thread);

  //-- Impulse a motor step
  
  while (1) {
    
    lastTimeMove[thread->motor_index] = micros();
    PT_WAIT_UNTIL(&thread->proto_thread, micros() - lastTimeMove[thread->motor_index] > motors[thread->motor_index].period_us);
    digitalWriteFast(motors[thread->motor_index].step_outpin, HIGH);
    
    lastTimeMove[thread->motor_index] = micros();
    PT_WAIT_UNTIL(&thread->proto_thread, micros() - lastTimeMove[thread->motor_index] > motors[thread->motor_index].period_us);
    digitalWriteFast(motors[thread->motor_index].step_outpin, LOW);
  }
  
  PT_END(&thread->proto_thread);
}

uint16_t speed_to_period_us(double factor)
{
  return DELAY_MAX_US + (DELAY_MIN_US - DELAY_MAX_US) * (1.0 - factor) ;
}

int16_t read_digit()
{
  int16_t res;
  
  while (!(isdigit(res = Serial.read()) || res == '-'));
  
  return res;
}

void setup ()
{
  is_reading_frame = 0;
  
  Serial.begin(9600);

  delay(5000);
  
  //-- Set motors' pinout
  
  motors[0].step_outpin = STEP_OUTPIN_M1;
  motors[0].dir_outpin = DIR_OUTPIN_M1;
  motors[0].enbl_outpin = ENBL_OUTPIN_M1;

  motors[1].step_outpin = STEP_OUTPIN_M2;
  motors[1].dir_outpin = DIR_OUTPIN_M2;
  motors[1].enbl_outpin = ENBL_OUTPIN_M2;

  motors[2].step_outpin = STEP_OUTPIN_M3;
  motors[2].dir_outpin = DIR_OUTPIN_M3;
  motors[2].enbl_outpin = ENBL_OUTPIN_M3;

  motors[3].step_outpin = STEP_OUTPIN_M4;
  motors[3].dir_outpin = DIR_OUTPIN_M4;
  motors[3].enbl_outpin = ENBL_OUTPIN_M4;
  
  //-- Init micro-stepping pins ( 1/16 stepping => [ HIGH, HIGH, HIGH ])

  pinMode(MS1_OUTPIN, OUTPUT) ;
  pinMode(MS2_OUTPIN, OUTPUT) ;
  pinMode(MS3_OUTPIN, OUTPUT) ;
  
  digitalWrite(MS1_OUTPIN, HIGH) ;
  digitalWrite(MS2_OUTPIN, HIGH) ;
  digitalWrite(MS3_OUTPIN, HIGH) ;
  
  //-- Init values

  for (uint8_t i = 0 ; i < 4 ; i++) {

    //-- Init proto-thread
    
    PT_PARAM_INIT(motors[i].thread, i);
    
    //-- Init step and dir pins
    
    pinMode(motors[i].step_outpin, OUTPUT) ;
    pinMode(motors[i].dir_outpin, OUTPUT) ;
  
    //-- Disable power supply
    
    bool is_running = false;
    motors[i].enabled = is_running ;
    pinMode(motors[i].enbl_outpin, OUTPUT) ;
    digitalWrite(motors[i].enbl_outpin, is_running ? LOW : HIGH) ;
    
    //-- Set motor speed
    
    motors[i].period_us = DELAY_DEFAULT_US ;
    
    //-- Set motor direction
    
    digitalWrite(motors[i].dir_outpin, HIGH) ;
  }

  Serial.println("Setup done. Waiting.");
}

void loop ()
{  
  //-- Rotate motors

  for (uint8_t i = 0 ; i < 4 ; i++) {
    if (motors[i].enabled) {
      proto_thread_execution(&motors[i].thread);
    }
  }
  
  //-- Read from serial
  
  if (Serial.available() > 0) {
    static uint8_t cur_motor, speed_0, speed_1, speed_2, speed_3, dir ; 
    static double speed ;
    static int16_t b ;

    //** Frame reading is splitted into 8 steps that are distributed over successive loop iterations
    //** (in order to interleave frame reading with threads execution and preserve motors' rotation)
    
    if (is_reading_frame == 0) {
      while (Serial.available() > 0 && is_reading_frame == 0) {

        //** STEP 1: Seek a motor value
        
        b = Serial.read() - '0' ;
        
        if (b >= 0 && b <= 4) {          
          cur_motor = b;
          
          //-- Start frame reading
          is_reading_frame++;
        }
      }
    }
    else if (is_reading_frame == 1) {

      //** STEP 2: Read direction and 1st speed digit
      
      b = read_digit() ;
      
      if (b == '-') {
        dir = LOW ; // reverse
        b = read_digit() ;
      }
      else {
        dir = HIGH ; // normal
      }
      
      speed_0 = b - '0' ;
      
      is_reading_frame++;
    }
    else if (is_reading_frame == 2) {

      //** STEP 3:  Read direction 2nd speed digit

      speed_1 = read_digit() - '0' ;
      is_reading_frame++;
    }
    else if (is_reading_frame == 3) {

      //** STEP 4:  Read direction 3rd speed digit
      
      speed_2 = read_digit() - '0' ;
      is_reading_frame++;
    }
    else if (is_reading_frame == 4) {

      //** STEP 5:  Read direction 4th speed digit

      speed_3 = read_digit() - '0' ;
      is_reading_frame++;
    }
    else if (is_reading_frame == 5) {
      
      //** STEP 6:  Calculate speed
      
      speed = speed_0 + speed_1 * 0.1 + speed_2 * 0.01 + speed_3 * 0.001 ;

      if (speed > 1.0) {
        speed = 1.0 ;
      }
      
      if (speed < 0.0) {
        speed = 0.0 ;
      }

      is_reading_frame++;
    }
    else if (is_reading_frame == 6) {

      //** STEP 7:  Update motors' parameters
      
      for (uint8_t motor_number = 1 ; motor_number <= 4 ; motor_number++) {
        if (motor_number == cur_motor || cur_motor == 0) {
          struct motor *motor = &motors[motor_number - 1];
          
          //-- Enable or disable motor
          motor->enabled = (speed > 0.0) ;
          digitalWriteFast(motor->enbl_outpin, motor->enabled ? LOW : HIGH) ;
          
          //-- Motor speed
          motor->period_us = speed_to_period_us (speed) ;
          
          //-- Motor direction
          digitalWriteFast(motor->dir_outpin, dir) ;
        }
      }
      
      is_reading_frame++;
    }
    else if (is_reading_frame == 7) {

      //** STEP 8: Seek frame end
      
      while (Serial.read() != '|') ;

      //-- End frame reading
      is_reading_frame = 0 ;
    }
  }

}
