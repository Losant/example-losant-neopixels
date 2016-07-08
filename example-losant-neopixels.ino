#include <WiFi101.h>
#include <Losant.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Your WiFi credentials.
const char* WIFI_SSID = "MY_NETWORK_NAME";
const char* WIFI_PASS = "MY_NETWORK_PASSWORD";

// Losant connection credentials
const char* LOSANT_DEVICE_ID = "MY_DEVICE_ID";
const char* LOSANT_ACCESS_KEY = "MY_ACCESS_KEY";
const char* LOSANT_ACCESS_SECRET = "MY_ACCESS_SECRET";

// NeoPixel setup
#define PIXEL_BRIGHTNESS 200 // 0 to 255, the standard pixel brightness
#define PIN 6 // the pin through which data will be sent
#define NUMPIXELS 4 // total number of pixels we're using
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800); // instantiating the library
uint32_t stripColor = pixels.Color(0, 0, 0); // create the color and begin with 'off'

WiFiSSLClient wifiClient;
LosantDevice device(LOSANT_DEVICE_ID);

// Called whenever the device receives a command from the Losant platform.
void handleCommand(LosantCommand *command) {
  // it will be necessary to change the MQTT_MAX_PACKET_SIZE in your pubsubclient library.
  // see https://forums.losant.com/t/sending-commands-with-larger-payloads/135
  Serial.print("Command received: ");
  Serial.println(command->name);
  JsonObject& payload = *command->payload;

  if(strcmp(command->name, "setColor") == 0) {
    // only do something if the command name matches
    setColor(payload);
  }

}

void setColor(JsonObject& color) {
  // expecting color to be an object
  // {"r": int,"g":int,"b":int, invert: bool, index: int}
  Serial.println("setting color");
  color.printTo(Serial);
  Serial.println("");
  int tmpColorR = color["r"];
  int tmpColorG = color["g"];
  int tmpColorB = color["b"];
  int tmpIndex = color["index"];
  bool tmpInverted = color["inverted"];
  if(tmpInverted == true) { // invert the color!
    tmpColorR = 255 - tmpColorR;
    tmpColorG = 255 - tmpColorG;
    tmpColorB = 255 - tmpColorB;
  }
  stripColor = pixels.Color(tmpColorR,tmpColorG,tmpColorB);
  // if a proper index is sent, set the color of just that LED.
  // otherwise, set all LEDs to that color.
  if(tmpIndex >= 0 && tmpIndex <= NUMPIXELS) {
    pixels.setPixelColor(tmpIndex, stripColor);
    sendPixelState(tmpIndex, tmpColorR, tmpColorG, tmpColorB);
  } else {
    for(int i = 0; i < NUMPIXELS; i = i + 1) {
      pixels.setPixelColor(i, stripColor);
      sendPixelState(i, tmpColorR, tmpColorG, tmpColorB);
    }
  }
  pixels.show(); // send the new data to the pixels
}

void sendPixelState(int index, int r, int g, int b) {
  char keyName [32];
  sprintf ( keyName, "pixel%dColor", index ); // construct our key from the given index
  char val [64];
  sprintf(val, "(%d, %d, %d)", r,g,b); // construct our string value from the given colors
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[keyName] = val; // provide the attribute and its value as a key/value pair
  device.sendState(root);
}

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
}


void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  device.onCommand(&handleCommand);

  connect();
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(PIXEL_BRIGHTNESS);
  pixels.show(); // Initialize all pixels to 'off'
  // and send state of all pixels as 'off'
  for(int i = 0; i < NUMPIXELS; i = i + 1) {
    sendPixelState(i,0,0,0);
  }
}



void loop() {

  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if(!device.connected()) {
    Serial.println("Disconnected from Losant");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if(toReconnect) {
    connect();
  }

  device.loop();

}
