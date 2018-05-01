#include "FastLED.h"
#define NUM_LEDS 170
#define DATA_PIN 9
int buttonPin = 5;
int buttonState = 0;

bool secondMonitorEnabled = true;
long lastDebounceTime = 0;
long debounceDelay = 200;

// Brightness control
#define maxBright 255       // максимальная яркость (0 - 255)
#define minBright 50       // минимальная яркость (0 - 255)
#define brightConstant 800  // константа усиления от внешнего света (0 - 1023)
#define coef 0.9             // коэффициент фильтра (0.0 - 1.0), чем больше - тем медленнее меняется яркость
int newBright, newBrightLeds;

unsigned long brightTimer;

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
#define serialRate 115200

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

// Initialise LED-array
CRGB leds[NUM_LEDS];

void setup() {
  // Use NEOPIXEL to keep true colors
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  
  // Initial RGB flash
  LEDS.showColor(CRGB(20, 0, 0));
  delay(200);
  LEDS.showColor(CRGB(0, 20, 0));
  delay(200);
  LEDS.showColor(CRGB(0, 0, 20));
  delay(200);
  LEDS.showColor(CRGB(0, 0, 0));
  
  pinMode(buttonPin, INPUT);

  Serial.begin(serialRate);
  // Send "Magic Word" string to host
  Serial.print("Ada\n");
}

void loop() { 
  buttonState = digitalRead(buttonPin);

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if ((buttonState == HIGH) && (!secondMonitorEnabled)) {
      secondMonitorEnabled = !secondMonitorEnabled;
      lastDebounceTime = millis();
    } else if ((buttonState == HIGH) && (secondMonitorEnabled)) {
      secondMonitorEnabled = !secondMonitorEnabled;
      lastDebounceTime = millis();
    }
  }
  // Wait for first byte of Magic Word
  for(i = 0; i < sizeof prefix; ++i) {
    waitLoop: while (!Serial.available()) ;;
    // Check next byte in Magic Word
    if(prefix[i] == Serial.read()) continue;
    // otherwise, start over
    i = 0;
    goto waitLoop;
  }
  
  // Hi, Lo, Checksum  
  while (!Serial.available()) ;;
  hi=Serial.read();
  while (!Serial.available()) ;;
  lo=Serial.read();
  while (!Serial.available()) ;;
  chk=Serial.read();
  
  // If checksum does not match go back to wait
  if (chk != (hi ^ lo ^ 0x55)) {
    i=0;
    goto waitLoop;
  }

  // Adjust brightness
  if (millis() - brightTimer > 100) {
    brightTimer = millis();
    newBright = map(analogRead(3), 0, brightConstant, minBright, maxBright);
    newBright = constrain(newBright, minBright, maxBright);
    newBrightLeds = newBrightLeds * coef + newBright * (1 - coef);
    LEDS.setBrightness(newBrightLeds);
  }
  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));

  // Read the transmission data and set LED values
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    byte r, g, b;    
    while(!Serial.available());
    r = Serial.read();
    while(!Serial.available());
    g = Serial.read();
    while(!Serial.available());
    b = Serial.read();
    
    // skip second monitor
    if (i > 90 && i <= 160) {
      for (int j = 0; j <= 69; j++) {
        if (secondMonitorEnabled) {
          leds[i].r = 55;
          leds[i].g = 55;
          leds[i].b = 50;
          i++;
        };
      }
      continue;
    }
    // dimming second monitor
    if (i > 82 | i < 3) {
      r = r / 2;
      g = g / 2;
      b = b / 2;
    }
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
    
  }
  
  // Shows new values
  FastLED.show();
}