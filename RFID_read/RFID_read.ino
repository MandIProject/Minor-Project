#include <EasyMFRC522.h>
#include <RfidDictionaryView.h>
#include <ESP8266WiFi.h>

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ -1);

int startBlock = 18;  // you may choose any block; choose #1 to have maximum storage available for the dictionary
RfidDictionaryView rfidDict(D8, D3, startBlock); // parameters: ports for SDA and RST pins, and initial block in the RFID tag
bool tagSelected = false;

const char *auth = "a0SHUV6doj1fws8b01D8dG6ujFHqaFa7";

const char *ssid = "Wifi_2ghz";
const char *pass = "Voidsetup()1";

const char *host = "blynk-cloud.com";
const unsigned int port = 8080;
String response;

WiFiClient client;

struct RFID_data{
  String name;
  String sex;
  String age;
  String phone;
};

struct RFID_data data;

void connectNetwork()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);

  u8g2.begin();
  connectNetwork();
}

bool httpRequest(const String& method,
                 const String& request,
                 String&       response)
{
  Serial.print(F("Connecting to "));
  Serial.print(host);
  Serial.print(":");
  Serial.print(port);
  Serial.print("... ");
  if (client.connect(host, port)) {
    Serial.println("OK");
  } else {
    Serial.println("failed");
    return false;
  }

  client.print(method); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(host);
  client.println(F("Connection: close"));
  if (request.length()) {
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: ")); client.println(request.length());
    client.println();
    client.print(request);
  } else {
    client.println();
  }

  //Serial.println("Waiting response");
  int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  //Serial.println("Reading response");
  int contentLength = -1;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    line.toLowerCase();
    if (line.startsWith("content-length:")) {
      contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
    } else if (line.length() == 0) {
      break;
    }
  }

  //Serial.println("Reading response body");
  response = "";
  response.reserve(contentLength + 1);
  while (response.length() < contentLength && client.connected()) {
    while (client.available()) {
      char c = client.read();
      response += c;
    }
  }
  client.stop();
  return true;
}

void write_data(String data, String V_pin){
  Serial.print("Sending value: ");
  Serial.println(data);

  String putData = String("[\"") + data + "\"]";
  if (httpRequest(String("PUT /") + auth + "/update/" + V_pin, putData, response)) {
    if (response.length() != 0) {
      Serial.print("WARNING: ");
      Serial.println(response);
    }
  }
}

void str_cpy(String dest, const String source){
  int length = source.length();
  int i = 0;
  for(i; source[i] != '\0'; i++){
    dest[i] = source[i]; 
  }
  dest[i] = '\0';
}

void loop() {
  // put your main code here, to run repeatedly:
  byte tagId[4] = {0, 0, 0, 0};
  if (!tagSelected) {
    Serial.println();
    Serial.println("APPROACH a Mifare tag. Waiting...");

    do {
      // returns true if a Mifare tag is detected
      tagSelected = rfidDict.detectTag(tagId);
      delay(5);
    } while (!tagSelected);

    Serial.printf("- TAG DETECTED, ID = %02X %02X %02X %02X \n", tagId[0], tagId[1], tagId[2], tagId[3]);
    Serial.printf("  space available for dictionary: %d bytes.\n\n", rfidDict.getMaxSpaceInTag());
  }

  if((tagId[0] == 0xB3 && tagId[1] == 0x73 && tagId[2] == 0x6F && tagId[3] == 0x19) || (tagId[0] == 0x0C && tagId[1] == 0xFB && tagId[2] == 0x05 && tagId[3] == 0x39)){
    Serial.println("Match found");
    String key, value;
    int numKeys = rfidDict.getNumKeys();
  
    for (int i = 0; i < numKeys; i ++) {
      key = rfidDict.getKey(i);
      if(key.equals("PERSON")){
        value = rfidDict.get(key);
        write_data(value, "V8");
        str_cpy(data.name, value);
      }else if(key.equals("AGE")){
        value = rfidDict.get(key);
        write_data(value, "V7");
        str_cpy(data.age, value);
      }else if(key.equals("SEX")){
        value = rfidDict.get(key);
        write_data(value, "V6");
        str_cpy(data.sex, value);
      }else if(key.equals("PHONE")){
        value = rfidDict.get(key);
        write_data(value, "V5");
        str_cpy(data.phone, value);
      }
    }
    write_data("1", "V10");
    Serial.printf("Name: %s", data.name);
    Serial.printf("Age: %s", data.age);
    Serial.printf("Sex: %s", data.sex);
    Serial.printf("Phone: %s", data.phone);
  }else{
    Serial.println("Unauthorized card");
    write_data("0", "V10");
  }

  rfidDict.disconnectTag(true);
  tagSelected = false;
  delay(4000);
}
