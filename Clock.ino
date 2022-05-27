/*
 * 3D printed smart shelving with a giant hidden digital clock in the front edges of the shelves - DIY Machines

==========

More info and build instructions: https://www.youtube.com/watch?v=8E0SeycTzHw

3D printed parts can be downloaded from here: https://www.thingiverse.com/thing:4207524

You will need to install the Adafruit Neopixel library which can be found in the library manager.

This project also uses the handy DS3231 Simple library:- https://github.com/sleemanj/DS3231_Simple   Please follow the instruction on installing this provided on the libraries page

Before you install this code you need to set the time on your DS3231. Once you have connected it as shown in this project and have installed the DS3231_Simple library (see above) you
 to go to  'File' >> 'Examples' >> 'DS3231_Simple' >> 'Z1_TimeAndDate' >> 'SetDateTime' and follow the instructions in the example to set the date and time on your RTC

==========


 * SAY THANKS:

Buy me a coffee to say thanks: https://ko-fi.com/diymachines
Support us on Patreon: https://www.patreon.com/diymachines

SUBSCRIBE:
■ https://www.youtube.com/channel/UC3jc4X-kEq-dEDYhQ8QoYnQ?sub_confirmation=1

INSTAGRAM: https://www.instagram.com/diy_machines/?hl=en
FACEBOOK: https://www.facebook.com/diymachines/

*/




#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#endif

#include <DS3231_Simple.h>
DS3231_Simple Clock;

// Create a variable to hold the time data 
DateTime MyDateAndTime;

// Which pin on the Arduino is connected to the NeoPixels?
#define LEDCLOCK_PIN    6
#define LEDDOWNLIGHT_PIN    5

int ledPerSegment = 10;
// How many NeoPixels are attached to the Arduino?
#define LEDCLOCK_COUNT 230  // This should be = 23 * ledPerSegment
#define LEDDOWNLIGHT_COUNT 12

//(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
// or 3 hex byte 00 -> ff for RGB eg 0x123456 for red=12(hex) green=34(hex), and green=56(hex) 
// this hex method is the same as html colour codes just with "0x" instead of "#" in front
uint32_t clockMinuteColour ; // pure red 
uint32_t clockHourColour ;   // pure green

const int colourCount = 10;
//                                   Pure Red, Pure Green , Redish    , White     , Golden    , Green     , Golden        , Golden Green , Peach      , White              
uint32_t hourColors[colourCount] = { 0x800000, 0x008000   , 0xFC4445  , 0xFFFFFF  , 0xFCCD04  , 0x86C232  , 0xFAED26      , 0xA4A71E     , 0xF4976C   , 0xFFFFFF }; 
//                                 pure green, pure red   , Greenish  , Blue      , Blue      , Pink      , Greanish Blue , Pink         , Light Blue , White
uint32_t minColors[colourCount] =  { 0x008000, 0x800000   , 0x5CDB95  , 0x84CEEB  , 0x84CEEB  , 0xF172A1  , 0x3B945E      , 0xF79E02     , 0xD2FDFF   , 0xFFFFFF };

int clockFaceBrightness = 0;

// Declare our NeoPixel objects:
Adafruit_NeoPixel stripClock(LEDCLOCK_COUNT, LEDCLOCK_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDownlighter(LEDDOWNLIGHT_COUNT, LEDDOWNLIGHT_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


//Smoothing of the readings from the light sensor so it is not too twitchy
const int numReadings = 12;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
long total = 0;                  // the running total
long average = 0;                // the average


int hourPin = 10,
    minPin = 9,
    colorPin = 7,
    lightIntensityPin = 8,
    colorAddress = 0,
    lightIntensityAddress = 1;


int colorIndex = 0,
    brightnessFactor;

int debugCnt;

String debugMsg;



void setup() {
  // put your setup code here, to run once:
 
  pinMode(hourPin, INPUT_PULLUP);  //Set hours
  pinMode(minPin, INPUT_PULLUP);  //Set Mins
  pinMode(colorPin, INPUT_PULLUP);  //Set Color
  pinMode(lightIntensityPin, INPUT_PULLUP);  //Set Brightness
  
  Serial.begin(9600);
  Clock.begin();

  stripClock.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
  stripClock.show();            // Turn OFF all pixels ASAP
  stripClock.setBrightness(240); // Set inital BRIGHTNESS (max = 255)
 

  stripDownlighter.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
  stripDownlighter.show();            // Turn OFF all pixels ASAP
  stripDownlighter.setBrightness(50); // Set BRIGHTNESS (max = 255)

  //smoothing
    // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
 
  debugCnt=0;
  debugMsg = "";
  brightnessFactor = 2;

  //Read EEPROM for color and light intensity value
  colorIndex = EEPROM.read(colorAddress);
  if(colorIndex >= colourCount)
  {
    colorIndex = 0;  
  }
  Serial.println("Color Index from EEPROM: " + String(colorIndex));
  clockMinuteColour = minColors[colorIndex] ;
  clockHourColour =  hourColors[colorIndex] ;
  
  brightnessFactor = EEPROM.read(lightIntensityAddress);
  if(brightnessFactor > 5)
  {
    brightnessFactor = 0;  
  }
  Serial.println("brightness Factor from EEPROM: " + String(brightnessFactor));

  
}

void loop() {
  debugCnt++;
 
  
  //read the time
  readTheTime();

  //Set Hours
  if(digitalRead(hourPin) == LOW){
    MyDateAndTime.Hour = MyDateAndTime.Hour + 1;
    if(MyDateAndTime.Hour > 23){
      MyDateAndTime.Hour=0;
    }
    MyDateAndTime.Year = 22;
    Clock.write(MyDateAndTime);  
    delay(200);
  } 

  //Set Minutes
  if(digitalRead(minPin) == LOW){
    MyDateAndTime.Minute = MyDateAndTime.Minute + 1;
    if(MyDateAndTime.Minute > 59){
      MyDateAndTime.Minute=0;
    }
    MyDateAndTime.Second = 0;
    Clock.write(MyDateAndTime);  
    delay(200);
  }

  //Set Color
  if(digitalRead(colorPin) == LOW){
    colorIndex++;
    if(colorIndex >= colourCount){
      colorIndex = 0;
    }
    clockMinuteColour = minColors[colorIndex];
    clockHourColour =  hourColors[colorIndex];
    EEPROM.update(colorAddress, colorIndex);
    delay(200);
  }

  //Set Bightness
  if(digitalRead(lightIntensityPin) == LOW){
    brightnessFactor++;
    if(brightnessFactor > 5){
      brightnessFactor = 0;
    }
    EEPROM.update(lightIntensityAddress, brightnessFactor);
    delay(200);
  }
   

  //display the time on the LEDs
  displayTheTime();


  //Record a reading from the light sensor and add it to the array
  readings[readIndex] = analogRead(A0); //get an average light level from previouse set of samples
  //Serial.print("Light sensor value added to array = ");
  //Serial.println(readings[readIndex]);
  readIndex = readIndex + 1; // advance to the next position in the array:

  // if we're at the end of the array move the index back around...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  //now work out the sum of all the values in the array
  int sumBrightness = 0;
  for (int i=0; i < numReadings; i++)
    {
        sumBrightness += readings[i];
    }
  //Serial.print("Sum of the brightness array = ");
  //Serial.println(sumBrightness);

  // and calculate the average: 
  int lightSensorValue = sumBrightness / numReadings;
  //Serial.print("Average light sensor value = ");
  //Serial.println(lightSensorValue);


  //set the brightness based on ambiant light levels
  clockFaceBrightness = map(lightSensorValue,50, 1000, 25 + (brightnessFactor * 45) , 10 + (brightnessFactor * 0)); 
  stripClock.setBrightness(clockFaceBrightness); // Set brightness value of the LEDs
  Serial.println("Light Sensor value = " + String(lightSensorValue) + " | Brigtness Factor: " + String(brightnessFactor) + " | LED Brightness:"  + String(clockFaceBrightness));
  
  
  stripClock.show();
   
  stripDownlighter.fill(0xFFF1E6, 0, LEDDOWNLIGHT_COUNT);
  stripDownlighter.show();

  //delay(5000);   //this 5 second delay to slow things down during testing
  //delay(500);
}


void debug(String val){
  if(debugCnt>100){
    Serial.println(val);
    debugCnt = 0;
  }
  else{
    debugMsg = debugMsg + "\n" + val;
  }
}


void readTheTime(){
  // Ask the clock for the data.
  MyDateAndTime = Clock.read();
  
  //String date = String(MyDateAndTime.Year) + "/" + String(MyDateAndTime.Month) + "/" + String(MyDateAndTime.Day);
  //String timeV = String(MyDateAndTime.Hour) + ":" + String(MyDateAndTime.Minute) + ":" + String(MyDateAndTime.Second);
  //debug("DateTime is: " + date + " " + timeV + " | Color Index: " + String(colorIndex)); 
}
  

void displayTheTime(){
  stripClock.clear(); //clear the clock face 
  
  int firstMinuteDigit = MyDateAndTime.Minute % 10; //work out the value of the first digit and then display it
  displayNumber(firstMinuteDigit, 0, clockMinuteColour);
  
  int secondMinuteDigit = floor(MyDateAndTime.Minute / 10); //work out the value for the second digit and then display it
  displayNumber(secondMinuteDigit, ledPerSegment * 7, clockMinuteColour);  

  int firstHourDigit = MyDateAndTime.Hour; //work out the value for the third digit and then display it
  if(firstHourDigit == 0) {
    firstHourDigit = 12;
  }
  if (firstHourDigit > 12){
    firstHourDigit = firstHourDigit - 12;
  }
 
  firstHourDigit = firstHourDigit % 10;
  displayNumber(firstHourDigit, ledPerSegment * 14, clockHourColour);


  int secondHourDigit = MyDateAndTime.Hour; //work out the value for the fourth digit and then display it
  if (secondHourDigit == 0){
    secondHourDigit = 12;
  }
 if (secondHourDigit > 12){
    secondHourDigit = secondHourDigit - 12;
  }
    if (secondHourDigit > 9){
      stripClock.fill(clockHourColour,ledPerSegment * 21, ledPerSegment * 2); 
    }
  }


void displayNumber(int digitToDisplay, int offsetBy, uint32_t colourToUse){
    switch (digitToDisplay){
    case 0:
    digitZero(offsetBy,colourToUse, ledPerSegment);
      break;
    case 1:
      digitOne(offsetBy,colourToUse, ledPerSegment);
      break;
    case 2:
    digitTwo(offsetBy,colourToUse, ledPerSegment);
      break;
    case 3:
    digitThree(offsetBy,colourToUse, ledPerSegment);
      break;
    case 4:
    digitFour(offsetBy,colourToUse, ledPerSegment);
      break;
    case 5:
    digitFive(offsetBy,colourToUse, ledPerSegment);
      break;
    case 6:
    digitSix(offsetBy,colourToUse, ledPerSegment);
      break;
    case 7:
    digitSeven(offsetBy,colourToUse, ledPerSegment);
      break;
    case 8:
    digitEight(offsetBy,colourToUse, ledPerSegment);
      break;
    case 9:
    digitNine(offsetBy,colourToUse, ledPerSegment);
      break;
    default:
     break;
  }
}

void digitZero(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, (0 + offset), ledPerLine * 3);
    stripClock.fill(colour, (ledPerLine * 4 + offset), ledPerLine * 3);
}

void digitOne(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour,  offset, ledPerLine);
    stripClock.fill(colour, (ledPerLine * 4 + offset), ledPerLine);
}

void digitTwo(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, (0 + offset), ledPerLine * 2);
    stripClock.fill(colour, (ledPerLine * 3 + offset), ledPerLine);
    stripClock.fill(colour, (ledPerLine * 5 + offset), ledPerLine * 2);
}

void digitThree(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, offset, ledPerLine * 2);
    stripClock.fill(colour, (ledPerLine* 3 + offset), ledPerLine * 3);
}

void digitFour(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, offset, ledPerLine);
    stripClock.fill(colour, (ledPerLine * 2 + offset), ledPerLine * 3);
}

void digitFive(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, (ledPerLine + offset), ledPerLine * 5);
}

void digitSix(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, (ledPerLine + offset), ledPerLine * 6);
}

void digitSeven(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, offset, ledPerLine * 2);
    stripClock.fill(colour, (ledPerLine * 4 + offset), ledPerLine);
}

void digitEight(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, offset, ledPerLine * 7);
}

void digitNine(int offset, uint32_t colour, int ledPerLine){
    stripClock.fill(colour, offset, ledPerLine * 6);
}
