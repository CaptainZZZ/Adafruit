//BME280+LED notification+wifi beacon detector+wifi scanner
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
//Adafruit io 
#define WLAN_SSID       ""
#define WLAN_PASS       ""
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish Net = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Net");
Adafruit_MQTT_Publish pressure = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pressure");
Adafruit_MQTT_Publish wifibeacon = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/wifibeacon");
Adafruit_MQTT_Subscribe ledonoff = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/LED");
Adafruit_MQTT_Subscribe ledbkg = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ledbkg");
float t;//BME280 readings
float rh;
float p;
Adafruit_BME280 bme;
void fade(int x, int y1, int y2=10, int y3=10);// for functions with optional parameter it need to be decleared in the header!Unused pin has to be 10

void setup() {
Serial.begin(9600);
bme.begin(0x76);  //The I2C address of the sensor is 0x76
// Connect to WiFi access point.
Serial.print("Connecting to ");
Serial.println(WLAN_SSID);
WiFi.mode(WIFI_STA);
WiFi.begin(WLAN_SSID, WLAN_PASS);
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
Serial.println();
Serial.println("WiFi connected");
Serial.println("IP address: "); 
Serial.println(WiFi.localIP());
mqtt.subscribe(&ledonoff);
mqtt.subscribe(&ledbkg);
pinMode(15,OUTPUT);//Relay control pin is GPIO15
}
//define publish & LED blink intervals
int accu_time1 = 0;
int accu_time2 = 0;
int pub_time = 10000;
int esp_res=0;
int led_time=1800;
int led_status=0;
int wificount=0;
int wifibeacon_status=1;//very important
int come_home;
int i;
void loop() {
MQTT_connect();
if (accu_time1 >= pub_time) {
  wificount=WiFi.scanNetworks();
  Serial.print(WiFi.SSID(0));
  Serial.print(",");
  Serial.print(WiFi.RSSI(0));
  Serial.print("dbm,channel ");
  Serial.println(WiFi.channel(0));
  for (i = 0; i < wificount; i++){
    if (WiFi.SSID(i)=="FaryLink_0B7651"){//find the presence of the beacon
      if (wifibeacon_status!=1){//to compare last value and new value
        come_home=1;
        wifibeacon.publish(come_home);
        fade(1,12,13,14);
      }
      wifibeacon_status=1;
      break;// if find beacon then jump out of loop
    }
    if (i==wificount-1){//if not at last loop set presence to 0
      if (wifibeacon_status==1){
        come_home=0;
        wifibeacon.publish(come_home);
        fade(1,13);
      }
      wifibeacon_status=0;
    }
  }
  Net.publish(WiFi.RSSI(0));
  WiFi.scanDelete();
  t = bme.readTemperature();
  rh = bme.readHumidity();
  p = bme.readPressure()/1000;
  if (String(p)=="nan"){
    esp_res++; //in case the BME280 has a "nan" reading then restart the chip
  }
  if (esp_res>=10){
    ESP.restart();}
  Serial.print("T:"); 
  Serial.print(t);
  Serial.print(" H: ");
  Serial.print(rh); 
  Serial.print(" P: ");
  Serial.println(p);
  temperature.publish(t); //publish data on Adafruit
  humidity.publish(rh);
  pressure.publish(p);
  analogWrite(2, 300); //blink when finish publishing
  delay(100);
  analogWrite(2, 1024);
  accu_time1 = 0;
}
if (accu_time2>=led_time){
  if (led_status==1){
    fade(1,12,13,14);
  }
  if (led_status==2){
    fade(1,14);
  }
  if (led_status==3){
    fade(1,13);
  }
  if (led_status==4){
    fade(1,12);
  }
  accu_time2 = 0;
}
  accu_time1 = accu_time1 + 50;
  accu_time2 = accu_time2 + 50;
  delay(30); // in total 25s/point published
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    // Check if its the onoff button feed
    if (subscription == &ledonoff) {
      Serial.print(F("Notification LED status: "));
      Serial.println((char *)ledonoff.lastread);
      if (strcmp((char *)ledonoff.lastread, "OFF") == 0) {
        analogWrite(13, 1024);
        analogWrite(12, 1024);
        analogWrite(14, 1024);
        led_status=0;}
      if (strcmp((char *)ledonoff.lastread, "ON") == 0) {
        analogWrite(13, 0);
        analogWrite(12, 0);
        analogWrite(14, 0);
        led_status=1;}
      if (strcmp((char *)ledonoff.lastread, "R") == 0) {
        analogWrite(13, 1024);
        analogWrite(12, 1024);
        analogWrite(14, 0);
        led_status=2;}
      if (strcmp((char *)ledonoff.lastread, "G") == 0) {
        analogWrite(13, 0);
        analogWrite(12, 1024);
        analogWrite(14, 1024);
        led_status=3;}
      if (strcmp((char *)ledonoff.lastread, "B") == 0) {
        analogWrite(13, 1024);
        analogWrite(14, 1024);
        analogWrite(12, 0);
        led_status=4;}}
    if (subscription == &ledbkg) {
      Serial.print(F("LED backlight On/Off button: "));
      Serial.println((char *)ledbkg.lastread);
      if (strcmp((char *)ledbkg.lastread, "ON") == 0) {
        digitalWrite(15,HIGH);}
      if (strcmp((char *)ledbkg.lastread, "OFF") == 0) {
        digitalWrite(15,LOW);}}
  }
}
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  int retry_interval = 10000;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(retry_interval);
    fade(1,2);
    fade(1,13);
    retry_interval = retry_interval * 2;
    if (retry_interval/864000000>=1){
      ESP.restart();
    }
  }
  Serial.println("MQTT Connected!");
  fade(2,2);
  fade(2,13);
}

void fade(int x, int y1,int y2, int y3) {
  //x is number of runs and y is pin#
  for (int i = x; i >= 1; i -= 1) {
    for (int fadeValue = 1024 ; fadeValue >= 0; fadeValue -= 16) {
      analogWrite(y1, fadeValue);
      analogWrite(y2, fadeValue);
      analogWrite(y3, fadeValue);
      delay(20);
    }
    for (int fadeValue = 0 ; fadeValue <= 1024; fadeValue += 16) {
      analogWrite(y1, fadeValue);
      analogWrite(y2, fadeValue);
      analogWrite(y3, fadeValue);
      delay(20);
    }
  }
}
