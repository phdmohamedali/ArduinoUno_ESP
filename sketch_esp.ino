#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <vector>
#include <ArduinoJson.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}

#define I2CAddressESPWifi 6
#define RSSI_THR  -90



const char* clientSsid  = "YStop-WiFi";
const char* clientPassword = "robebekeme";


static const char* ssid = "eduroam";
// Username for authentification
static const char* username = "......";
// Password for authentication
static const char* password = "......";


static const char* ap_name = "1016";
static const char* ap_password = "1234567";
 
struct softap_config wifi_ap_config;
struct station_config wifi_config;

int cur = 0;


char c_temp[3] = {'9', '9', '9'};
char c_dis[3] = {'9', '9', '9'};
char c_temp_dis[6] = {'9', '9', '9','9', '9', '9'};


HTTPClient http;
WiFiEventHandler probeRequestPrintHandler;


String macToString(const unsigned char* mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

std::vector<WiFiEventSoftAPModeProbeRequestReceived> myList;

void onProbeRequestPrint(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
  Serial.println("kookoooooo!!");
  if (evt.rssi >= RSSI_THR)
    myList.push_back(evt);
}

typedef struct t  {
  unsigned long tStart;
  unsigned long tTimeout;
};


t t_envir = {0, 3600000}; //Run every 4 seconds.

//Environment sensors
//int temp;
int dist = 0;
int temp = 0;


long offPeriod = 0;
long upperBound = 0;
long freq = 0;

bool tCheck (struct t *t ) {
  if (millis() > (t->tStart + t->tTimeout)) return true;
  return false;
}

void tRun (struct t *t) {
  t->tStart = millis();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("json:");

  Wire.begin(0, 2); //Change to Wire.begin() for non ESP.
  
  setup_wifi_AP_Station();
  Serial.println();
  Serial.println("Waiting for connection and IP Address from DHCP");
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  //protocol calssical WPA2
  /////////
  ///connecting to classical access point
  ///  WiFi.mode(WIFI_AP_STA);
  ///WiFi.softAP(apSsid, apPassword);
  ///WiFi.begin(clientSsid, clientPassword);
  ///while (WiFi.status() != WL_CONNECTED) {
  ///  Serial.print(".");
  ///  delay(100);
  ///}
  //////////
  
  Serial.println("");
  probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequestPrint);
 

  upperBound = 0;
  freq = 0;
  syncWithServer();
  checkProperAction(&t_envir);

}

void syncWithServer()
{
    http.begin("http://recsys.ystop.com.au/report1.php");
    Serial.print("requesting>>>>>");
    int httpCode = http.GET();
    //Check the returning code
    if (WiFi.status() == WL_CONNECTED) {                                                                  
      if (httpCode > 0) {
        const size_t bufferSize = JSON_OBJECT_SIZE(3) + 35;
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(http.getString());
        upperBound = root["to"]; 
        freq = root["freq"];
        freq = freq*1000; 
        offPeriod = root["off"];
 
        Serial.print("offPeriod:");
        Serial.println(offPeriod);
        Serial.print("to:");
        Serial.println(upperBound);
        Serial.print("freq:");
        Serial.println(freq);
      }
      http.end();   //Close connection
   }else{
      Serial.print("wireless is not connected");
   }
}

void checkProperAction(struct t *t)
{
  int hours, minutes, seconds;

  if (upperBound == 0)   //go sleep response
  {
    Serial.println("turnoff");
    ESP.deepSleep(offPeriod*1000);
    //freq = 1;
  } else if ( millis() < upperBound ) {  // still executing or start execting
    Serial.print("turn on:");
    Serial.println(freq);
    t->tTimeout = upperBound - millis() ;
  } else {  //this case is related only to stay off after execution
    Serial.print("in between");
    //ESP.deepSleep(offPeriod*1000);
    system_deep_sleep(offPeriod*1000);
    //freq = 1;
  }
  Serial.print("under control");
}

void loop() {
  //need update
  //curr_sec_of_day =  millis();
  if (tCheck(&t_envir)) {
    Serial.println("bet3eeeb");
    checkProperAction(&t_envir);
    //curr_sec_of_day = curr_sec_of_day + hour;
    tRun(&t_envir);
  }
  //curr_sec_of_day =  millis();
  if (millis() <= upperBound) {
      Wire.beginTransmission(I2CAddressESPWifi);
      Wire.write('1');
      Wire.endTransmission();
      delay(50);
      get_env();
      get_probe_send();
      Serial.println("leih keda");
      Serial.println(freq);
      delay(freq);
  }
}

void get_probe_send(void) {
  //Serial.println('<start probe send');
  String json = "";
  if (myList.size() > 0 )
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& device = root.createNestedObject("dev");
    device["id"] = ap_name;
    JsonArray& probes = root.createNestedArray("probes");
    bool has_value = false;

    for (WiFiEventSoftAPModeProbeRequestReceived w : myList) {
      JsonObject& probe = probes.createNestedObject();
      probe["address"] = macToString(w.mac);
      probe["rssi"] = w.rssi;
      has_value = true;
    }

    if (dist != 999 || temp != 999)
    {
      JsonObject& env = root.createNestedObject("env");
      if (dist != 999)
        env["dis"] = dist;
      if (temp != 999)
        env["tmp"] = temp;
    }
    
    root.printTo(json);
  } else if (dist != 999 || temp != 999)
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& device = root.createNestedObject("dev");
    device["id"] = ap_name;
    JsonObject& env = root.createNestedObject("env");
    if (dist != 999)
      env["dis"] = dist;
    if (temp != 999)
      env["tmp"] = temp;
    root.printTo(json);
  }
  if (dist != 999 || temp != 999 || myList.size() > 0) {
    Serial.println("json:" + json);
    http.begin("http://recsys.ystop.com.au/index_cwd.php");
    //http.begin("http://192.168.20.23/index.php");
    http.addHeader("Content-Type", "application/json");
    http.POST(json);
    http.end();
    Serial.println("done");
  }

  myList.clear();
  Serial.println("start probe send>");
}

void get_env(void) {
  //delay(500);
  Wire.requestFrom(I2CAddressESPWifi, 6);
  int i = 0;
  
  while (Wire.available())
  {
    c_temp_dis[i++] = Wire.read();
  }
  
  c_temp[0] = c_temp_dis[0];
  c_temp[1] = c_temp_dis[1];
  c_temp[2] = c_temp_dis[2];
  c_dis[0] = c_temp_dis[3];
  c_dis[1] = c_temp_dis[4];
  c_dis[2] = c_temp_dis[5]; 
  //sscanf(c_temp, "%d", &temp);
  sscanf(c_temp, "%d", &temp);
  sscanf(c_dis, "%d", &dist);
  Serial.println("start env>");
}

void setup_wifi_AP() {
  //wifi_set_opmode(SOFTAP_MODE);
  //Parameters for wifi AP
  //Wi-Fi name
  Serial.write("\r\nStart .....AP");
  strcpy((char*)wifi_config.ssid, ap_name);
  strcpy((char*)wifi_config.password, ap_password);
  Serial.write("\r\ninter 1 .....AP");
  //memcpy(wifi_ap_config.ssid,ap_name, os_strlen(ap_name)*sizeof(char));
  //Wi-Fi password
  //memcpy(wifi_ap_config.password,ap_password, sizeof(ap_password));
  //Other parameters
  wifi_ap_config.ssid_len = sizeof(wifi_ap_config.ssid);
  wifi_ap_config.channel = 5;
  wifi_ap_config.ssid_hidden = 0;
  wifi_ap_config.beacon_interval = 60000;
  //if(os_strcmp(ap_password,"")==0) {
  //    wifi_ap_config.authmode = AUTH_OPEN;
  //}
  //else
  wifi_ap_config.authmode = AUTH_WPA2_PSK;
  wifi_ap_config.max_connection = 4;
  Serial.write("\r\ninter 3 .....AP");
  wifi_softap_set_config(&wifi_ap_config);
  Serial.write("\r\ninter 4 .....AP");
  wifi_softap_dhcps_start();
  Serial.write("\r\ninter 5 .....AP");
}

void setup_wifi_AP_Station()
{
   WiFi.persistent(false);
   wifi_station_disconnect();
   wifi_station_set_reconnect_policy(true); 
   wifi_set_opmode(STATIONAP_MODE);
   memset(&wifi_config, 0, sizeof(wifi_config));
   strcpy((char*)wifi_config.ssid, ssid);
    wifi_station_set_config(&wifi_config);
    //wifi_station_clear_cert_key();
    //wifi_station_clear_enterprise_ca_cert();
    wifi_station_set_wpa2_enterprise_auth(1);
    wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
    wifi_station_set_enterprise_username((uint8*)username, strlen(username));
    wifi_station_set_enterprise_password((uint8*)password, strlen(password));
    wifi_station_connect();

    //WiFi.softAP("dd", "1111222233");
  // WPA2 Connection ends here

  // Normal Connection starts here
  /*
  WiFi.mode(WIFI_STA);
  Serial.write("\r\nConnect to WLAN");
  WiFi.begin(ssid, password);
  // Normal Connection ends here
  */
  //
  //WiFi.softAP(ap_name, ap_password);
  setup_wifi_AP();
  delay(8000);
  // Wait for connection AND IP address from DHCP
  Serial.println();
  Serial.println("Waiting for connection and IP Address from DHCP");
}
