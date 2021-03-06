/*
 *  SoundSensor
 *  by Scott Kildall
 *  www.kildall.com
 *  
 *  Sonaqua: EC Water Sensor to play sounds
 *  
 *  Designed for Arduino Uno / Metro Mini, but will work on other platforms
 *  
 *  Adapted from this Hackaday project
 *  https://hackaday.io/project/7008-fly-wars-a-hackers-solution-to-world-hunger/log/24646-three-dollar-ec-ppm-meter-arduino
 *  
 */

#include "Adafruit_LEDBackpack.h"
#include "MSTimer.h"
#include <Adafruit_NeoPixel.h>

//#define SOLO_MODULE
//-- leave uncommented to see serial debug messages
//#define SERIAL_DEBUG

//-- Conditional defines for testing hardware
//#define SPEAKER_ONLY            // just play ascending tones, to make sure speaker works, tests speakers
//  #define SPEAKER_POT             // plays speaker according to potentiometer, tests pot

//-- TYPE OF FLUID

#define noneSpecified   (0)
#define blood           (1)
#define coconutWater    (2)
#define coffee          (3)
#define breastMilk      (4)
#define cocaCola        (5)
#define holyWater       (6)
#define wine            (7)
#define gasoline        (8)
#define kombucha        (9)
#define bourbon         (10)

//-- SET FLUID TYPE HERE, use when compiling to each module
int fluidType = noneSpecified;    // default


//-- PINS
#define speakerPin (6)
#define waterLEDPin (10)
#define SwitchPin (8)
#define neoPixelPin (11)


#define PotPin (A0)
#define ECPower (A2)
#define ECPin (A1)   

#define LEDPin (9)

//-- NeoPixel colors
int r;
int b;
int g;

unsigned int toneValue;

//-- If raw EC is above this, we won't play the sounds
#define EC_SILENT_THRESHOLD (970)
#define DELAY_TIME (100)
#define MIN_TONE (31)   // lowest possible tone for Arduinos
#define MAX_TONE (680)    // otherwise, is annoying

Adafruit_7segment matrix = Adafruit_7segment();
MSTimer displayTimer = MSTimer();

//-- if this is set to TRUE, then we look at a pin for a digital input (from another Arduino), which acts like a swetch
//-- OR this can be just a switch to activate
boolean bUseSwitch = true;
boolean bLightsOn = false;

// Instatiate the NeoPixel from the ibrary
#define numPixels (16)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, neoPixelPin, NEO_GRB + NEO_KHZ800);

// EC 

// for tone-testing
//#define TEST_EC_VALUES
#define numECValues (8)
unsigned int ecValues [] = { 163, 168, 247, 302, 366, 408, 422, 544 };


void setup() {
  randomSeed(A0);
  
   //-- pin inputs / outputs
  pinMode(ECPin,INPUT);
  pinMode(ECPower,OUTPUT);                // set pin for sourcing current
  pinMode(speakerPin, OUTPUT); 
  pinMode(LEDPin,OUTPUT);                // set pin for sourcing current
  pinMode(waterLEDPin,OUTPUT);
  
  pinMode(SwitchPin, INPUT);


  // Flash LED
  for(int i = 0; i < 6; i++ ) {
    digitalWrite(LEDPin,HIGH);
    delay(100);
    digitalWrite(LEDPin,LOW);
    delay(100);
  }
  digitalWrite(LEDPin,HIGH);
  
  matrix.begin(0x70);
  matrix.print(9999, DEC);
  matrix.writeDisplay();
  
#ifdef SERIAL_DEBUG
  //-- no serial support for the Digispark
  Serial.begin(115200);
  Serial.println("startup");
#endif

 
  //-- speaker ground is always low
  //digitalWrite(speakerGndPin,LOW);

  // every 1000ms we will update the display, for readability
  displayTimer.setTimer(1000);
  displayTimer.start();
   
   matrix.begin(0x70);
 
  matrix.print(7777, DEC);
  matrix.writeDisplay();

  setNeoPixelColors();

  strip.begin();  // initialize the strip
  lightsOn();
  delay(1000);
  lightsOff();
  
  digitalWrite(waterLEDPin, LOW);
}

//-- rawEC == 0 -> max conductivity; rawEC == 1023, no conductivity
void loop() { 

#ifdef TEST_EC_VALUES
  while(true) {
    for( int i = 0; i < numECValues; i++ ) {
      
      toneValue = getToneValueFromEC(ecValues[i]);
      
      matrix.print(ecValues[i], DEC);
      matrix.writeDisplay();
      
      tone(speakerPin, toneValue );
      delay(2000);    
      matrix.print(toneValue, DEC);
      matrix.writeDisplay();
      delay(1000);
    }
  }

#endif

#ifdef SOLO_MODULE
  boolean bSwitchOn = true;
#else
  boolean bSwitchOn = digitalRead(SwitchPin);
  if( bUseSwitch == false )
    bSwitchOn = true;
#endif
    
  // Get inputs: EC and Pot Value
  unsigned int rawEC = getEC(); 
  int potValue = analogRead(PotPin);;

  if( displayTimer.isExpired() ) {
    //-- quick test
   // matrix.print(bSwitchOn, DEC);
    matrix.print(rawEC, DEC);
    matrix.writeDisplay();
    displayTimer.start();
  }



#ifdef SPEAKER_ONLY
  speakerOnlyTest();
  return;
#endif

#ifdef SPEAKER_POT
  speakerPotTest(potValue);
  return;
#endif

  // Only activate if we below the EC Threshold
  if( rawEC > EC_SILENT_THRESHOLD ) { 
    noTone(speakerPin);
    digitalWrite(waterLEDPin, LOW);
    
    lightsOff();
    delay(DELAY_TIME);

    #ifdef SERIAL_DEBUG       
      Serial.println("-----");
    #endif
    
    return;
  }

  //-- clean this up
  boolean bPlaySound = true;
  if( bUseSwitch == true )
    bPlaySound = digitalRead(SwitchPin);
    
  
#ifdef SERIAL_DEBUG       
    Serial.print("rawEC = ");
    Serial.println(rawEC);
#endif

#ifdef SERIAL_DEBUG       
    Serial.print("Pot value = ");
    Serial.println(potValue);
#endif

  toneValue = getToneValueFromEC(rawEC);
 
  

    // deacivate pot
     // toneValue += (potValue/5);

#ifdef SERIAL_DEBUG       
     Serial.print("Tone value = ");
     Serial.println(toneValue);
#endif 



    //-- emit some sort of tone based on EC
    if( bSwitchOn ) {
      lightsOn();
      tone(speakerPin, toneValue );
      digitalWrite(waterLEDPin, HIGH);
    }
    else {
      noTone(speakerPin);
      digitalWrite(waterLEDPin, LOW);
       lightsOff();
    }
     
    delay(DELAY_TIME);    
}

//-- Sample EC using Analog pins, returns 0-1023
unsigned int getEC(){
  unsigned int raw;
 
  digitalWrite(ECPower,HIGH);

  // This is not a mistake, First reading will be low beause of charged a capacitor
  raw= analogRead(ECPin);
  raw= analogRead(ECPin);   
  
  digitalWrite(ECPower,LOW);

  //-- special-cases, floating EQ modulation
  if( fluidType == gasoline )
    return random(605,613);
  
  if( fluidType == bourbon )
    return random(500,508);

  
 return raw;
}

// expand the range of the tone value by doubling the rawEC and doing some various math to it
// this works for a sampling range where minumim EC < 400
unsigned int getToneValueFromEC(unsigned int rawEC) {
  //-- Cybernetic spirits
  unsigned int toneValue =  MIN_TONE + (rawEC) - 100;

  //-- old, water sample
  //unsigned int toneValue =  MIN_TONE + (rawEC * 2) - 800;

  if (toneValue < 0 )
    toneValue = MIN_TONE;
  else if( toneValue > MAX_TONE )
    toneValue = MAX_TONE;

   // Handle less-than-zero, will overflow to large 8-bit (65535) number
    if( toneValue > 50000 )
      toneValue = MIN_TONE;

      
  int randRange = 8;
  return toneValue + random(randRange+1) - randRange/2;
}

//-- depending on the type of fluid we are using, we have different neoPixelColors, default is pure white
void setNeoPixelColors() { 
  switch(fluidType) {
    case noneSpecified:
      r = 255; g = 0; b = 0;
      break;

    case blood:
      r = 255; g = 0; b = 35;
      break;

    case coconutWater:
      r = 0; g = 255; b = 72;
      break;
      
    case wine:
      r = 255; g = 0; b = 255;
      break;

    case coffee:
      r = 255; g = 64; b = 128;
      break;

    case holyWater:
      r = 3; g = 104; b = 215;
      break;

    case bourbon:
      r = 255; g = 37; b = 37;
      break;

     case breastMilk:
      r = 255; g = 255; b = 255;
      break;

    case cocaCola:
       r = 255; g = 0; b = 128;
       break;
       
    case gasoline:
      r = 255; g = 35; b = 0;
      break;

    case kombucha:
      r = 239; g = 201; b = 7;
      break;
      
    default:
      r = 255; g = 255; b = 255;
      break;
  }
}

//-- turn on the neopixel ring when we are activating the circuit
void lightsOn() {
  if( bLightsOn )
    return;
    
  // set the colors for the strip
   for( int i = 0; i < numPixels; i++ )
       strip.setPixelColor(i, r,g,b);
   
   // show all pixels  
   strip.show();
   bLightsOn = true;
}


void lightsOff() {
  if( !bLightsOn )
    return;
    
  strip.clear();  // Initialize all pixels to 'off
  strip.show();
  bLightsOn = false;
}


void speakerOnlyTest() {
  for( int i = 100; i < 600; i += 10 ) {
    tone( speakerPin, i );
    delay(100);
  }
  
  return;
}

void speakerPotTest(int potValue) {
#ifdef SERIAL_DEBUG  
  Serial.println(potValue);
#endif

  tone( speakerPin, 100 + (potValue/4) );
  delay(100);
}

