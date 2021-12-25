#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <DFRobot_LIS2DW12.h>

#define USE_SERIAL Serial

const char* ssid = "Home";
const char* password = "genesiswifi";

/** WEBSOCKETS CONNECTION PARAMETERS **/
const char* path = "/control";
const char* host = "192.168.0.2";
const int port = 80;

int lastOrientation = 0; // no event happened

// WEBSOCKETS CLIENT
WebSocketsClient webSocket;

// LIS2Dw12 ACCELEROMETER DFRobot SEN0409  (LIB)
DFRobot_LIS2DW12_I2C acce(&Wire, 0x18);

// WEBSOCKET EVENTS
void webSocket_event(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");

            break;
        case WStype_CONNECTED: {
            // String text = (char*)payload;

            // webSocket.sendTXT(websocket);
            webSocket.sendTXT("Connected");
            break;
        }
        case WStype_TEXT: {
            // String text = (char*)payload;  // convert to string (-_-)
            USE_SERIAL.printf("[WSc] get text: %s\n", payload);

            // send message to server
            webSocket.sendTXT("message here");
            break;
        }
        case WStype_BIN:
            USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);

            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
        default:
            break;
    }
}

void setup() {
  // set I2C pin
  Wire.begin(0, 2); // set SDA SCL pin

  USE_SERIAL.begin(115200);
  delay(10);

  // we start by connecting to a WiFi network
  USE_SERIAL.print("Connecting to ");
  USE_SERIAL.println(ssid);

  /* explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default, */
  /* would try to act as both a client and an access-point and could cause */
  /* network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  };

  USE_SERIAL.println("");
  USE_SERIAL.println("WiFi connected");
  USE_SERIAL.println("IP address: ");
  USE_SERIAL.println(WiFi.localIP());

 // connection begin
  webSocket.begin(host, port, path);

  // event handler
  webSocket.onEvent(webSocket_event);

  // try ever 2000 again if connection has failed
  webSocket.setReconnectInterval(2000);
 
  while(!acce.begin()){
    USE_SERIAL.println("Communication failed, check the connection and I2C address setting when using I2C communication.");

    delay(1000);
  }

  USE_SERIAL.print("chip id : ");
  USE_SERIAL.println(acce.getID(),HEX);

  // chip soft reset
  acce.softReset();
  // set whether to collect data continuously
  acce.continRefresh(true);
  // set the sensor data collection rate:
  // acce.setDataRate(DFRobot_LIS2DW12::eRate_200hz);
  acce.setDataRate(DFRobot_LIS2DW12::eRate_800hz);
  // set the sensor measurement range: e2_g /<±2g>/  e4_g /<±4g>/  e8_g /<±8g>/  e16_g /< ±16g>/
  acce.setRange(DFRobot_LIS2DW12::e2_g);
  // set filter 
  acce.setFilterPath(DFRobot_LIS2DW12::eLPF);
  // set bandwidth
  acce.setFilterBandwidth(DFRobot_LIS2DW12::eRateDiv_4);
  // set power mode:
  acce.setPowerMode(DFRobot_LIS2DW12::eContLowPwrLowNoise1_12bit);  

  // double tap detection
  // enable tap detection in the Z direction
  acce.enableTapDetectionOnZ(true);
  // the threshold setting in the Z direction   
  // threshold(mg),Can only be used in the range of ±2g)
  acce.setTapThresholdOnZ(/*Threshold = */0.5);
  // set the interval time between two taps when detecting double tap
  acce.setTapDur(/*dur=*/15);
  // set tap detection mode:
  acce.setTapMode(DFRobot_LIS2DW12::eBothSingleDouble);

  // set the threshold of the angle when turning
  acce.set6DThreshold(DFRobot_LIS2DW12::eDegrees50);

  /**！
    Set the interrupt source of the int1 pin:
    eDoubleTap(Double click)
    eFreeFall(Free fall)
    eWakeUp(wake)
    eSingleTap(single-Click)
    e6D(Orientation change check)
  */
  acce.setInt1Event(DFRobot_LIS2DW12::eDoubleTap);
  delay(100);

}


unsigned long last_tap_request = 0;
unsigned long delay_in_millis = 200;


void loop() {
  webSocket.loop();

  // tap detected
  DFRobot_LIS2DW12:: eTap_t tapEvent = acce.tapDetect();
  // tap source detection
  DFRobot_LIS2DW12::eTapDir_t dir = acce.getTapDirection();

  uint8_t tap = 0;

  // double tap detection
  if(tapEvent  == DFRobot_LIS2DW12::eDTap){  
      tap = 1;
  }
   
  if(tap == 1){
        if (millis() - last_tap_request >= delay_in_millis) {  
          if(dir == DFRobot_LIS2DW12::eDirZUp){
            /* creating a string that can be parsed by JSON */
            String dir_data("{\"sensor\":{\"name\":\"LIS2DW12\",\"type\":\"tap_dir\",\"data\":" + String(dir) + "}}");
            webSocket.sendTXT(dir_data);
          }else if(dir == DFRobot_LIS2DW12::eDirZDown){
            String dir_data("{\"sensor\":{\"name\":\"LIS2DW12\",\"type\":\"tap_dir\",\"data\":" + String(dir) + "}}");
            webSocket.sendTXT(dir_data);
          }
        
        tap = 0;
        last_tap_request = millis();
    }
  }

  // check changes detected in six directions
  if(acce.oriChangeDetected()){
    DFRobot_LIS2DW12::eOrient_t orientation = acce.getOrientation();
    if(lastOrientation != orientation){
        String ori_data("{\"sensor\":{\"name\":\"LIS2DW12\",\"type\":\"orientation\",\"data\":" + String(orientation) + "}}");
        webSocket.sendTXT(ori_data);
        lastOrientation = orientation;
    }
  }

}