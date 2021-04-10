

FastLED_HD108
=============

This is a branch of FastLED designed for 16-bit control of HD108 LEDs.  It includes control for
the 5-bit brightness value for each LED, for each R/G/B channel.

Look at the Fade_HD108 example for how to use it.  I've built and tested it as a .cpp in VScode
with PlatformIO on ESP32, but it should also work with Arduino IDE as an .ino if you import the
library (as a ZIP, or in your Arduino libraries folder).

This is not meant to be a final or optimal implementation, but it does allow you to use standard
8-bit CRGB structures for the hi-byte of the 16-bit value if you wish, or full control of the 
16-bit value and individual brightness channels.

Feel free to leave feedback or questions, or links to similar projects.

## Notes

HD108 have a very non-linear light response from what I've measured, so I have separate
code for doing color correction and linearizing the brightness response in each channel.  
It involves a lot of fixed point math and is still too ugly to share.

No guarantees are given that this code is not also ugly.  But it should be close enough to 
working to get you started with 16-bit LED control, and those sweet dim fades.

## Simple example

How quickly can you get up and running with the library?  Here's a 'simple' rainbow program:


	#include <FastLED_HD108.h>
	#define NUM_LEDS 10
	#define DATA_PIN 2
	#define CLOCK_PIN 17 
		
	// Hi-byte for 16 bit RGB value:
	CRGB leds[NUM_LEDS];
	// Lo-byte for 16 bit RGB value:
	CRGB ledlos[NUM_LEDS];
	// 5-bit brightness channel for each channel of each LED
	uint8_t ledBrights_R[NUM_LEDS]; 
	uint8_t ledBrights_G[NUM_LEDS]; 
	uint8_t ledBrights_B[NUM_LEDS]; 
	uint32_t loopcnt=0;	
		
	void setup() { 
	  FastLED.addLeds<HD108, DATA_PIN, CLOCK_PIN, RGB>(leds, ledlos, ledBrights_R, ledBrights_G, ledBrights_B, NUM_LEDS);
	}
		
	void loop() {
	  loopcnt++;
	  for (int i=0;i<NUM_LEDS;i++)
	  {
	    uint16_t cur_r = (10000+1*loopcnt)%65535;
	    uint16_t cur_g = (10000-2*loopcnt)%65535;
	    uint16_t cur_b = (10000+3*loopcnt)%65535;
	    	
	    // load hi and lo byte:
	    leds[i].r = cur_r >> 8;
	    leds[i].g = cur_g >> 8;
	    leds[i].b = cur_b >> 8;
	    ledlos[i].r = cur_r & 0b11111111;
	    ledlos[i].g = cur_g & 0b11111111;
	    ledlos[i].b = cur_b & 0b11111111;
	    	
			// set brightness
	    uint8_t _bright = 0; // 0 is dimmest,  31 brightest
	    ledBrights_R[i] = _bright; // can be unique per channel
	    ledBrights_G[i] = _bright;
	    ledBrights_B[i] = _bright;
	  }
	  FastLED.show();
	  delay(3);
	}
	

## Supported LED chipsets

HD107 / APA102 / SK9822 all also have 5-bit brightnes control supported in the standard branch of FastLED, 
but not with per-LED control.  I have this working in a development branch but haven't cleaned it up for 
here yet.  You can try it out but as of posting this I don't think it's working.

HD108 should be working fine.  I am not aware of any other 16 bit LED chips but send me a message or open
an issue if you find any and are interested in having them included.


## Supported platforms

Tested so far on ESP32.  With 72 bits per LED and 16/32 bit values used for calculations, the abundant 
memory and speed comes in handy for longer runs.

## For more information

PM /u/flaming_s_word on reddit



