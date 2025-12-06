#include <Wire.h>
#include <Adafruit_Sensor.h> // Adafruit standard sensor library
#include <Adafruit_TSL2561_U.h> // Adafruit lux sensor library
#include <Adafruit_NeoPixel.h> // Adafruit neopixel library
#include <DHT.h> // temperature sensor library
#include <DHT_U.h> // temperature sensor library
#include <LiquidCrystal_I2C.h> // lcd library

// neopixel defines
#define LED_PIN 27 // digital pin on neopixel
#define LED_COUNT  8 // amount of leds on neopixel
#define BRIGHTNESS 50 // max 255

// Temperature defines
#define DHTPIN 14 // digital pin on temperature sensor
#define DHTTYPE DHT11 // DHT 11

// button define
#define BUTTON_PIN 21 // digital pin on button

// buzzer define
#define BUZZER 15

// UV variable
int UV = 0;

// Set to false to remove the UV limit of 11 to easily test the system
bool UVLimiter = false;

class Temperature
{
  public: 
  // initializing temperature sensor and neopixel
  DHT_Unified dht;
  Adafruit_NeoPixel pixels;
  
  // constructor
  Temperature() : dht(DHTPIN, DHTTYPE), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}

  // the setup for this class
  void tempSetup()
  {
    dht.begin();
    pixels.begin();
    pixels.setBrightness(BRIGHTNESS);
    pixels.show();
  }

  // this sets the neopixel leds at the right color depending on the temperature
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
      else if(i < 5 && i >= 2) color = pixels.Color(150, 150, 0); // orange 
      else if(i < 7 && i >= 5) color = pixels.Color(255, 0, 0); // red
      else                     color = pixels.Color(255, 0, 255); // purple
      pixels.setPixelColor(i, color);
    }
    pixels.show();
  }

  // the loop that keeps repeating for this class
  void tempLoop()
  {
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    }
    else {
    //Serial.print(F("Temperature: "));
    //Serial.print(event.temperature);
    //Serial.println(F("°C"));
    setNeoPixel(event);
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    }
    else {
    //Serial.print(F("Humidity: "));
    //Serial.print(event.relative_humidity);
   // Serial.println(F("%"));
    }
  }
};

class Light 
{
  public:
  // initializing lux sensor
  Adafruit_TSL2561_Unified tsl;

  // constructor
  Light() : tsl(TSL2561_ADDR_FLOAT, 12345) {}

  // method delivered by library to configure light sensor
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
  // the setup for this class
  void lightSetup()
  {
    configureSensor();
  }

  // converts the lux to UV for simulation
  void luxToUv(sensors_event_t event)
  {
    tsl.getEvent(&event);
    // Everything above 2000 lux is UV 10, every 200 lux is 1 one extra UV
    if(event.light < 2399 && UVLimiter == true)
    {
      UV = event.light / 200;
    }
    else if(UVLimiter == false)
    {
      UV = event.light / 7;
    }
    else
    {
      UV = 11;
    }

    Serial.print(UV); Serial.println(" UV");
  }
  // the loop that keeps repeating for this class
  void lightLoop()
  {
    // Get a new sensor event 
    sensors_event_t event;
    tsl.getEvent(&event);
  
    /* Display the results (light is measured in lux) */
    if (event.light)
    {
      //Serial.print(event.light); 
      //Serial.println(" lux");
      luxToUv(event);
    }
    else
    {
      /* If event.light = 0 lux the sensor is probably saturated
        and no reliable data could be generated! */
      Serial.println("Sensor overload");
    }
  }
};

// For calculations we are gonna assume this person has skintype I
// For calculations we are gonna assume this person has sunscreen with SPF 30
// For calculations we are gonna assume this person starts walking outside right when they applied their first layer of sunscreen
class SunScreen
{
  public:
  // timers
  int currentTimeTillBurn = 0;
  int lastTimeTillBurn = 0;
  int pastTimeAsInt = 0;

  // button variables
  bool startCountDown = false;
  bool lastButtonState = HIGH; // vorige status van de knop
  bool firstPressHappened = false;

  // lcd initialisation
  LiquidCrystal_I2C lcd;

  // constructor
  SunScreen() : lcd(0x27,16, 2) {}

  // this method checks if the buzzer needs to be activated
  void setBuzzer()
  {
    if (currentTimeTillBurn <= 0)
    {
      tone(BUZZER, 1000); // send 1KHz sound signal
      vTaskDelay(pdMS_TO_TICKS(1000));
      noTone(BUZZER); // stop sound
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
      noTone(BUZZER);  // stop sound
    }
  }

  // this method sets the start screen of the lcd
  void setInitialLCD()
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start doormiddel");
    lcd.setCursor(0, 1);
    lcd.print("van knop");
  }

  // this method sets the updated version of the lcd based on the remaining time untill you get burned
  void setUpdatedLCD()
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentTimeTillBurn); lcd.print(" seconden");
    lcd.setCursor(0, 1);
    lcd.print("tot je verbrand");
  }

  // the setup for this class
  void sunscreenSetup()
  {
    // intialize button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // initialize the lcd 
    lcd.init();                      
    lcd.backlight();
    lcd.clear();
    // initialize buzzer
    pinMode(BUZZER, OUTPUT);
  }
  
  // this method calculates the seconds needed untill you need to apply sunscreen
  void sunscreenCalculation(bool appliedSunscreen)
  {
    // this calculates the needed seconds when you just applied sunscreen
    if(appliedSunscreen)
    {
      lastTimeTillBurn = (30 * 67) / UV;
      lastTimeTillBurn *= 60;
      pastTimeAsInt = 0;
    }
    // this calculates the needed seconds after u applied the sunscreen
    else
    {
      lastTimeTillBurn = (30 * 67) / UV;
      lastTimeTillBurn *= 60;
      lastTimeTillBurn -= pastTimeAsInt;
    }
  }

  // this method checks if the button has been pressed and if it was the first press
  void checkButton()
  {
    bool currentState = digitalRead(BUTTON_PIN);
    // Detect if button is pushed
    if (lastButtonState == HIGH && currentState == LOW) {
      startCountDown = true;
      if(firstPressHappened == false)
      {
        firstPressHappened = true;
      }
    }
    // Update previous status
    lastButtonState = currentState;
  }

  /* this method is responsible for making the sunscreen timer and using all the methods above besides the checButton 
     to notify the person if they are burning */
  void sunscreenTimer(int UV)
  {
    // this calculates the first time how long it will take to burn
    if (startCountDown == true && UV > 0)
    {
      sunscreenCalculation(true);
      startCountDown = false;
    }
    // this calculates everything after the first press of the button
    else if(firstPressHappened == true && UV > 0)
    {
      sunscreenCalculation(false);
      currentTimeTillBurn = lastTimeTillBurn - 1; // minus 1 because of vTaskDelay, since its an int -0.5 wont work 
      lastTimeTillBurn = currentTimeTillBurn;
      pastTimeAsInt++;
      setUpdatedLCD();
      setBuzzer();
    }
    // this is a backup to check if the UV is to low to burn
    else if(UV <= 0)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("UV te laag");
    }
    // this sets the start lcd
    else
    {
      setInitialLCD();
    }
  }

  // this is for the button loop
  void buttonLoop()
  {
    checkButton();
  }
  
  // this is for the sunscreen loop
  void sunscreenLoop()
  {
    sunscreenTimer(UV); 
    Serial.print(currentTimeTillBurn);
    Serial.println( " seconden tot je verbrand");
  }

};

// initializing the classes
Temperature temperature;
Light light;
SunScreen sunscreen;

// this task calls the temperature loop
void tempTask(void *pvParameters) 
{
  for (;;) {
    temperature.tempLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // every second
  }
}

// this task calls the light loop
void lightTask(void *pvParameters) 
{
  for (;;) {
    light.lightLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // every second
  }
}

// this task calls the sunscreen loop
void sunTask(void *pvParameters) 
{
  for (;;) {
    sunscreen.sunscreenLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // every second
  }
}

// this task calls the button loop
void buttonTask(void *pvParameters) 
{
  for (;;) {
    sunscreen.buttonLoop();
    vTaskDelay(pdMS_TO_TICKS(100)); // every 10th of a second
  }
}

void setup() {
  Serial.begin(115200);
  // the setups of the classes
  temperature.tempSetup();
  light.lightSetup();
  sunscreen.sunscreenSetup();
  
  // initializing the tasks 
  xTaskCreatePinnedToCore(tempTask, "TempTask",  16000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(lightTask, "LightTask",  16000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sunTask, "SunTask",  4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(buttonTask, "ButtonTask",  4096, NULL, 1, NULL, 0);
}

void loop() {

}
