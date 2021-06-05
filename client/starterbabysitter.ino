// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include "SparkFun_SGP30_Arduino_Library.h"

#include <Grove_LCD_RGB_Backlight.h>

#include "HttpClient.h"

#include <SparkFunRHT03.h>

#include "Adafruit_CAP1188.h"

#include <Wire.h>

#include "application.h"

#include <cmath>

#define ROTARY_ANGLE_SENSOR A0
SGP30 aqs;
HttpClient http;
Adafruit_CAP1188 touch = Adafruit_CAP1188();
RHT03 rht;
rgb_lcd lcd;

http_header_t headers[] {
    {"Accept", "*/*"},
    {NULL, NULL}
};

http_request_t request;
http_response_t response;

const int colorR = 255;
const int colorG = 255;
const int colorB = 255;
const int RHT03_DATA_PIN = 4;
const int ALC_FEED_THRESHOLD = 10;
const int FEED_TIME = 10000;
double referenceEthanolConc = 0;
bool working = false;

unsigned int feedTimeTracker = 0;
unsigned int nextTime = 0;
unsigned int nextMeasureTime = 0;

// Source: https://github.com/sparkfun/SparkFun_SGP30_Arduino_Library/blob/main/examples/Example3_Humidity/Example3_Humidity.ino
// From the example code for the SGP30 air quality sensor; Just some math to convert relative humidity to absolute humidity.
double RHtoAbsolute (float relHumidity, float tempC) {
  double eSat = 6.11 * pow(10.0, (7.5 * tempC / (237.7 + tempC)));
  double vaporPressure = (relHumidity * eSat) / 100; //millibars
  double absHumidity = 1000 * vaporPressure * 100 / ((tempC + 273) * 461.5); //Ideal gas law with unit conversions
  return absHumidity;
}

// Source: https://github.com/sparkfun/SparkFun_SGP30_Arduino_Library/blob/main/examples/Example3_Humidity/Example3_Humidity.ino
// From the example code for the SGP30 air quality sensor; Just some math to make the double useable for the intended function.
uint16_t doubleToFixedPoint(double number) {
  int power = 1 << 8;
  double number2 = number * power;
  uint16_t value = floor(number2 + 0.5);
  return value;
}

double ethanolRelative(double S){
    // Doesn't actually give a PPM since that'd require a reference concentration,
    // but this will offer an idea of concentration relative to atmospheric conditions.
    // This in mind, we will just say that there's a reference concentration of 1 part per million ethanol
    // (which is way off, but again, we don't actually care about absolute concentration)
    // Numerator of this formula derived from the one given in the SGP30 datasheet.
    return pow(M_E, ((referenceEthanolConc-S)/512));
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    if(aqs.begin(Wire) == false){
        Serial.printlnf("oop");
    }
    aqs.initAirQuality();
    if (!touch.begin(0x28)){
        Serial.printlnf("Touch sensor not connected");
    }
    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);
    rht.begin(RHT03_DATA_PIN);
    
    // Increase the accuracy of the air quality sensor by giving it real absolute humidity data.
    aqs.setHumidity(doubleToFixedPoint(RHtoAbsolute(rht.humidity(), rht.tempC())));
    aqs.measureRawSignals();
    referenceEthanolConc = aqs.ethanol;
    touch.setSensitivity(3);
}

void loop() {
    // Datasheet says measurements have to be taken every second to ensure accuracy,
    // so I am doing my best of adhering to that here.
    if(millis() > nextMeasureTime){
        aqs.measureAirQuality();
        aqs.measureRawSignals();
        nextMeasureTime += 1000;
    }
    if(nextTime > millis()){
        return;
    }
    
    String dataTransfer = "/?";
    
    // SENSING
    int updatedSuccessfully = rht.update();
    float humidity = rht.humidity();
    float tempC = rht.tempC();
    double alcoholReading = ethanolRelative(aqs.ethanol);
    bool risen = touch.touched() == 1;
    
    lcd.print(working);
    dataTransfer += "tempC=" + String(tempC, 1);
    dataTransfer += "&hum=" + String(humidity, 1);
    dataTransfer += "&alc=" + String(alcoholReading, 2);
    dataTransfer += "&risen=" + String(risen);
    
    // DATA TRANSMISSION
    Serial.println();
    Serial.println(dataTransfer);
    Serial.println("Application>\tStart of Loop.");
    
    request.hostname = "13.58.178.222";
    request.port = 5000;
    request.path = dataTransfer;
    
    http.get(request, response, headers);
    
    // The server now determines when to feed the starter;
    // This is preferable since it lets feeding preferences be updated
    // by only touching the easy-to-access server, rather than having to change
    // any code here.
    Serial.print("Application>\tResponse status: ");

    Serial.println(response.status);
    
    Serial.print("Application>\tHTTP Response Body: ");
    
    Serial.println(response.body);
    
    // If the starter need to be fed given the most recent reading, then the 
    // server will tell this code to "feed" it.
    if(response.body != ""){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Feed your starter!");
        feedTimeTracker = millis() + FEED_TIME;
    }else{
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Monitoring...");
    }
    
    nextTime = millis() + 10000;

    delay(10);
}