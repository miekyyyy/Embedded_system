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
bool UVLimiter = true;

class Temperature
{
  public: 
  DHT_Unified dht;
  Adafruit_NeoPixel pixels;
 
  Temperature() : dht(DHTPIN, DHTTYPE), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}


  void tempSetup()
  {
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
      else if(i < 5 && i >= 2) color = pixels.Color(150, 150, 0); // orange like
      else if(i < 7 && i >= 5) color = pixels.Color(255, 0, 0); // red
      else                     color = pixels.Color(255, 0, 255); // purple
      pixels.setPixelColor(i, color);
    }
    pixels.show();
  }

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
  Adafruit_TSL2561_Unified tsl;

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

  void lightLoop()
  {
    /* Get a new sensor event */ 
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

class SunScreen
{
  // For calculations we are gonna assume this person has skintype I
  // For calculations we are gonna assume this person has sunscreen with SPF 30
  // For calculations we are gonna assume this person starts walking outside right when they applied their first layer of sunscreen
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


  SunScreen() : lcd(0x27,16, 2) {}

  void setBuzzer()
  {
    if (currentTimeTillBurn <= 0)
    {
      tone(BUZZER, 1000); // Send 1KHz sound signal...
      vTaskDelay(pdMS_TO_TICKS(1000));
      noTone(BUZZER); 
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
      noTone(BUZZER);  // Stop sound...
    }
  }

  void setInitialLCD()
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start doormiddel");
    lcd.setCursor(0, 1);
    lcd.print("van knop");
  }

  void setUpdatedLCD()
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentTimeTillBurn); lcd.print(" seconden");
    lcd.setCursor(0, 1);
    lcd.print("tot je verbrand");
  }

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
  
  void sunscreenCalculation(bool appliedSunscreen)
  {
    if(appliedSunscreen)
    {
      lastTimeTillBurn = (30 * 67) / UV;
      lastTimeTillBurn *= 60;
      pastTimeAsInt = 0;
    }
    else
    {
      lastTimeTillBurn = (30 * 67) / UV;
      lastTimeTillBurn *= 60;
      lastTimeTillBurn -= pastTimeAsInt;
    }
  }

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
     // Serial.println("Button check");
    }
    // Update vorige status
    lastButtonState = currentState;
  }

  void sunscreenTimer(int UV)
  {
    if (startCountDown == true && UV > 0)
    {
      sunscreenCalculation(true);
      startCountDown = false;
    }
    else if(firstPressHappened == true && UV > 0)
    {
      sunscreenCalculation(false);
      currentTimeTillBurn = lastTimeTillBurn - 1; // min getal waardoor die om de minuut 1 naar beneden gaat;
      lastTimeTillBurn = currentTimeTillBurn;
      pastTimeAsInt++;
      setUpdatedLCD();
      setBuzzer();
    }
    else if(UV <= 0)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("UV te laag");
    }
    else
    {
      setInitialLCD();
    }
  }

  void buttonLoop()
  {
    checkButton();
  }
  void sunscreenLoop()
  {
    sunscreenTimer(UV); 
    Serial.print(currentTimeTillBurn);
    Serial.println( " seconden tot je verbrand");
  }

};

Temperature temperature;
Light light;
SunScreen sunscreen;

void tempTask(void *pvParameters) 
{
  for (;;) {
    temperature.tempLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // elke seconde
  }
}

void lightTask(void *pvParameters) 
{
  for (;;) {
    light.lightLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // elke seconde
  }
}

void sunTask(void *pvParameters) 
{
  for (;;) {
    sunscreen.sunscreenLoop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // elke seconde
  }
}

void buttonTask(void *pvParameters) 
{
  for (;;) {
    sunscreen.buttonLoop();
    vTaskDelay(pdMS_TO_TICKS(100)); // elke seconde
  }
}

void setup() {
  Serial.begin(115200);
  //put your setup code here, to run once:
  temperature.tempSetup();
  light.lightSetup();
  sunscreen.sunscreenSetup();
  
  xTaskCreatePinnedToCore(tempTask, "TempTask",  16000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(lightTask, "LightTask",  16000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sunTask, "SunTask",  4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(buttonTask, "ButtonTask",  4096, NULL, 1, NULL, 0);
}

void loop() {

}
