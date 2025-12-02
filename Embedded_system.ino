#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <DHT_U.h>

// neopixel defines
#define LED_PIN 27 // digital pin on neopixel
#define LED_COUNT  8 // amount of leds on neopixel
#define BRIGHTNESS 50 // max 255

// Temperature defines
#define DHTPIN 14 // digital pin on temperature sensor
#define DHTTYPE DHT11 // DHT 11

// button define
#define BUTTON_PIN 12 // digital pin on button

// UV variable
int UV = 0;

class Temperature
{
  public: 
  DHT_Unified dht;
  Adafruit_NeoPixel pixels;
 
  Temperature() : dht(DHTPIN, DHTTYPE), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}
  uint32_t delayMS = 1000;

  void tempSetup()
  {
    Serial.begin(115200);
    dht.begin();
    pixels.begin();
    pixels.setBrightness(BRIGHTNESS);
    pixels.show();
  }

  void setNeoPixel(sensors_event_t event)
  {
    pixels.clear();
    float temp = event.temperature;
    int index = 0;

    // sets how many leds need to light up
    if(temp <= 13)      index = 0;
    else if(temp <= 16) index = 1;
    else if(temp <= 19) index = 2;
    else if(temp <= 22) index = 3;
    else if(temp <= 25) index = 4;
    else if(temp <= 28) index = 5;
    else if(temp <= 31) index = 6;
    else                index = 7;
  
    // sets the color the led at a certain index has to take  
    for (int i = 0; i <= index; i++)
    {
      uint32_t color;
      if(i < 2)                color = pixels.Color(0, 150, 0); // green
      else if(i < 5 && i >= 2) color = pixels.Color(150, 150, 0); // oranje like
      else if(i < 7 && i >= 5) color = pixels.Color(255, 0, 0); // red
      else                     color = pixels.Color(255, 0, 255); // purple
      pixels.setPixelColor(i, color);
    }
    pixels.show();
  }

  void tempLoop()
  {
    delay(delayMS);
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    }
    else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    setNeoPixel(event);
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    }
    else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    }
  }
};

class Light 
{
  public:
  Adafruit_TSL2561_Unified tsl;
  uint32_t delayMS = 1000;

  Light() : tsl(TSL2561_ADDR_FLOAT, 12345) {}

  void configureSensor(void)
  {
    /* You can also manually set the gain or enable auto-gain support */
    // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
    // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
    tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
    
    /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

    /* Update these values depending on what you've set above! */  
    Serial.println("------------------------------------");
    Serial.print  ("Gain:         "); Serial.println("Auto");
    Serial.print  ("Timing:       "); Serial.println("13 ms");
    Serial.println("------------------------------------");
  }

  void lightSetup()
  {
    configureSensor();
  }

  void luxToUv(sensors_event_t event)
  {
    tsl.getEvent(&event);
    // Everything above 2000 lux is UV 10, every 200 lux is 1 one extra UV
    if(event.light < 2199)
    {
      UV = event.light / 200;
    }
    else
    {
      UV = 10;
    }

    Serial.print(UV); Serial.println(" UV");
  }

  void lightLoop()
  {
    /* Get a new sensor event */ 
    sensors_event_t event;
    tsl.getEvent(&event);
  
    /* Display the results (light is measured in lux) */
    if (event.light)
    {
      Serial.print(event.light); Serial.println(" lux");
      luxToUv(event);
    }
    else
    {
      /* If event.light = 0 lux the sensor is probably saturated
        and no reliable data could be generated! */
      Serial.println("Sensor overload");
    }
    delay(delayMS);
  }
};

class SunScreen
{
  // For calculations we are gonna assume this person has skintype I
  // For calculations we are gonna assume this person has sunscreen with SPF 30
  // For calculations we are gonna assume this person starts walking outside right when they applied their first layer of sunscreen
  public:
  int timeTillBurn = 0;
  bool startCountDown = true;
  SunScreen() {}

  void sunscreenSetup()
  {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
  }

  void checkButton()
  {
    if(digitalRead(BUTTON_PIN) == LOW)
    {
      startCountDown = true;
    }
  }

  void sunscreenTimer(int UV)
  {
    if (startCountDown == true && UV > 0)
    {
      timeTillBurn = (30 * 67) / UV;
      startCountDown = false;
    }
    else timeTillBurn = 0;
  }

  void sunscreenLoop()
  {
    sunscreenTimer(UV);
    checkButton();
    Serial.print(timeTillBurn);
    Serial.println( " minuten tot je verbrand");
  }

};
Temperature temperature;
Light light;
SunScreen sunscreen;


void setup() {
  // put your setup code here, to run once:
  temperature.tempSetup();
  light.lightSetup();
  sunscreen.sunscreenSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  temperature.tempLoop();
  light.lightLoop();
  sunscreen.sunscreenLoop();
}
