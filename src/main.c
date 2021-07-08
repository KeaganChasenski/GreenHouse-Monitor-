#include "main.h"

void initGPIO()
{
    wiringPiSetup();

    pinMode(AD, INPUT);
    pullUpDnControl(AD, PUD_UP);
    wiringPiISR(AD, INT_EDGE_FALLING, &AlarmDismissal);
    pinMode(TR, INPUT);
    pullUpDnControl(TR, PUD_UP);
    wiringPiISR(TR, INT_EDGE_FALLING, &TimeReset);
    pinMode(SFT, INPUT);
    pullUpDnControl(SFT, PUD_UP);
    wiringPiISR(SFT, INT_EDGE_FALLING, &SampleFrequencyToggle);
    pinMode(PB, INPUT);
    pullUpDnControl(PB, PUD_UP);
    wiringPiISR (PB, INT_EDGE_FALLING, &Pause);

}

void initRTC()
{
    wiringPiI2CSetup(RTCAddr);
    wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(getHours()));
	wiringPiI2CWriteReg8(RTC, MIN, decCompensation(getMins()));
	wiringPiI2CWriteReg8(RTC, SEC, decCompensation(getSecs())|0b10000000);
}

void initSPI()
{
    DAC = wiringPiSPISetup(0, 500000);
    wiringPiSPISetup(1, 500000);
    //ADC = wiringPiSPISetup(1, 10000);
    mcp3004Setup(BASE, SPI_CHAN);
}

void initAlarm()
{
    char ch;
    FILE *filePointer;
    printf("%s\n", FILENAME);
    filePointer = fopen(FILENAME, "r"); // read mode

    if (filePointer == NULL)
    {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }
    //load sound file contents into an array
    int i = 0;
    while(ch != EOF)
    {
        ch = fgetc(filePointer);
        soundFile[i] = ch;
        i++;
    }

    fclose(filePointer);
}

int main()
{
    //calling all initialization functions and threads
    getCurrentTime();
    initGPIO();
    initRTC();
    initSPI();
    //FILE *filePointer;
    //initAlarm();
    //initializing used variables
    sec = 0;
    prevSecs = 0;
    SF = 0;
    voltage = 0;
    //readings[0] = 0;
    //readings[1] = 0;
    //readings[3] = 0;
    //creating threads
    pthread_t readRTC, readADC;
    pthread_create(&readRTC, NULL, fetchTime, NULL);
    pthread_create(&readADC, NULL, fetchADC, NULL);
    //phtread_join(readRTC, readADC);
    //pthread_join(readADC, NULL);

    while(true)
    {
        if(sec>prevSecs)
        {
            if(resume == true)
            {
                printf("Active time: %d seconds\n", sec);
            }
        }

        if(sec%SF == 0)
        {
            //lightIntensity = readings[0];
            //humidity = readings[1];
            //temperature = readings[2];
            printf("Light Intensity: %d | Humidity: %d | Temperature: %d \n",lightIntensity,humidity,temperature);
            voltage = (1/1023)*lightIntensity*humidity;
            if(voltage > 2.65 || voltage < 0.65)
            {
                if((sec - TSLA) > 180 || firstChance == true)
                {
                    alarmCheck = true;
                    while(alarmCheck == true)
                    {
                        playAlarm();
                        delay(6000);
                    }

                }
            }
        }
    }

    return 0;
}


void *fetchTime(void *arg)
{
    //int *fetchingTime = (int*)arg;
    prevSecs = hexCompensation(wiringPiI2CReadReg8(RTC, SEC));
    int secs;
    int minu;
    int hours;
    while(true)
    {
        secs = hexCompensation(wiringPiI2CReadReg8(RTC, SEC));
        minu = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
        hours = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));

        if(secs > prevSecs)
        {
            prevSecs = secs;
            sec ++;
            printf("%d:%d:%d\n", hours, minu, secs);
        }
    }

    pthread_exit(NULL);
}

void *fetchADC(void *arg)
{
    //int buffer[] = (int*)arg;
    //unsigned char* temp = (unsigned char*)(0b10000000);
    //unsigned char* light = (unsigned char*)(0b10010000);
    //unsigned char* humi = (unsigned char*)(0b10100000);


    while(true)
    {
        //read from ADC
        temperature = analogRead(BASE);//wiringPiSPIDataRW (ADC_channel, temp, 1);
        lightIntensity = analogRead(BASE + 1);//wiringPiSPIDataRW (ADC_channel, light, 1);
        humidity = analogRead(BASE +2); //wiringPiSPIDataRW (ADC_channel, humi, 1);
        //update appropriate values
        //temperature = (int)temp;
        //lightIntensity = (int)light;
        //humidity = (int)humi;

    }

}

void playAlarm()
{
      TSLA = sec;
      firstChance = false;
      printf("*ALARM*\n");
      //unsigned char* soundbyte;
      //play audio through DAC
      //for(int i = 0; i < 172284; i++)
      //{
       //  soundbyte = (unsigned char*)(soundFile + i);
        // wiringPiSPIDataRW(DAC_channel,soundbyte,2);
     // }
}

void AlarmDismissal()
{
    long interruptTime = millis();
    if (interruptTime - lastInterruptTime>500)
    {
        alarmCheck = false;
    }
    lastInterruptTime = interruptTime;
}

void TimeReset()
{
    long interruptTime = millis();
    if (interruptTime - lastInterruptTime>500)
    {
        sec = 0;
        TSLA = 0;
        firstChance = true;
    }
    lastInterruptTime = interruptTime;
}

void SampleFrequencyToggle()
{
    long interruptTime = millis();
    if (interruptTime - lastInterruptTime>500)
    {
        if(SF == 1)
        {
            SF = 2;
        }
        else if(SF == 2)
        {
            SF = 5;
        }
        else if(SF == 5)
        {
            SF = 1;
        }
    }
    lastInterruptTime = interruptTime;
}

void Pause()
{
    long interruptTime = millis();
    if (interruptTime - lastInterruptTime>500)
    {
        if(resume == true)
        {
            resume = false;
        }
        else if(resume == false)
        {
            resume = true;
        }
    }
    lastInterruptTime = interruptTime;
}

void getCurrentTime(void)
{
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  HH = timeinfo ->tm_hour;
  MM = timeinfo ->tm_min;
  SS = timeinfo ->tm_sec;
}

int getHours(void)
{
    getCurrentTime();
    return HH;
}

int getMins(void)
{
    return MM;
}

int getSecs(void)
{
    return SS;
}

int decCompensation(int units)
{
	int unitsU = units%10;

	if (units >= 50)
	{
		units = 0x50 + unitsU;
	}
	else if (units >= 40)
	{
		units = 0x40 + unitsU;
	}
	else if (units >= 30)
	{
		units = 0x30 + unitsU;
	}
	else if (units >= 20)
	{
		units = 0x20 + unitsU;
	}
	else if (units >= 10)
	{
		units = 0x10 + unitsU;
	}
	return units;
}

int hexCompensation(int units)
{
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic)
	*/
	int unitsU = units%0x10;

	if (units >= 0x50)
	{
		units = 50 + unitsU;
	}
	else if (units >= 0x40)
	{
		units = 40 + unitsU;
	}
	else if (units >= 0x30)
	{
		units = 30 + unitsU;
	}
	else if (units >= 0x20)
	{
		units = 20 + unitsU;
	}
	else if (units >= 0x10)
	{
		units = 10 + unitsU;
	}
	return units;
}

