//#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define USE_OCTOWS2811
#include <OctoWS2811.h>
#include <FastLED.h>
#include <avr/pgmspace.h>  // Needed to store stuff in Flash using PROGMEM

FASTLED_USING_NAMESPACE

// Heavily modified by Greig. Based on:
//
// FastLED "100-lines-of-code" demo reel
// XY Example
// Some patterns stolen from https://github.com/evilgeniuslabs/torch/blob/master/torch.ino
// https://gist.github.com/StefanPetrick/c856b6d681ec3122e5551403aabfcc68
// https://bitbucket.org/ratkins/ledeffects/src/default/SinzianaPanel.ino
// ... and with the magic speed fix courtesy of https://www.reddit.com/user/Jem_Spencer
// Others are credited in the code:


#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif


/////////////////////////////////////////////////////////
// Pattern management / mode
// Note these values are overridden by the DMX mode switch
/////////////////////////////////////////////////////////
#define Manual 0;
#define Random 1;
#define Sequential 2;
uint8_t PATTERN_MODE = 0; // Options: Manual, Random, Sequential
uint8_t gCurrentPatternNumber = 30; // Index number of which pattern is currently 'playing'
uint8_t gStoredPatternNumber =  0;  // Previous value, before the DMX switch was toggled

// 0  = All OFF. BLACK / DBO
// 1  = SPARE
// 2  = random individual LEDs. No pattern of which to speak.
// 3  = 'sinelon'. NEEDS WORK
// 4  = crazy many-led chase pattern ('juggle')
// 5  = coloured vertical bars chase around the ball
// 6  = TEST - my basic snake
// 7  = AWESOME colour wave. (original xy demo pattern)
// 8  = Spare
// 9  = crossoverChase. A single led sweeps down from the top and back up the other side, then +1 x and continues
// 10 = multi-coloured column (all lit) chases around the ball 
// 11 = horizontal bounce. multi-coloured row (all lit) pulses from top to bottom down the ball 
// 12 = faulty
// 13 = Stefan's noise pattern
// 14 = plasma - awesome, but a little like 7?
// 15 = 'applause' - sparse, purply sparkles
// 16 = huecycle - the whole ball is the SAME colour, changing through the rainbow
// 17 = 'pride'
// 18 = fire2012WithPalette
// 19 = wave
// 20 = matrixEffect.ino - kinda coloured snowfall
// 21 = test pattern. Static corkscrew
// 22 = simple rotating corkscrew
// 23 = my corkscrew, with trailing bits
// 24 = reverse trailing corkscrew
// 25 = doSnake
// 26 = Pac-Man
// 27 = all on, slow pulsing rainbow
// 28 = same, but with sparkles
// 29 = spare
// 30 = spare
// 31 = spare

int corkScrew_x = 0; //Used for the corkscrew pattern to pass back to itself per pass through loop
int worm_x = 0; // unused
int worm_y = 0; // unused

CHSV ch9Colour1 = CHSV( random8(), 255, 255);
CHSV ch9Colour2 = CHSV( random8(), 255, 255);
int ch9lastX = 0;

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 96
#define FRAMES_PER_SECOND  30


/////////////////////////////////////////////////////////
// DMX Mode
/////////////////////////////////////////////////////////

#define DMXModeSelect   24  //High for DMX mode, Low for Manual (code controlled mode)

#define DMXPattern0     25 //LSB 
#define DMXPattern1     26 //
#define DMXPattern2     27 //
#define DMXPattern3     28 //
#define DMXPattern4     29 //MSB
#define DMXSpeed0       30 //LSB
#define DMXSpeed1       31 //
#define DMXSpeed2       32 //MSB
#define DMXBrightness0  33 // 
#define DMXBrightness1  34 //
#define DMXBrightness2  35 //

bool lastDmxMode = false; //Stores the last state change of the DMX mode switch
uint8_t MASTER_BRIGHTNESS = BRIGHTNESS;


/////////////////////////////////////////////////////////
// Colour code, from Jason's "torch"
/////////////////////////////////////////////////////////

#include "GradientPalettes.h"
CRGB solidColor = CRGB::White;

typedef uint16_t(*PatternFunctionPointer)();
typedef PatternFunctionPointer PatternList [];
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

int currentPatternIndex = 0;
PatternFunctionPointer currentPattern;

CRGB w(85, 85, 85), W(CRGB::White);
CRGBPalette16 snowColors = CRGBPalette16( W, W, W, W, w, w, w, w, w, w, w, w, w, w, w, w );

CRGB l(0xE1A024);
CRGBPalette16 incandescentColors = CRGBPalette16( l, l, l, l, l, l, l, l, l, l, l, l, l, l, l, l );

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  OceanColors_p,
  CloudColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p,
  LavaColors_p,
  snowColors,
};

const int paletteCount = ARRAY_SIZE(palettes);

int currentPaletteIndex = 0;
CRGBPalette16 palette = palettes[0];


/////////////////////////////////////////////////////////
// Setup the ball for XY/array operation
/////////////////////////////////////////////////////////

// Params for width (columns) and height (LEDs per column)
const uint16_t kMatrixWidth  = 64;  // Test ball = 16, Death Star = 64
const uint16_t kMatrixHeight = 45;  // Test ball = 16, Death Star = 45

#define NUM_LEDS_PER_STRIP 360 // Test ball = 32, Death Star = 360
#define NUM_STRIPS 8           // 8

const uint16_t NUM_LEDS (kMatrixWidth * kMatrixHeight);

uint8_t CentreX = (kMatrixWidth  / 2) - 1;
uint8_t CentreY = (kMatrixHeight / 2) - 1;

CRGB leds[ NUM_LEDS ];

/////////////////////////////////////////////////////////
// Stefan's noise pattern
/////////////////////////////////////////////////////////
DEFINE_GRADIENT_PALETTE( pit ) {
    0,   3,   3,   3,
   64,  13,  13, 255,  //blue
  128,   3,   3,   3,
  192, 255, 130,   3,  //orange
  255,   3,   3,   3
};

// parameters and buffer for the noise array
#define NUM_LAYERS 1
uint32_t x[NUM_LAYERS];
uint32_t y[NUM_LAYERS];
uint32_t z[NUM_LAYERS];
uint32_t scale_x[NUM_LAYERS];
uint32_t scale_y[NUM_LAYERS];
uint8_t  noise[1][16][16];

// cheap correction with gamma 2.0
void adjust_gamma()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);
  }
}


/////////////////////////////////////////////////////////
// XY Serpentine correction
/////////////////////////////////////////////////////////
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
 
  if( x & 0x01)
  {
    // Odd columns run bottom to top
    uint8_t reverseY = (kMatrixHeight - 1) - y;
    i = (x * kMatrixHeight) + reverseY;
  }
  else
  {
    // Even columns run top to bottom
    i = (x * kMatrixHeight) + y;
  }
  return i;
}


void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  byte lineStartHue = startHue8;
  for( byte y = 0; y < kMatrixHeight; y++)
  {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;      
    for( byte x = 0; x < kMatrixWidth; x++)
    {
      pixelHue += xHueDelta8;
      leds[ XY(x, y)]  = CHSV( pixelHue, 255, 255);
    }
  }
}

#include "Fire2012WithPalette.h"
#include "Wave.h"
#include "Snake.cpp"
#include "Pac-Man.h"
#include "Corkscrews.h"


uint8_t GetDmxPattern()
{
  uint8_t NewPattern = 0;
  if (digitalRead(DMXPattern4)) { NewPattern += 16; }
  if (digitalRead(DMXPattern3)) { NewPattern +=  8; }
  if (digitalRead(DMXPattern2)) { NewPattern +=  4; }
  if (digitalRead(DMXPattern1)) { NewPattern +=  2; }
  if (digitalRead(DMXPattern0)) { NewPattern +=  1; }
  Serial.println("DMX pattern is " + String(NewPattern));
  return NewPattern;
}

uint8_t DMXBrightnessArray[8] = 
{
10,
40,
70,
110,
140,
180,
210,
255
};

uint8_t GetDmxBrightness()
{
  uint8_t NewBrightness = 0;
  if (digitalRead(DMXBrightness2)) { NewBrightness +=  4; }
  if (digitalRead(DMXBrightness1)) { NewBrightness +=  2; }
  if (digitalRead(DMXBrightness0)) { NewBrightness +=  1; }
  Serial.println("DMX Brightness is " + String(DMXBrightnessArray[NewBrightness]));
  return DMXBrightnessArray[NewBrightness];
}

void setup()
{
  Serial.begin(9600);
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);

  //Setup DMX mode input pins. All use internal pull-down resistors, except pin 24 which is physical
  pinMode(DMXModeSelect, INPUT);
  pinMode (DMXPattern0, INPUT_PULLDOWN);
  pinMode (DMXPattern1, INPUT_PULLDOWN);
  pinMode (DMXPattern2, INPUT_PULLDOWN);
  pinMode (DMXPattern3, INPUT_PULLDOWN);
  pinMode (DMXPattern4, INPUT_PULLDOWN);
  pinMode (DMXSpeed0, INPUT_PULLDOWN);
  pinMode (DMXSpeed1, INPUT_PULLDOWN);
  pinMode (DMXSpeed2, INPUT_PULLDOWN);
  pinMode (DMXBrightness0, INPUT_PULLDOWN);
  pinMode (DMXBrightness1, INPUT_PULLDOWN);
  pinMode (DMXBrightness2, INPUT_PULLDOWN);

  //Set the inital value of the DMX mode switch:
  if (digitalRead(DMXModeSelect))
  {
    lastDmxMode = true;
    gCurrentPatternNumber = GetDmxPattern();
    MASTER_BRIGHTNESS = GetDmxBrightness();
  }
  else
  {
    lastDmxMode = false;
    gStoredPatternNumber = gCurrentPatternNumber;
    MASTER_BRIGHTNESS = BRIGHTNESS;
  }

  // set master brightness control
  LEDS.setBrightness(MASTER_BRIGHTNESS);
}

uint16_t gCurrentPatternDelay;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { black, spare1, confetti, sinelon, 
    juggle, bpm, basicSnake, xyDemo, crossoverChase, pattern3, pattern4, horizontalBounce, 
    pattern6, noise_noise1, plasma, applause, huecycle, pride, fire2012WithPalette, 
    wave, matrixEffect, testPattern, simpleCorkscrew, trailingCorkscrew, reverseTrailingCorkscrew,
    doSnake, PacMan, rainbow, rainbowWithGlitter, spare29, spare30, spare31};

uint16_t PatternDelay[ARRAY_SIZE(gPatterns)] =
{
40, // 0  = BLACK - DBO
40, // 1  = SPARE1
40, // 2  = random individual LEDs. No pattern of which to speak.
40, // 3  = 'sinelon'. NEEDS WORK
40, // 4  = crazy many-led chase pattern ('juggle')
40, // 5  = coloured vertical bars chase around the ball. might be ok?
20, // 6  = TEST - my basic snake
20, // 7  = AWESOME colour wave. (original xy demo pattern)
20, // 8  = a single vertical column of RED leds chases around the ball
30, // 9  = Single led sweeps down from the top and back up the other side, then +1 x and continues
40, // 10 = multi-coloured column (all lit) chases around the ball 
40, // 11 = multi-coloured row (all lit) pulses from top to bottom down the ball 
20, // 12 = faulty
90, // 13 = Stefan's noise pattern
20, // 14 = plasma - awesome, but a little like 7?
90, // 15 = 'applause' - sparse, purply sparkles
90, // 16 = huecycle - the whole ball is the SAME colour, changing through the rainbow
300,// 17 = 'pride'
120,// 18 = fire2012WithPalette - OK
60, // 19 = wave - GREAT
300,// 20 = matrixEffect.ino - kinda coloured snowfall
20, // 21 = Test pattern. Static multi-coloured corkscrew (for IO pin rotation/alignment)
30, // 22 = multi-coloured rotating corkscrew
60, // 23 = my corkscrew, with trailing bits
60, // 24 = reverse corkscrew
2,  // 25 = doSnake
30, // 26 = Pac-Man
40, // 27  = rainbow
40, // 28 = rainbowWithGlitter
20, // 29 = spare
20, // 30 = spare
20  // 31 = spare
};

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  bool nowDmxMode;
  
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  LEDS.show();  
  // insert a delay to keep the framerate modest
  LEDS.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  // Check the DMX mode and settings:
  EVERY_N_MILLISECONDS( 20 )
  {
    nowDmxMode = digitalRead(DMXModeSelect);
    if (nowDmxMode != lastDmxMode)
    {
      lastDmxMode = nowDmxMode;
      if (nowDmxMode)
      {
        Serial.println("Yes, now in DMX mode.");
        gStoredPatternNumber = gCurrentPatternNumber; //Save the last pattern we were showing
      }
      else
      {
        Serial.println("Dropped out of DMX mode.");
        gCurrentPatternNumber = gStoredPatternNumber; //Reinstate the old stored pattern
        MASTER_BRIGHTNESS = BRIGHTNESS; //Reinstate the hard-coded default
        LEDS.setBrightness(MASTER_BRIGHTNESS);
      }
    }
    if (nowDmxMode)
    {
      gCurrentPatternNumber = GetDmxPattern();
      MASTER_BRIGHTNESS = GetDmxBrightness();
      LEDS.setBrightness(MASTER_BRIGHTNESS);
    }
  }


  // https://github.com/marmilicious/FastLED_examples/blob/master/every_n_timer_variable.ino
  // change patterns periodically - based on the chosen pattern:
  EVERY_N_SECONDS_I( timingObj, 20)
  {
    // This initally defaults to 20 seconds, but then will change the run
    // period to a new random number of seconds from 10 and 30 seconds.
    // You can name "timingObj" whatever you want.
    nextPattern();
    timingObj.setPeriod( random8(15,PatternDelay[gCurrentPatternNumber]) );
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  if (digitalRead(DMXModeSelect))
  {
    // We're in DMX mode. Do nothing here.
  }
  else
  {
    switch (PATTERN_MODE)
    {
      case 0 :
        // Manual. Do nothing, the pattern number will not change.
        return;
        break;
      case 1 :
        // Random. Choose another pattern at random - possibly even the one that's currently running    
          gCurrentPatternNumber = random8(ARRAY_SIZE(gPatterns)-1);
        break;
      case 2 :
        // Sequential.
        gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
        break;        
    }
    Serial.println("Next pattern is " + String(gCurrentPatternNumber));
  }
}

// 0
void black() 
{
  // This pattern is only an option when in Manual or DMX Modes:
  if ((PATTERN_MODE != 0) || (lastDmxMode == false)) { nextPattern(); return; }
  
  // Turn the ball off!
  fill_solid( leds, NUM_LEDS, CRGB::Black);
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 1
void spare1() 
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
}

// 2
void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 3
void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( gHue, 255, 192);
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 4
void juggle()
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 5
void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 90;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) 
  { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 6
void basicSnake()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }

  int x = worm_x;
  int y = worm_y;
  int random_change;
  int random_direction;
  do
  {
    x++;
    if (x > kMatrixWidth) { x = 0;}
    leds[ XY( x, y) ] = CRGB::Blue;
    LEDS.show();
    leds[ XY( x, y) ] = CRGB::Black;
    random_change = random8(kMatrixWidth);
    if (x == random_change)
    {
      // Change Y:
      random_direction = random8(2);
      if (random_direction == 0)
      {
        y--;
        if (y < 0) { y = 0 ;}
      }
      else
      {
        y++;
        if (y >= kMatrixWidth) { y = kMatrixWidth -1;}
      }
    }
    LEDS.delay(1000/FRAMES_PER_SECOND);
  } while (true);
}   
 
// 7
void xyDemo()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
  //SKIPPING: I can't dial the brightness down enough.
  
  //Pretty. Was originally the XY demo pattern
  uint32_t ms = millis();
  int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / kMatrixWidth));
  int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / kMatrixHeight));
  DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
  if( ms < 5000 )
  {
    LEDS.setBrightness( scale8( MASTER_BRIGHTNESS, (ms * 256) / 5000));
  }
  else
  {
    LEDS.setBrightness(MASTER_BRIGHTNESS);
  }
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}

// 8
void crossoverChase()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
  
  // Run a single red LED through the ball. This will zig-zag as it doesn't correct for the 'serpentining'
  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Red;
    LEDS.show();
    leds[i] = CRGB::Black;
    //LEDS.delay(1000/FRAMES_PER_SECOND);
  }
  LEDS.show(); // Turn off the last LED
}

// 9
void pattern3()
{
  int x;
  int x2;
  int z;
  int z2;
  int xx;
  int xx2;
  int yy;
  int zz;
  int zz2;
  LEDS.setBrightness(MASTER_BRIGHTNESS);
  // A coloured led runs top to bottom, crosses over and back up the other side.
  // A different coloured led runs at 45 degrees, 180 degrees out of phase
  
  // This outer loop will go over each column
  x = ch9lastX + 1;
  if (x >= kMatrixWidth) { x = 0;}
  
  x2 = x+1;
  if (x2 >= kMatrixWidth) { x2 -= kMatrixWidth;}
  if (x >= kMatrixWidth/4)
  {
    xx = x - (kMatrixWidth/4);
  }
  else
  {
    xx = x + (kMatrixWidth/4);
  }
  xx2 = xx + 1;
  if (xx2 >= kMatrixWidth) { xx2 -= kMatrixWidth;} 
  
  // This inner loop will go over each row
  for (int y = 0; y < kMatrixHeight; y++)
  {
    if (y == 0)
    {
      ch9Colour1 = CHSV( random8(), 255, 255); // at the top of the ball we change colour
    }
    yy = (kMatrixHeight - 1) - y;
    if (yy == 0)
    {
      ch9Colour2 = CHSV( random8(), 255, 255); // at the top of the ball we change colour
    }
    leds[ XY( x, y) ] = ch9Colour1;
    leds[ XY( x2, y) ] = ch9Colour1;
    leds[ XY( xx, yy) ] = ch9Colour2;
    leds[ XY( xx2, yy) ] = ch9Colour2;
    LEDS.show();
    leds[ XY( x, y)] = CRGB::Black;
    leds[ XY( x2, y)] = CRGB::Black;
    leds[ XY( xx, yy)] = CRGB::Black;
    leds[ XY( xx2, yy)] = CRGB::Black;
    //LEDS.delay(1000/FRAMES_PER_SECOND);
  }
     
  if (x >= kMatrixWidth/2)
  {
    z = x - (kMatrixWidth/2);
  }
  else
  {
    z = x + (kMatrixWidth/2);
  }
  z2 = z + 1;
  if (z2 >= kMatrixWidth) { z2 -= kMatrixWidth;} 
  
  if (xx >= kMatrixWidth/2)
  {
    zz = xx - (kMatrixWidth/2);
  }
  else
  {
    zz = xx + (kMatrixWidth/2);
  }
  zz2 = zz + 1;
  if (zz2 >= kMatrixWidth) { zz2 -= kMatrixWidth;} 
  for (int y = kMatrixHeight - 1; y > 1; y--)
  {
    yy = (kMatrixHeight - 1) - y;
    leds[ XY( z, y) ] = ch9Colour1;
    leds[ XY( z2, y) ] = ch9Colour1;
    leds[ XY( zz, yy) ] = ch9Colour2;
    leds[ XY( zz2, yy) ] = ch9Colour2;
    LEDS.show();
    leds[ XY( z, y)] = CRGB::Black;
    leds[ XY( z2, y)] = CRGB::Black;
    leds[ XY( zz, yy)] = CRGB::Black;
    leds[ XY( zz2, yy)] = CRGB::Black;
    //LEDS.delay(1000/FRAMES_PER_SECOND);
  }
  LEDS.show(); // Turn off the last LED
  LEDS.setBrightness(MASTER_BRIGHTNESS);
  ch9lastX = x; // ... and pass back to the main loop.
}

// 10
void pattern4()
{
  // Run a random coloured LED column around the ball
  // This outer loop will go over each column
  int xx; //The second column, 180degrees out of phase
  int x2; // The second column shadowing the first
  int x3; // 
  int x4; // 
  int xx2;// The 180-deg shadown column
  int xx3;// 
  int xx4;// 
  CHSV thisColour[kMatrixHeight];
  // This loop will determine the colour of each row. (x2 because it's two sides of the ball)
  for(int y = 0; y < (kMatrixHeight); y++)
  {
    thisColour[y] = CHSV( random8(HUE_BLUE, HUE_GREEN), 255, 255);
  }
  for(int x = 0; x < kMatrixWidth; x++) 
  {
    if (x >= kMatrixWidth/2)
    {
      xx = x - (kMatrixWidth/2);
    }
    else
    {
      xx = x + (kMatrixWidth/2);
    }
    x2 = x + 1;
    if (x2 >= kMatrixWidth) { x2 = 0;}
    x3 = x + 2;
    if (x3 >= kMatrixWidth) { x3 = 0;}
    x4 = x + 3;
    if (x4 >= kMatrixWidth) { x4 = 0;}
    xx2 = xx + 1;
    if (xx2 >= kMatrixWidth) { xx2 = 0;}
    xx3 = xx + 2;
    if (xx3 >= kMatrixWidth) { xx3 = 0;}
    xx4 = xx + 3;
    if (xx4 >= kMatrixWidth) { xx4 = 0;}
    // This inner loop will LIGHT each LED in the row
    for(int y = 0; y < kMatrixHeight; y++)
    {
      leds[ XY( x, y) ] = thisColour[y];
      leds[ XY( x2, y) ] = thisColour[y];
      leds[ XY( x3, y) ] = thisColour[y];
      leds[ XY( x4, y) ] = thisColour[y];
      leds[ XY( xx, y) ] = thisColour[y];
      leds[ XY( xx2, y) ] = thisColour[y];
      leds[ XY( xx3, y) ] = thisColour[y];
      leds[ XY( xx4, y) ] = thisColour[y];
    }
    LEDS.show(); // Show the ENTIRE column (all rows) lit.
    // This inner loop will EXTINGUISH all rows in readiness for the next column
    for(int y = 0; y < kMatrixHeight; y++)
    {
      leds[ XY( x,   y) ] = CRGB::Black;
      leds[ XY( x2,  y) ] = CRGB::Black;
      leds[ XY( x3,  y) ] = CRGB::Black;
      leds[ XY( x4,  y) ] = CRGB::Black;
      leds[ XY( xx,  y) ] = CRGB::Black;
      leds[ XY( xx2, y) ] = CRGB::Black;
      leds[ XY( xx3, y) ] = CRGB::Black;
      leds[ XY( xx4, y) ] = CRGB::Black;
    }
    LEDS.show();
    LEDS.delay(10);
  }
}


// 11
void horizontalBounce()
{
  // Runs a horizontal row 'ring' down the ball and back again
  // This outer loop will go over each vertical *row*
  int yy = 0;
  CHSV thisColour;
  for(int y = 0; y < kMatrixHeight - 1; y++) 
  {
    yy = y + 1;
    // This inner loop will LIGHT the led in every column
    for(int x = 0; x < kMatrixWidth; x++)
    {
      thisColour = CHSV( random8(), 255, 255);
      leds[ XY( x, y) ] = thisColour;
      leds[ XY( x, yy) ] = thisColour;
    }
    LEDS.show();
    LEDS.delay(1000/FRAMES_PER_SECOND);
    // This inner loop will EXTINGUISH the row
    for(int x = 0; x < kMatrixWidth; x++)
    {
      leds[ XY( x, y) ] = CRGB::Black;
      leds[ XY( x, yy) ] = CRGB::Black;
    }
    LEDS.show();
  }
  // This does it all in reverse:
  for(int y = kMatrixHeight-1; y >= 1 ; y--) 
  {
    yy = y - 1;
    // This inner loop will LIGHT the led in every column
    for(int x = 0; x < kMatrixWidth; x++)
    {
      thisColour = CHSV( random8(), 255, 255);
      leds[ XY( x, y) ] = thisColour;
      leds[ XY( x, yy) ] = thisColour;
    }
    LEDS.show();
    LEDS.delay(1000/FRAMES_PER_SECOND);
    // This inner loop will EXTINGUISH the row
    for(int x = 0; x < kMatrixWidth; x++)
    {
      leds[ XY( x, y) ] = CRGB::Black;
      leds[ XY( x, yy) ] = CRGB::Black;
    }
    LEDS.show();
  }
}


// 12
void pattern6()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
  
  //Just colour cycle LED 0 to prove a point!
  //turn everything OFF:
  // This outer loop will go over each column
  for(int x = 0; x < kMatrixWidth; x++) 
  {
    // This inner loop will go over each row
    for(int y = 0; y < kMatrixHeight; y++)
    {
      leds[ XY( x, y)] = CRGB::Black;
    }
  }
  leds[ XY( 0, 5) ] = CRGB::Yellow; // CHSV( random8(), 255, 255);
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND);
}


//as shown on youtube
//a noise controlled & modulated by itself
// 13
void noise_noise1()
{
  CRGBPalette16 Pal( pit );
  //modulate the position so that it increases/decreases x
  //(here based on the top left pixel - it could be any position else)
  //the factor "2" defines the max speed of the x movement
  //the "-255" defines the median moving direction
  x[0] = x[0] + (2 * noise[0][0][0]) - 255;
  //modulate the position so that it increases/decreases y
  //(here based on the top right pixel - it could be any position else)
  y[0] = y[0] + (2 * noise[0][kMatrixWidth-1][0]) - 255;
  //z just in one direction but with the additional "1" to make sure to never get stuck
  //in case the movement is stopped by a crazy parameter (noise data) combination
  //(here based on the down left pixel - it could be any position else)
  z[0] += 1 + ((noise[0][0][kMatrixHeight-1]) / 4);
  //set the scaling based on left and right pixel of the middle line
  //here you can set the range of the zoom in both dimensions
  scale_x[0] = 8000 + (noise[0][0][CentreY] * 16);
  scale_y[0] = 8000 + (noise[0][kMatrixWidth-1][CentreY] * 16);

  //calculate the noise data
  uint8_t layer = 0;
  for (uint8_t i = 0; i < kMatrixWidth; i++) {
    uint32_t ioffset = scale_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < kMatrixHeight; j++) {
      uint32_t joffset = scale_y[layer] * (j - CentreY);
      uint16_t data = inoise16(x[layer] + ioffset, y[layer] + joffset, z[layer]);
      // limit the 16 bit results to the interesting range
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      // normalize
      data = data - 11000;
      // scale down that the result fits into a byte
      data = data / 161;
      // store the result in the array
      noise[layer][i][j] = data;
    }
  }

  //map the colors
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      //I will add this overlay CRGB later for more colors
      //it´s basically a rainbow mapping with an inverted brightness mask
      CRGB overlay = CHSV(noise[0][y][x], 255, noise[0][x][y]);
      //here the actual colormapping happens - note the additional colorshift caused by the down right pixel noise[0][15][15]
      leds[XY(x, y)] = ColorFromPalette( Pal, noise[0][kMatrixWidth-1][kMatrixHeight-1] + noise[0][x][y]) + overlay;
    }
  }

  //make it looking nice
  adjust_gamma();

  //and show it!
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}


// 14 - https://forum.arduino.cc/index.php?topic=600200.0
void plasma() 
{
  static byte offset  = 0; // counter for radial color wave motion
  static int plasVector = 0; // counter for orbiting plasma center

  // Calculate current center of plasma pattern (can be offscreen)
  int xOffset = cos8(plasVector / 256);
  int yOffset = sin8(plasVector / 256);

  // Draw one frame of the animation into the LED array
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      byte color = sin8(sqrt(sq(((float)x - 7.5) * 10 + xOffset - 127) + sq(((float)y - 2) * 10 + yOffset - 127)) + offset);
      leds[XY(x, y)] = CHSV(color, 255, 255);
    }
  }

  offset++; // wraps at 255 for sin8
  plasVector += 16; // using an int for slower orbit (wraps at 65536)
  LEDS.show(); // send the contents of the led memory to the LEDs
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}


// 15 https://github.com/evilgeniuslabs/torch/blob/master/torch.ino
void applause()
{
  static uint16_t lastPixel = 0;
  fadeToBlackBy(leds, NUM_LEDS, 4); // Was originally 32
  leds[lastPixel] = CHSV(random8(HUE_BLUE, HUE_PURPLE), 255, 255);
  lastPixel = random16(NUM_LEDS);
  leds[lastPixel] = CRGB::White;
  return ;
}


// 16 https://github.com/evilgeniuslabs/torch/blob/master/torch.ino
void huecycle()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  LEDS.delay(1000/FRAMES_PER_SECOND); 
}


// Pride2015 by Mark Kriegsman
// https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
// 17 
void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (int i = 0; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t) ((uint32_t) b16 * (uint32_t) b16) / 65536;
    uint8_t bri8 = (uint32_t) (((uint32_t) bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }
  LEDS.delay(1000/FRAMES_PER_SECOND); 
  return ;
}


// 18 - fire (.h file)
// 19 - wave (.h file)

                                      
// 20 - https://gist.github.com/gnkarn/908383d5b81444362bc2e5421566fdd1
void matrixEffect()
{
  EVERY_N_MILLIS(75) // falling speed
  // move code downward
  // start with lowest row to allow proper overlapping on each column
  for (int8_t row=kMatrixHeight-1; row>=0; row--)
  {
    for (int8_t col=0; col<kMatrixWidth; col++)
    {
      if (leds[XY(col, row)] == CRGB(175,255,175))
      {
        leds[XY(col, row)] = CRGB(27,130,39); // create trail
        if (row < kMatrixHeight-1) leds[XY(col, row+1)] = CRGB(175,255,175);
      }
    }
  }

  // fade all leds
  for(int i = 0; i < NUM_LEDS; i++) {
    if (leds[i].g != 255) leds[i].nscale8(192); // only fade trail
  }

  // check for empty screen to ensure code spawn
  bool emptyScreen = true;
  for(int i = 0; i < NUM_LEDS; i++) {
    if (leds[i])
    {
      emptyScreen = false;
      break;
    }
  }

  // spawn new falling code
  //if (random8(8) == 0 || emptyScreen) // lower number == more frequent spawns
  //{
  int8_t spawnX = random8(kMatrixWidth);
  leds[XY(spawnX, 0)] = CRGB(175,255,175 );
  //}
  LEDS.delay(400/FRAMES_PER_SECOND); 
  LEDS.show();
}

// 21 - test pattern: static corkscrew
// 22 - simple corkscrew
// 23 - my trailing corkscrew: trailingCorkscrew()
// 24 - my trailing corkscrew, backwards: reverseCorkscrew()


// 25 https://bitbucket.org/ratkins/ledeffects/src/default/
void doSnake()
{
    if (PATTERN_MODE != 0) { nextPattern(); return; }
    Snake snake(leds, kMatrixWidth, kMatrixHeight);
    snake.start();
}


//26 - Pac-Man (.h file)


//27
void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  LEDS.delay(1000/FRAMES_PER_SECOND);  
}


// 28
void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(200);
}


void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter)
  {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}


//29
void spare29()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
}


//30
void spare30()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
}


//31
void spare31()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
}

