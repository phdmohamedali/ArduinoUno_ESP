#include <Wire.h>
//#include <hcsr04.h>
#include <NewPing.h>

/**
 * wires mapping 
 */
//temperature 
#define TEMP_BIN A0
//address of ESP13
#define I2CAddressESPWifi 6
//12 and 13 is for  ultrasonic
#define TRIG_PIN 11
#define ECHO_PIN 12

//#define TRIG_PIN_1 10
//#define ECHO_PIN_1 11

#define MAX_DISTANCE 200
//#define RESET_PIN 8

/** 
 *  datastructure for distance/temperature where t_* are used to track 
 *  last sent temp to avoid sending same information again
*/
int dist = 0;
int t_dist = 0;
//int dist_2 = 0;
//int t_dist_2 = 0;
const int err_d = 10;
int temp = 0;
int t_temp = 0;
const int err_t = 10;
//datastructure for temp/dis
char c_temp_dis[6] ={'9','9','9','9','9','9'};

int do_it = 0;

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);


void computeAndSend()
{
  int reading = analogRead(TEMP_BIN);
  float voltage = reading * 5.0;
  voltage /= 1024.0;
  float temperatureC = (voltage - 0.5) * 100;  //converting from 10 mv per degree wit 500 mV offset
  temp = (int)(temperatureC);
  if(temp == 0){
    printf(&c_temp_dis[0], "%03d", 999);
  }else{
    if (t_temp <= (temp + err_t) && t_temp >= (temp - err_t))
    {
      sprintf(&c_temp_dis[0], "%03d", 999);
    }
    else
    {
      t_temp = temp; 
      sprintf(&c_temp_dis[0], "%03d", temp);   
    }
  }
  
  dist = (int)sonar.ping_cm();
  if(dist == 0){
    printf(&c_temp_dis[3], "%03d", 999);
  }
  else{
    if (t_dist <= (dist + err_d) && t_dist >= (dist - err_d)){
      sprintf(&c_temp_dis[3], "%03d", 999);
    }
    else{
      t_dist = dist;
      sprintf(&c_temp_dis[3], "%03d", dist);     
    }
  }
    Serial.println(c_temp_dis);
}
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(115200);
  Serial.println("setup");
  Wire.begin(I2CAddressESPWifi);
  Wire.onReceive(espWifiReceiveEvent);
  Wire.onRequest(espWifiRequestEvent);
}

void loop()
{
  if(do_it > 0)
  {
    computeAndSend();
    do_it = 0;
  }
}

void espWifiReceiveEvent(int count)
{
  int i = 0;
  char c;
  while (Wire.available())
  {
    c = Wire.read();
    i++;
  }
  do_it = 10;
  Serial.println("koko");
}
void espWifiRequestEvent()
{
  //update
  Wire.write(c_temp_dis);
}
