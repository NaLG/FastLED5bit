
#include <FastLED_HD108.h>

#define NUM_LEDS 10

// For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the HD108, define both DATA_PIN and CLOCK_PIN

#define DATA_PIN 2
#define CLOCK_PIN 17  // on ESP32, needs to be higher than 16

// Select 
// TODO: HD107 is broken currently, it was working not long ago:
// #define USE_HD107  // also used for APA102, SK9822
#define USE_HD108

// SK9822, APA102, and HD107 use the same protocol, including a 5-bit per-LED brightness value,
//     with 8 additional bits for each R G B channel.
#ifdef USE_HD107
  #define LEDTYPE SK9822 // same protocol as HD107 and APA102
  #define LEDRGBORDER BGR // SK9832 seems to use BGR, or the driver makes it so

// HD108 has 5-bit brightness for EACH R G B channel,
//     and 16 additional bits for each R G B channel.
#elif defined(USE_HD108)
  #define LEDTYPE HD108 // 
  #define LEDRGBORDER RGB // HD108 uses GRB but the chipset controller handles that
#endif

// #define LEDTYPE WS2812

#define BAUDRATE 115200

// Define the array of leds
// Hi-byte for 16 bit RGB value:
CRGB leds[NUM_LEDS];
// Lo-byte for 16 bit RGB value:
CRGB ledlos[NUM_LEDS];

// 5-bit brightness channel for each LED - used for HD107 and SK9822 / APA102
uint8_t ledBrights[NUM_LEDS]; //used for chips w/brightness per-LED (apa102 sk9822 hd107s)

// 5-bit brightness channel for each channel of each LED - used for HD108
uint8_t ledBrights_R[NUM_LEDS]; // used for chips w/brightness per-CHANNEL per-LED (HD108)
uint8_t ledBrights_G[NUM_LEDS]; // 
uint8_t ledBrights_B[NUM_LEDS]; // 


#if defined(ESP32) // math library adaptation for ESP32
  #define max(x,y) _max(x,y)
  #define min(x,y) _min(x,y)
#endif


#define DEFAULT_TEMPERATURE_CORRECTION LEDColorCorrection::UncorrectedColor


void setup() { 

  Serial.begin(BAUDRATE);

  #ifdef USE_HD107
    // TODO: HD107 is broken currently, it was working not long ago 
    //         with 5-bit brightness per-led control, I'll try to get it back up soon:
    FastLED.addLeds<LEDTYPE, DATA_PIN, CLOCK_PIN, LEDRGBORDER>(leds, ledBrights, NUM_LEDS);
  #elif defined(USE_HD108)

    FastLED.addLeds<LEDTYPE, DATA_PIN, CLOCK_PIN, LEDRGBORDER>(leds, ledlos, ledBrights_R, ledBrights_G, ledBrights_B, NUM_LEDS); // TODO: Doublecheck brights order

    // You can tinker with the data rate, though this implementation is not super fast or efficient either way.
    //   FastLED may be limiting the clock rate below what is selected here.
      // For fast driving I've had better luck using a separate driver and/or parallel I2C by /u/yves-bazin 
    // FastLED.addLeds<LEDTYPE, DATA_PIN, CLOCK_PIN, LEDRGBORDER, DATA_RATE_MHZ(10)>(leds, ledlos, ledBrights_R, ledBrights_G, ledBrights_B, NUM_LEDS);

    // DATA_RATE_MHZ is non linear (for 700 leds)
    //   DATA_RATE_MHZ(1)  : 53ms
    //   DATA_RATE_MHZ(4)  : 22ms
    //   DATA_RATE_MHZ(10) : 13ms
    //   DATA_RATE_MHZ(20) : 12ms
    //   DATA_RATE_MHZ(40) : 13ms
    //   DATA_RATE_MHZ(50) : 13ms
    //
    //   DATA_RATE_MHZ(10), norm writeled() : 13ms
    //   DATA_RATE_MHZ(10), simp writeled() : 13ms
    //   FastLED may limit clock

  #else // if you want to test with WS2812, etc
    FastLED.addLeds<LEDTYPE, DATA_PIN, LEDRGBORDER>(leds, NUM_LEDS);
  #endif
  FastLED.setTemperature( DEFAULT_TEMPERATURE_CORRECTION );
  
  //Turn off dithering.  Dithering by fastLED is handled with the hi-byte, it may not work as intended here.
  FastLED.setDither(0); 

}


uint32_t loopcnt=0;

#ifdef USE_HD107
  #define NUMBRIGHTLEVELS 4
  // 0 to 31 - with HD107, 0 provides no light.
  // May need to experiment with SK9822 / APA102:
  int brightlevels[NUMBRIGHTLEVELS] = {1, 3, 9, 31};
#elif defined(USE_HD108)
  // 0 to 31 - with HD108, 0 still provides light
  #define NUMBRIGHTLEVELS 2
  int brightlevels[NUMBRIGHTLEVELS] = {0, 31};
#endif

#define BRIGHTCYCLESECONDS 5


#ifdef USE_HD108 //16  bit
  uint32_t flr    = 0;
  // uint32_t maxval = 1000; // Very dim, test out the floor of the dim fade range
  uint32_t maxval = 4000; // Fairly dim, test out the dim fade range
  // uint32_t maxval = 20000; // 
  // int32_t maxval = 65535; // Very bright - don't damage any eyeballs looking directly
  
  // can differentiate these, or just use color balancing above.
  uint32_t maxval_r = maxval; 
  uint32_t maxval_g = maxval; 
  uint32_t maxval_b = maxval; 
  const uint32_t _bitscale = 256;
#else  // 8 bit
  uint32_t flr    = 0;
  uint32_t maxval = 60;
  // int32_t maxval = 255; // Very bright if you're looking directly - don't damage any eyeballs
  uint32_t maxval_r = maxval;
  uint32_t maxval_g = maxval;
  uint32_t maxval_b = maxval;
  const uint32_t _bitscale = 1;
#endif


int _bright;
uint32_t cur_r; // only 0 to 65535, but 32bit is easier to work with and not overflow
uint32_t cur_g;
uint32_t cur_b;

float scale_r = ( (float)_bitscale * maxval / 10000 ) / 11.0 ;
float scale_g = ( (float)_bitscale * maxval / 10000 ) / 17.0 ;
float scale_b = ( (float)_bitscale * maxval / 10000 ) / 29.0 ;


void loop() {

  loopcnt++;

  if (loopcnt*_bitscale > 4000000000)
    loopcnt = 0; // basic overflow protection


  // #ifdef USE_HD107

  // {
  //   // Be kind, don't blind - ramp up:
  //   byte brightness_ = min( ((millis()%TOTAL_MS) - SEC1_MS)/6, 255);

  //   for (int i=0;i<NUM_LEDS;i++) 
  //     leds[i] = CRGB(brightness_,brightness_,brightness_);
  // }

  // simple fading demo, uses gamma and low fades.
  // uses floats and redundant loop calculations, not efficient
  for (int i=0;i<NUM_LEDS;i++)
  {

    cur_r = max( abs(maxval_r - ((i*3*_bitscale + (uint32_t)(loopcnt*scale_r) ) % (maxval_r*2)) ), flr);
    cur_g = max( abs(maxval_g - ((i*3*_bitscale + (uint32_t)(loopcnt*scale_g) ) % (maxval_g*2)) ), flr);
    cur_b = max( abs(maxval_b - ((i*3*_bitscale + (uint32_t)(loopcnt*scale_b) ) % (maxval_b*2)) ), flr);

    // gamma:
    cur_r *= cur_r;
    // cur_r /= maxval_r;
    cur_r /= _bitscale;
    cur_r /= 256;
    cur_g *= cur_g;
    // cur_g /= maxval_g;
    cur_g /= _bitscale;
    cur_g /= 256;
    cur_b *= cur_b;
    // cur_b /= maxval_b;
    cur_b /= _bitscale;
    cur_b /= 256;

    // load hi and lo byte:
    leds[i].r = cur_r >> 8;
    leds[i].g = cur_g >> 8;
    leds[i].b = cur_b >> 8;

    ledlos[i].r = cur_r & 0b11111111;
    ledlos[i].g = cur_g & 0b11111111;
    ledlos[i].b = cur_b & 0b11111111;

    // Change brightness across time and led position, group each 3 leds:
    _bright = brightlevels[  ( (millis() % (BRIGHTCYCLESECONDS * 1000 * NUMBRIGHTLEVELS) )/(BRIGHTCYCLESECONDS * 1000)
                                  + (( i/3 ) % NUMBRIGHTLEVELS )
                                ) % NUMBRIGHTLEVELS  ];

    // load bright byte (really 5 bits):
    #ifdef USE_HD107
      ledBrights[i] = _bright;
    #endif
    #ifdef USE_HD108
      // can be independent, for this demo they're the same:
      ledBrights_R[i] = _bright; 
      ledBrights_G[i] = _bright;
      ledBrights_B[i] = _bright;
    #endif
  }


  FastLED.show();

  delay(3);
}







