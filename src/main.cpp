#include <Arduino.h>
// #include <Wire.h>
#include <ESP8266WiFi.h>
// #include <SocketEvent.h>
#include <WebSocketsClient.h>

#define USE_SERIAL Serial

const char* ssid = "Home";
const char* password = "genesiswifi";

/** WEBSOCKETS CONNECTION PARAMETERS **/
const char* path = "/control";
const char* host = "192.168.0.2";
const int port = 80;

// WEBSOCKETS CLIENT
WebSocketsClient webSocket;

// SOCKETEVENT HANDLE FOR CAR
// SocketEvent SOCKET(webSocket);


void webSocket_event(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");

            break;
        case WStype_CONNECTED: {
            // String text = (char*)payload;
            // String websocket("{\"websocket\":{\"status\":\"connected\", \"url\":\"" + text + "\"}}");

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
  // webSocket.onEvent([&](WStype_t typ, uint8_t* payload, size_t length) {
  //   SOCKET.webSocket_event(typ, payload, length);
  // });

  // try ever 2000 again if connection has failed
  webSocket.setReconnectInterval(2000);

 
}

int data = 0;

unsigned long last_request = 0;
unsigned long delay_in_millis = 500;


void loop() {
  webSocket.loop();


 if (millis() - last_request >= delay_in_millis) {  
      data++;
      /* creating a string that can be parsed by JSON */
      String test_data("{\"sensor\":{\"name\":\"test\",\"data\":" + String(data) + "}}");
      webSocket.sendTXT("test");  // send string to websocket server

      last_request = millis();
    }

}