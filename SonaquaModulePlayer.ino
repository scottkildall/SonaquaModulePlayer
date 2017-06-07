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

//-- leave uncommented to see serial debug messages
#define SERIAL_DEBUG

//-- Conditional defines for testing hardware
//#define SPEAKER_ONLY            // just play ascending tones, to make sure speaker works, tests speakers
//#define SPEAKER_POT             // plays speaker according to potentiometer, tests pot

//-- PINS
#define speakerGndPin (10)        // set this to low so we can do direct soldering to Metro Mini
#define speakerPin (12)

#define PotPin (A0)
#define ECPower (A1)
#define ECPin (A5)   

//-- If raw EC is above this, we won't play the sounds
#define EC_SILENT_THRESHOLD (970)
#define DELAY_TIME (100)
#define MIN_TONE (25)
 
void setup() {
#ifdef SERIAL_DEBUG
  //-- no serial support for the Digispark
  Serial.begin(115200);
  Serial.println("startup");
#endif

  //-- pin inputs / outputs
  pinMode(ECPin,INPUT);
  pinMode(ECPower,OUTPUT);                // set pin for sourcing current
  pinMode(speakerPin, OUTPUT); 
  pinMode(speakerGndPin, OUTPUT);

  //-- speaker ground is always low
  digitalWrite(speakerGndPin,LOW);
}

//-- rawEC == 0 -> max conductivity; rawEC == 1023, no conductivity
void loop() {
  // Get inputs: EC and Pot Value
  unsigned int rawEC = getEC(); 
  int potValue = analogRead(PotPin);;
  
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
    delay(DELAY_TIME);

    #ifdef SERIAL_DEBUG       
      Serial.println("-----");
    #endif
    
    return;
  }

#ifdef SERIAL_DEBUG       
    Serial.print("rawEC = ");
    Serial.println(rawEC);
#endif

#ifdef SERIAL_DEBUG       
    Serial.print("Pot value = ");
    Serial.println(potValue);
#endif

  unsigned int toneValue =  MIN_TONE + rawEC;

  // Here, we can modulate the toneValue with something like this
   //(rawEC*4) - 600 - ( (500 - potValue)/5);
  
  
  if (toneValue < 0 )
    toneValue = MIN_TONE;

    // Handle less-than-zero, will overflow to large 8-bit (65535) number
    if( toneValue > 50000 )
      toneValue = MIN_TONE;

#ifdef SERIAL_DEBUG       
     Serial.print("Tone value = ");
     Serial.println(toneValue);
#endif 

    //-- emit some sort of tone based on EC
    tone(speakerPin, toneValue );
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
 
 return raw;
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

