/*                 DCO Nano 
 *                 Copyright (c) 2020 Vernon Billingsley  
 *                     
 *                     
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission
  * notice shall be included in all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  * THE SOFTWARE.   
  * 
*/

#include  "TimerOne.h"
#include <elapsedMillis.h>
#include <Wire.h> 


#define adc1  A0
#define adc2  A1
#define adc3  A2
#define adc4  A3
#define adc5  A4
#define adc6  A5   
#define osc   9
#define d_c   2



const float dco_freq[] PROGMEM = {
32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,
65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,
130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,
261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,
523.25,554.37,587.33,622.25,659.26,698.46,739.99,783.99,830.61,880.00,932.33,987.77,
1046.50,1108.73,1174.66,1244.51,1318.51,1396.91,1479.98,1567.98,1661.22,1760.00,1864.66,1975.53,
2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.31,3951.07,
4186.01,  
};

const float dac_val[] PROGMEM = {
  70,75,81,87,93,99,110,117,123,130,138,146,
  156,167,179,192,206,221,237,254,272,246,256,295,
  315,335,356,377,399,421,444,467,491,515,540,570,
  597,626,657,690,725,762,801,842,885,930,977,1028,
  1083,1192,1301,1410,1519,1628,1737,1846,1955,2064,2173,2283,
  2403,2528,
};

//-------------------------- Variables ----------------------------------------------
//statup freq C4
float hertz = 261.63;
//cycle time
int long cycleTime = 3816;    //startup cycle time
int long cycleOnTime = 0;
int long cycleOffTime = 0;
float dutyCycle = 0.50;

boolean cycleOn = true;       //used to set wave high/low
boolean write_dac = false;

//modulation
int modAmount = 0;            //place holder for modulation amount
int modCycle = 10000;         //the period of modulation
byte modOn;                   //turn modulation on or off

int sweepCount;
boolean sweepUp = true;       //used to set sweep up/down
int sweepCycle;


//Store the ADC values
int adc1_val;
int adc2_val;
int adc3_val;

int old_adc1_val;
int old_adc2_val;
int old_adc3_val;


//--------------------------- Functions ---------------------------------------------
//Set output pin High or Low acording to the timer................................
void setCycleTime()
{
  if(cycleOn == true)
  {
    Timer1.setPeriod(cycleOffTime);
    digitalWrite(osc, HIGH);
    cycleOn = false;
  }else{
    Timer1.setPeriod(cycleOnTime);
    digitalWrite(osc, LOW);
    cycleOn = true;
  }

}
void calculateCycleTime()
{
  cycleTime = 1000000/ (hertz  + modAmount) ;
  cycleOnTime = cycleTime * dutyCycle;
  cycleOffTime = cycleTime - cycleOnTime;
}


//***********************************************************************
void setup() {
pinMode(osc, OUTPUT);
pinMode(d_c, INPUT_PULLUP);
Serial.begin(115200);

//--------------- Startup wire
Wire.begin();
Wire.setClock(400000);

//analogReference(EXTERNAL);

//----------------- Setup and interrupt timer -------------------------
calculateCycleTime();
Timer1.initialize(cycleOnTime);    //init time to 100 uS
//On timer update the Clock timer
Timer1.attachInterrupt(setCycleTime);



}//************************** End Setup ***************************************

elapsedMillis check_adc2;
elapsedMillis check_adc3;
elapsedMicros sweep_time;

void loop() {
  //check the IO pins
  byte sweep_On = !(digitalRead(d_c));

//adc2, A1, freq set pot...Check every 10 milliseconds
if(check_adc2 > 10){
  //keep running average
  adc2_val = (adc2_val + adc_smooth(adc2)) / 2;  
    //Serial.println(adc2_val);
 if(adc2_val > old_adc2_val || adc2_val < old_adc2_val ){
    //map adc2 to the note array
    //int temp_freq = map(adc2_val, 0, 1024, 0, 48);

    int temp_freq = float(adc2_val / 17);
    int temp_dac =  pgm_read_float_near(&dac_val[temp_freq]) ;
   Serial.print(temp_dac );
   Serial.print(" ");
   Serial.println(float(dutyCycle));

    //read note value from array
    hertz =  pgm_read_float_near(&dco_freq[temp_freq + 12]) ;
    //set new cycle time
    calculateCycleTime();
    //set the dac
    dac_write(temp_dac);
  }
    //save old adc value
    old_adc2_val = adc2_val;
    //reset timer
    check_adc2 = 0; 
}//*************************  End Check ADC **********************************

//If not sweeping the duty cycle, check for control change ever 20 milliseconds
if(!sweep_On){
  if(check_adc3 > 20){
    adc3_val = (adc3_val + adc_smooth(adc3)) / 2; 
      if(adc3_val > (old_adc3_val )  || adc3_val < (old_adc3_val )){
        int temp_duty_cycle = constrain(adc3_val, 100, 900);  //limit to 10% to 90%
        dutyCycle = (float)temp_duty_cycle * .001;
        calculateCycleTime();
      }
    old_adc3_val = adc3_val;
    check_adc3 = 0;
  }
}

if(sweep_On){
    adc3_val = (adc3_val + adc_smooth(adc3)) / 2; 
    int temp_sweep = adc3_val + 1;        //avoid a zero reading
    sweepCycle = 50 +(temp_sweep * 20);
    if(sweep_time > sweepCycle){
      if(sweepUp){
        sweepCount = sweepCount + 5;
        if(sweepCount > 400){
          sweepUp = false;
        }
      }
      if(!sweepUp){
        sweepCount = sweepCount - 5;
          if(sweepCount < -400){
            sweepUp = true;
          }
      }
      dutyCycle = (float)((525 + sweepCount) * .001);
      calculateCycleTime(); 
      sweep_time = 0;
    }
}

  
}//************************* End Loop *****************************************

int adc_smooth(int pin){
  int adc_smooth_max = 0;
  int adc_smooth_min = 1024;
  int adc_smooth_avg = 0;
  int adc_smooth_temp = 0;
  float adc_smooth_return = 0;
  for(byte ss = 0; ss < 4; ss ++){
    adc_smooth_temp = analogRead(pin);
    //get min and max values
    if(adc_smooth_temp > adc_smooth_max){adc_smooth_max = adc_smooth_temp;}
    if(adc_smooth_temp < adc_smooth_min){adc_smooth_min = adc_smooth_temp;}
    adc_smooth_avg = adc_smooth_avg + adc_smooth_temp;
  }
    //remove the lowest and high value and return the average
    adc_smooth_return = ((adc_smooth_avg - adc_smooth_max) - adc_smooth_min) / 2;
    return adc_smooth_return;
}

void dac_write(int cc){

  Wire.beginTransmission(0x61);
  Wire.write(64);                     // cmd to update the DAC
  Wire.write(cc >> 4);        // the 8 most significant bits...
  Wire.write((cc & 15) << 4); // the 4 least significant bits...
  Wire.endTransmission();
  write_dac = false;                //Reset for the next interrupt   
}
