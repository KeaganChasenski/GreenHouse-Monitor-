#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
//#include <iostream>
#include <stdbool.h>
#include <wiringPiI2C.h>
#include <time.h>
#include <mcp3004.h>
//defined variables for SPI communication
#define DAC_channel 0
#define ADC_channel 1
#define SPI_SPEED 256000
#define FILENAME "src/why-are.mp3"
#define BASE 100
#define SPI_CHAN 0
//Addresses for RTC
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

//Global Varibles;
int HH,MM,SS; // hours, minutes and seconds to give the correct time for the RTC, this was found in PRAC 3
int sec, prevSecs;// sec keeps track of the active time of the system, preSecs is used for comparaitive purposes
bool resume = true;// this is used to pause or resume the displaying of the active system time
int DAC, ADC;
int SF = 1;// sampling frequency; acceptable values are 1,2,5 seconds
float voltage;// this is used to calculate the voltage readings of the ADC and based on the value it will sound an alarm
int readings[3];// this array contains the values of the temperature, light intensity and humidity
int lightIntensity;// this is used in the voltage calculation
int humidity;// this is used in the voltage calculation
int temperature;// this is used in the voltage calculation
bool alarmCheck = false;// this is used to check if the alarm must sound or not
int TSLA = 0; //time since last alarm
bool firstChance = true;
unsigned char soundFile[172284];//this array holds the contents of the sound file
long lastInterruptTime = 0; //used for button debounce
//Initializing methods
void initGPIO(void);//initializes the buttons
void initRTC(void);//initializes I2C for the RTC
void initSPI(void);//initializes SPI for the DAC & ADC
void initAlarm(void);//loads a sound file into an array to write into the DAC for the alarm
void playAlarm(void);// loads the contents of the array and plays it through the DAC
void AlarmDismissal(void);// when the alarm is on, this will cause the alarm to go off
void TimeReset(void);// this will reset the sec variable to zero, thereby resettiing active system time
void SampleFrequencyToggle(void);//this changes SF and changes when the ADC values are displayed
void Pause(void);//this stops the active system time from being displayed and counted
void getCurrentTime(void);//gets the current time to load into the RTC
int getHours(void);
int getMins(void);
int getSecs(void);
int hexCompensation(int units);//converts hex values into dec values
int decCompensation(int units);//converts dec values into hex values
void *fetchTime(void *arg);
void *fetchADC(void *arg);
//void minInc(void);
//GPIO pin labels
const int AD = 15; //Alarm Dismissal
const int TR = 16; //Time Reset
const int SFT = 1; //Sample Frequency Toggle
const int PB = 4; //pause button
//Component labels
const int RTC = 0;
