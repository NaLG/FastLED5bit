#ifndef __INC_CHIPSETS_H
#define __INC_CHIPSETS_H

#include "FastLED.h"
#include "pixeltypes.h"

///@file chipsets.h
/// contains the bulk of the definitions for the various LED chipsets supported.

FASTLED_NAMESPACE_BEGIN
///@defgroup chipsets
/// Implementations of CLEDController classes for various led chipsets.
///
///@{

#if defined(ARDUINO) //&& defined(SoftwareSerial_h)


// #include <SoftwareSerial.h> // NLG
// SoftwareSerial Serial; // NLG

// #if defined(SoftwareSerial_h) && 0 // NLG
#if defined(SoftwareSerial_h)
#include <SoftwareSerial.h>

#define HAS_PIXIE

/// Adafruit Pixie controller class
/// @tparam DATAPIN the pin to write data out on
/// @tparam RGB_ORDER the RGB ordering for the led data
template<uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class PixieController : public CPixelLEDController<RGB_ORDER> {
  SoftwareSerial Serial;
  CMinWait<2000> mWait;
public:
  PixieController() : Serial(-1, DATA_PIN) {}

protected:
  virtual void init() {
    Serial.begin(115200);
    mWait.mark();
  }

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mWait.wait();
    while(pixels.has(1)) {
      uint8_t r = pixels.loadAndScale0();
      Serial.write(r);
      uint8_t g = pixels.loadAndScale1();
      Serial.write(g);
      uint8_t b = pixels.loadAndScale2();
      Serial.write(b);
      pixels.advanceData();
      pixels.stepDithering();
    }
    mWait.mark();
  }

};

// template<SoftwareSerial & STREAM, EOrder RGB_ORDER = RGB>
// class PixieController : public PixieBaseController<STREAM, RGB_ORDER> {
// public:
//  virtual void init() {
//    STREAM.begin(115200);
//  }
// };
#endif
#endif

///@name Clocked chipsets - nominally SPI based these chipsets have a data and a clock line.
///@{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LPD8806 controller class - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// LPD8806 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(12)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB,  uint32_t SPI_SPEED = DATA_RATE_MHZ(12) >
class LPD8806Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;

  class LPD8806_ADJUST {
  public:
    // LPD8806 spec wants the high bit of every rgb data byte sent out to be set.
    __attribute__((always_inline)) inline static uint8_t adjust(register uint8_t data) { return ((data>>1) | 0x80) + ((data && (data<254)) & 0x01); }
    __attribute__((always_inline)) inline static void postBlock(int len) {
      SPI::writeBytesValueRaw(0, ((len*3+63)>>6));
    }

  };

  SPI mSPI;
public:

  LPD8806Controller()  {}
  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.template writePixels<0, LPD8806_ADJUST, RGB_ORDER>(pixels);
  }
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WS2801 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// WS2801 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(1)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(1)>
class WS2801Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;
  CMinWait<1000>  mWaitDelay;
public:
  WS2801Controller() {}

  virtual void init() {
    mSPI.init();
    mWaitDelay.mark();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mWaitDelay.wait();
    mSPI.template writePixels<0, DATA_NOP, RGB_ORDER>(pixels);
    mWaitDelay.mark();
  }
};

template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(25)>
class WS2803Controller : public WS2801Controller<DATA_PIN, CLOCK_PIN, RGB_ORDER, SPI_SPEED> {};

/// LPD6803 controller class (LPD1101).
/// 16 bit (1 bit - const "1", 5 bit - red, 5 bit - green, 5 bit blue).
/// In chip CMODE pin must be set to 1 (inside oscillator mode).
/// Datasheet: https://cdn-shop.adafruit.com/datasheets/LPD6803.pdf
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(12)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(12)>
class LPD6803Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;

  void startBoundary() { mSPI.writeByte(0); mSPI.writeByte(0); mSPI.writeByte(0); mSPI.writeByte(0); }

public:
  LPD6803Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.select();

    startBoundary();
    while(pixels.has(1)) {
            register uint16_t command;
            command = 0x8000;
            command |= (pixels.loadAndScale0() & 0xF8) << 7; // red is the high 5 bits
            command |= (pixels.loadAndScale1() & 0xF8) << 2; // green is the middle 5 bits
      mSPI.writeByte((command >> 8) & 0xFF);
            command |= pixels.loadAndScale2() >> 3 ; // blue is the low 5 bits
      mSPI.writeByte(command & 0xFF);

      pixels.stepDithering();
      pixels.advanceData();
    }
    //endBoundary(pixels.size());
    mSPI.waitFully();
    mSPI.release();
  }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// APA102 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// APA102 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(12)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(12)>
class APA102Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;

  void startBoundary() { mSPI.writeWord(0); mSPI.writeWord(0); }
  void endBoundary(int nLeds) { int nDWords = (nLeds/32); do { mSPI.writeByte(0xFF); mSPI.writeByte(0x00); mSPI.writeByte(0x00); mSPI.writeByte(0x00); } while(nDWords--); }

  inline void writeLed(uint8_t brightness, uint8_t b0, uint8_t b1, uint8_t b2) __attribute__((always_inline)) {
#ifdef FASTLED_SPI_BYTE_ONLY
    mSPI.writeByte(0xE0 | brightness);
    mSPI.writeByte(b0);
    mSPI.writeByte(b1);
    mSPI.writeByte(b2);
#else
    uint16_t b = 0xE000 | (brightness << 8) | (uint16_t)b0;
    mSPI.writeWord(b);
    uint16_t w = b1 << 8;
    w |= b2;
    mSPI.writeWord(w);
#endif
  }

public:
  APA102Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.select();

    uint8_t s0 = pixels.getScale0(), s1 = pixels.getScale1(), s2 = pixels.getScale2();
#if FASTLED_USE_GLOBAL_BRIGHTNESS == 1
    const uint16_t maxBrightness = 0x1F;
    uint16_t brightness = ((((uint16_t)max(max(s0, s1), s2) + 1) * maxBrightness - 1) >> 8) + 1;
    s0 = (maxBrightness * s0 + (brightness >> 1)) / brightness;
    s1 = (maxBrightness * s1 + (brightness >> 1)) / brightness;
    s2 = (maxBrightness * s2 + (brightness >> 1)) / brightness;
#else
    const uint8_t brightness = 0x1F;
#endif

    startBoundary();
    while (pixels.has(1)) {
      writeLed(brightness, pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2));
      pixels.stepDithering();
      pixels.advanceData();
    }
    endBoundary(pixels.size());

    mSPI.waitFully();
    mSPI.release();
  }

};

/// SK9822 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(24)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(24)>
class SK9822Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;

  void startBoundary() { mSPI.writeWord(0); mSPI.writeWord(0); }
  void endBoundary(int nLeds) { int nLongWords = (nLeds/32); do { mSPI.writeByte(0x00); mSPI.writeByte(0x00); mSPI.writeByte(0x00); mSPI.writeByte(0x00); } while(nLongWords--); }

  inline void writeLed(uint8_t brightness, uint8_t b0, uint8_t b1, uint8_t b2) __attribute__((always_inline)) {
#ifdef FASTLED_SPI_BYTE_ONLY
    mSPI.writeByte(0xE0 | brightness);
    mSPI.writeByte(b0);
    mSPI.writeByte(b1);
    mSPI.writeByte(b2);
#else
    uint16_t b = 0xE000 | (brightness << 8) | (uint16_t)b0;
    mSPI.writeWord(b);
    uint16_t w = b1 << 8;
    w |= b2;
    mSPI.writeWord(w);
#endif
  }

public:
  SK9822Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.select();

    uint8_t s0 = pixels.getScale0(), s1 = pixels.getScale1(), s2 = pixels.getScale2();
#if FASTLED_USE_GLOBAL_BRIGHTNESS == 1
    const uint16_t maxBrightness = 0x1F;
    uint16_t brightness = ((((uint16_t)max(max(s0, s1), s2) + 1) * maxBrightness - 1) >> 8) + 1;
    s0 = (maxBrightness * s0 + (brightness >> 1)) / brightness;
    s1 = (maxBrightness * s1 + (brightness >> 1)) / brightness;
    s2 = (maxBrightness * s2 + (brightness >> 1)) / brightness;
#elif FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
    // TODO -
#else
    const uint8_t brightness = 0x1F; //0x01; // debug - nlg
    // const uint8_t brightness = 0x01; // debug - nlg
#endif

    // brightness = 1 ; //debug - nlg
    // brightness = 32 ; //debug - nlg
    startBoundary();
    while (pixels.has(1)) {
      #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
        writeLed(pixels.get5bitBright(), pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2));
      #else
        writeLed(brightness, pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2));
      #endif
      pixels.stepDithering();
      pixels.advanceData();
    }

    endBoundary(pixels.size());

    mSPI.waitFully();
    mSPI.release();
  }

};


/// HD108 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(24)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = GRB, uint32_t SPI_SPEED = DATA_RATE_MHZ(4)>
class HD108Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;
  uint16_t count_ = 0; // NLG

  void startBoundary() 
  { 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
    mSPI.writeWord(0); 
  }
  void endBoundary(int nLeds) 
  { int nLongWords = (nLeds/48);
    do 
    { 
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
      mSPI.writeByte(0x00);
    } while(nLongWords--); 
  }

  // inline uint16_t bytetohex(byte b)
  // {
  //   uint16_t hexstr= "00";
  //   hexstr[0] = (b >> 4) + 0x30;
  //   if (hexstr[0] > 0x39) 
  //     hexstr[0] +=7;
  //   hexstr[1] = (b & 0x0f) + 0x30;
  //   if (hexstr[1] > 0x39) 
  //     hexstr[1] +=7;
  // }
  inline char bytetohexhi(byte b)
  {
    char hi = (b >> 4) + 0x30;
    if (hi > 0x39) 
      hi +=7;
    return hi;
  }
  inline char bytetohexlo(byte b)
  {
    char lo = (b & 0x0f) + 0x30;
    if (lo > 0x39) 
      lo +=7;
    return lo;
  }

  inline void writeLed(uint8_t brightness, uint8_t b0, uint8_t b1, uint8_t b2) __attribute__((always_inline)) {
#ifdef FASTLED_SPI_BYTE_ONLY
    // mSPI.writeByte(0xE0 | brightness);
    mSPI.writeByte(0x80 | (brightness<<2) | ((brightness>>3) & 0x03));
    mSPI.writeByte((0xE0 & (brightness<<5)) | brightness);
    mSPI.writeByte(b0);
    mSPI.writeByte(00);
    mSPI.writeByte(b1);
    mSPI.writeByte(00);
    mSPI.writeByte(b2);
    mSPI.writeByte(00);
#else
    // uint16_t b = 0xE000 | (brightness << 8) | (uint16_t)b0;
    uint16_t b = 0x8000 | (brightness << 10) | (brightness << 5) | (brightness);
    uint16_t w0 = b0 << 8;
    uint16_t w1 = b1 << 8;
    uint16_t w2 = b2 << 8;
    mSPI.writeWord(b);
    mSPI.writeWord(w0);
    mSPI.writeWord(w1);
    mSPI.writeWord(w2);
#endif
  }

  inline void writeLed(uint8_t brightness0, uint8_t brightness1, uint8_t brightness2, uint8_t b0hi, uint8_t b0lo, uint8_t b1hi, uint8_t b1lo, uint8_t b2hi, uint8_t b2lo) __attribute__((always_inline)) {
#ifdef FASTLED_SPI_BYTE_ONLY
    // mSPI.writeByte(0xE0 | brightness);
    mSPI.writeByte(0x80 | (brightness0<<2) | ((brightness1>>3) & 0x03));
    mSPI.writeByte((0xE0 & (brightness1<<5)) | brightness2);
    mSPI.writeByte(b0hi);
    mSPI.writeByte(b0lo);
    mSPI.writeByte(b1hi);
    mSPI.writeByte(b1lo);
    mSPI.writeByte(b2hi);
    mSPI.writeByte(b2lo);
#else
    // if (count_++==10000)
    if (false)
    {
      Serial.write("oy ");
      Serial.write(bytetohexhi(brightness0));
      Serial.write(bytetohexlo(brightness0));
      Serial.write(bytetohexhi(brightness1));
      Serial.write(bytetohexlo(brightness1));
      Serial.write(bytetohexhi(brightness2));
      Serial.write(bytetohexlo(brightness2));
      Serial.write(bytetohexhi(b0hi));
      Serial.write(bytetohexlo(b0hi));
      Serial.write(bytetohexhi(b0lo));
      Serial.write(bytetohexlo(b0lo));
      Serial.write(bytetohexhi(b1hi));
      Serial.write(bytetohexlo(b1hi));
      Serial.write(bytetohexhi(b1lo));
      Serial.write(bytetohexlo(b1lo));
      Serial.write(bytetohexhi(b2hi));
      Serial.write(bytetohexlo(b2hi));
      Serial.write(bytetohexhi(b2lo));
      Serial.write(bytetohexlo(b2lo));
      Serial.write("\n");
      count_=0;
    }
    // uint16_t b = 0xE000 | (brightness << 8) | (uint16_t)b0;
    uint16_t b = 0x8000 | (brightness0 << 10) | (brightness1 << 5) | (brightness2);
    uint16_t w0 = (b0hi<<8) | b0lo;
    uint16_t w1 = (b1hi<<8) | b1lo;
    uint16_t w2 = (b2hi<<8) | b2lo;
    mSPI.writeWord(b);
    mSPI.writeWord(w0);
    mSPI.writeWord(w1);
    mSPI.writeWord(w2);
    // mSPI.writeWord(65535);
    // mSPI.writeWord(count_);
    // mSPI.writeWord(count_);
    // mSPI.writeWord(count_);
    // mSPI.writeWord(35);
    // mSPI.writeWord(32768);
    // mSPI.writeWord(30000);
    // mSPI.writeWord(30000);
    // mSPI.writeWord(30000);
    // mSPI.writeWord(32768);
    // mSPI.writeWord(10);
    // mSPI.writeWord(10);
    // mSPI.writeWord(10);
    // mSPI.writeWord(33825); // 1 1 1
    // mSPI.writeWord(10);
    // mSPI.writeWord(10);
    // mSPI.writeWord(10);
    // mSPI.writeWord(33825); // 1 1 1
    // mSPI.writeWord(10000);
    // mSPI.writeWord(10000);
    // mSPI.writeWord(10000);
#endif
  }

  inline void writeLed(uint8_t brightness0, uint8_t brightness1, uint8_t brightness2, uint16_t b0, uint16_t b1, uint16_t b2) __attribute__((always_inline)) {
#ifdef FASTLED_SPI_BYTE_ONLY
    // mSPI.writeByte(0xE0 | brightness);
    mSPI.writeByte(0x80 | (brightness0<<2) | ((brightness1>>3) & 0x03));
    mSPI.writeByte((0xE0 & (brightness1<<5)) | brightness2);
    mSPI.writeByte(b0 >> 8);
    mSPI.writeByte(b0 & 0xFF);
    mSPI.writeByte(b1 >> 8);
    mSPI.writeByte(b1 & 0xFF);
    mSPI.writeByte(b2 >> 8);
    mSPI.writeByte(b2 & 0xFF);
#else
    // uint16_t b = 0xE000 | (brightness << 8) | (uint16_t)b0;
    uint16_t b = 0x8000 | (brightness0 << 10) | (brightness1 << 5) | (brightness2);
    uint16_t w0 = b0;
    uint16_t w1 = b1;
    uint16_t w2 = b2;
    mSPI.writeWord(b);
    mSPI.writeWord(w0);
    mSPI.writeWord(w1);
    mSPI.writeWord(w2);
#endif
  }

public:
  HD108Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.select();

    uint8_t s0 = pixels.getScale0(), s1 = pixels.getScale1(), s2 = pixels.getScale2();
#if FASTLED_USE_GLOBAL_BRIGHTNESS == 1
    const uint16_t maxBrightness = 0x1F;
    uint16_t brightness = ((((uint16_t)max(max(s0, s1), s2) + 1) * maxBrightness - 1) >> 8) + 1;
    s0 = (maxBrightness * s0 + (brightness >> 1)) / brightness;
    s1 = (maxBrightness * s1 + (brightness >> 1)) / brightness;
    s2 = (maxBrightness * s2 + (brightness >> 1)) / brightness;
#elif FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
    // TODO -
    // s0 = 255;
    // s1 = 255;
    // s2 = 255;
#else
    const uint8_t brightness = 0x1F; //0x01; // debug - nlg
    // const uint8_t brightness = 0x01; // debug - nlg
#endif

    // brightness = 1 ; //debug - nlg
    // brightness = 32 ; //debug - nlg
    startBoundary();
    while (pixels.has(1)) {
      #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
        #if FASTLED_USE_16_BIT_PIXELS == 1
          // if (count_++==10000)
          // {
          //   Serial.write('a');
          //   Serial.write('k');
          //   // Serial.write(bytetohexhi(b2lo));
          //   // Serial.write(bytetohexlo(b2lo));
          //   Serial.write("\n");
          //   count_=0;
          // }
          // writeLed(1, 1, 1, 5, 5, 5, 5, 5, 5);
          // writeLed(1, 1, 1, 0, 0, 0, 0, 0, 0);
          // writeLed(1, 1, 1, 255, 255, 0, 0, 0, 0);
          // writeLed(0, 0, 0, 0, 0, 0, 0, 0, 0);
          
          // writeLed(pixels.get5bitBright0(), pixels.get5bitBright1(), pixels.get5bitBright2(), pixels.loadAndScale0(0), 0, pixels.loadAndScale1(0), 0, pixels.loadAndScale2(0), 0); // DEBUG
          // writeLed(pixels.get5bitBright0(), pixels.get5bitBright1(), pixels.get5bitBright2(), pixels.loadAndScale0(0, s0), 0, pixels.loadAndScale1(0, s1), 0, pixels.loadAndScale2(0, s2), 0); // DEBUG
          // writeLed(pixels.get5bitBright0(), pixels.get5bitBright1(), pixels.get5bitBright2(), pixels.loadAndScale0(0, s0), pixels.loadAndScale0Lo(0, s0), pixels.loadAndScale1(0, s1), 0, pixels.loadAndScale2(0, s2), 0); // DEBUG
  
          writeLed(pixels.get5bitBright0(), pixels.get5bitBright1(), pixels.get5bitBright2(), pixels.loadAndScale0(0, s0), pixels.loadAndScale0Lo(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale1Lo(0, s1), pixels.loadAndScale2(0, s2), pixels.loadAndScale2Lo(0, s2)); // WORKS
          // DEBUG:
          // writeLed(pixels.get5bitBright0(), pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2)); // DEBUG
        #else
          writeLed(pixels.get5bitBright(), pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2));
        #endif
      #else
        writeLed(brightness, pixels.loadAndScale0(0, s0), pixels.loadAndScale1(0, s1), pixels.loadAndScale2(0, s2));
        // writeLed(brightness, pixels.loadAndScaleHi0(0, s0), pixels.loadAndScale0Lo(0, s0), pixels.loadAndScaleHi1(0, s1), pixels.loadAndScale1Lo(0, s1), pixels.loadAndScaleHi2(0, s2), pixels.loadAndScale2Lo(0, s2));
      #endif
      pixels.stepDithering();
      pixels.advanceData();
    }

    endBoundary(pixels.size());

    mSPI.waitFully();
    mSPI.release();
  }

};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// P9813 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// P9813 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(10)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(10)>
class P9813Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;

  void writeBoundary() { mSPI.writeWord(0); mSPI.writeWord(0); }

  inline void writeLed(uint8_t r, uint8_t g, uint8_t b) __attribute__((always_inline)) {
    register uint8_t top = 0xC0 | ((~b & 0xC0) >> 2) | ((~g & 0xC0) >> 4) | ((~r & 0xC0) >> 6);
    mSPI.writeByte(top); mSPI.writeByte(b); mSPI.writeByte(g); mSPI.writeByte(r);
  }

public:
  P9813Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    mSPI.select();

    writeBoundary();
    while(pixels.has(1)) {
      writeLed(pixels.loadAndScale0(), pixels.loadAndScale1(), pixels.loadAndScale2());
      pixels.advanceData();
      pixels.stepDithering();
    }
    writeBoundary();
    mSPI.waitFully();

    mSPI.release();
  }

};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SM16716 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// SM16716 controller class.
/// @tparam DATA_PIN the data pin for these leds
/// @tparam CLOCK_PIN the clock pin for these leds
/// @tparam RGB_ORDER the RGB ordering for these leds
/// @tparam SPI_SPEED the clock divider used for these leds.  Set using the DATA_RATE_MHZ/DATA_RATE_KHZ macros.  Defaults to DATA_RATE_MHZ(16)
template <uint8_t DATA_PIN, uint8_t CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(16)>
class SM16716Controller : public CPixelLEDController<RGB_ORDER> {
  typedef SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
  SPI mSPI;

  void writeHeader() {
    // Write out 50 zeros to the spi line (6 blocks of 8 followed by two single bit writes)
    mSPI.select();
    mSPI.template writeBit<0>(0);
    mSPI.writeByte(0);
    mSPI.writeByte(0);
    mSPI.writeByte(0);
    mSPI.template writeBit<0>(0);
    mSPI.writeByte(0);
    mSPI.writeByte(0);
    mSPI.writeByte(0);
    mSPI.waitFully();
    mSPI.release();
  }

public:
  SM16716Controller() {}

  virtual void init() {
    mSPI.init();
  }

protected:

  virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
    // Make sure the FLAG_START_BIT flag is set to ensure that an extra 1 bit is sent at the start
    // of each triplet of bytes for rgb data
    // writeHeader();
    mSPI.template writePixels<FLAG_START_BIT, DATA_NOP, RGB_ORDER>( pixels );
    writeHeader();
  }

};
/// @}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Clockless template instantiations - see clockless.h for how the timing values are used
//
// Base template for clockless controllers.  These controllers have 3 control points in their cycle for each bit.
// At T=0        : the line is raised hi to start a bit
// At T=T1       : the line is dropped low to transmit a zero bit
// At T=T1+T2    : the line is dropped low to transmit a one bit
// At T=T1+T2+T3 : the cycle is concluded (next bit can be sent)
//
// The units used for T1, T2, and T3 is nanoseconds.
// For 8MHz/16MHz/24MHz frequencies, these values are also guaranteed
// to be integral multiples of an 8MHz clock (125ns increments).
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef FASTLED_HAS_CLOCKLESS
/// @name clockless controllers
/// Provides timing definitions for the variety of clockless controllers supplied by the library.
/// @{

// Allow clock that clockless controller is based on to have different
// frequency than the CPU.
#if !defined(CLOCKLESS_FREQUENCY)
    #define CLOCKLESS_FREQUENCY F_CPU
#endif

// We want to force all avr's to use the Trinket controller when running at 8Mhz, because even the 328's at 8Mhz
// need the more tightly defined timeframes.
#if (CLOCKLESS_FREQUENCY == 8000000 || CLOCKLESS_FREQUENCY == 16000000 || CLOCKLESS_FREQUENCY == 24000000) //  || CLOCKLESS_FREQUENCY == 48000000 || CLOCKLESS_FREQUENCY == 96000000) // 125ns/clock
#define FMUL (CLOCKLESS_FREQUENCY/8000000)

// GE8822
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GE8822Controller800Khz : public ClocklessController<DATA_PIN, 3 * FMUL, 5 * FMUL, 3 * FMUL, RGB_ORDER, 4> {};

// LPD1886
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 3 * FMUL, 2 * FMUL, RGB_ORDER, 4> {};

// LPD1886
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz_8bit : public ClocklessController<DATA_PIN, 2 * FMUL, 3 * FMUL, 2 * FMUL, RGB_ORDER> {};

// WS2811@800khz 2 clocks, 5 clocks, 3 clocks
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2812Controller800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 5 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2811Controller800Khz : public ClocklessController<DATA_PIN, 3 * FMUL, 4 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>                                                             //not tested
class WS2813Controller : public ClocklessController<DATA_PIN, 3 * FMUL, 4 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2811Controller400Khz : public ClocklessController<DATA_PIN, 4 * FMUL, 10 * FMUL, 6 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6822Controller : public ClocklessController<DATA_PIN, 3 * FMUL, 8 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16703Controller : public ClocklessController<DATA_PIN, 3 * FMUL, 4 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6812Controller : public ClocklessController<DATA_PIN, 3 * FMUL, 3 * FMUL, 4 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903Controller400Khz : public ClocklessController<DATA_PIN, 4 * FMUL, 12 * FMUL, 4 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903BController800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 4 * FMUL, 4 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1904Controller800Khz : public ClocklessController<DATA_PIN, 3 * FMUL, 3 * FMUL, 4 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS2903Controller : public ClocklessController<DATA_PIN, 2 * FMUL, 6 * FMUL, 2 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1809Controller800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 5 * FMUL, 3 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1803Controller400Khz : public ClocklessController<DATA_PIN, 6 * FMUL, 9 * FMUL, 6 * FMUL, RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 5 * FMUL, 3 * FMUL, RGB_ORDER, 0, true, 500> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller400Khz : public ClocklessController<DATA_PIN, 6 * FMUL, 7 * FMUL, 6 * FMUL, RGB_ORDER, 4> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 4 * FMUL, 4 * FMUL, RGB_ORDER, 4> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class PL9823Controller : public ClocklessController<DATA_PIN, 3 * FMUL, 8 * FMUL, 3 * FMUL, RGB_ORDER> {};

#else

// Similar to NS() macro, this calculates the number of cycles for
// the clockless chipset (which may differ from CPU cycles)

#ifdef FASTLED_TEENSY4
// just use raw nanosecond values for the teensy4
#define C_NS(_NS) _NS
#else
#define C_NS(_NS) (((_NS * ((CLOCKLESS_FREQUENCY / 1000000L)) + 999)) / 1000)
#endif

// GE8822 - 350ns 660ns 350ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GE8822Controller800Khz : public ClocklessController<DATA_PIN, C_NS(350), C_NS(660), C_NS(350), RGB_ORDER, 4> {};

// GW6205@400khz - 800ns, 800ns, 800ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller400Khz : public ClocklessController<DATA_PIN, C_NS(800), C_NS(800), C_NS(800), RGB_ORDER, 4> {};

// GW6205@400khz - 400ns, 400ns, 400ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller800Khz : public ClocklessController<DATA_PIN, C_NS(400), C_NS(400), C_NS(400), RGB_ORDER, 4> {};

// UCS1903 - 500ns, 1500ns, 500ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903Controller400Khz : public ClocklessController<DATA_PIN, C_NS(500), C_NS(1500), C_NS(500), RGB_ORDER> {};

// UCS1903B - 400ns, 450ns, 450ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903BController800Khz : public ClocklessController<DATA_PIN, C_NS(400), C_NS(450), C_NS(450), RGB_ORDER> {};

// UCS1904 - 400ns, 400ns, 450ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1904Controller800Khz : public ClocklessController<DATA_PIN, C_NS(400), C_NS(400), C_NS(450), RGB_ORDER> {};

// UCS2903 - 250ns, 750ns, 250ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS2903Controller : public ClocklessController<DATA_PIN, C_NS(250), C_NS(750), C_NS(250), RGB_ORDER> {};

// TM1809 - 350ns, 350ns, 550ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1809Controller800Khz : public ClocklessController<DATA_PIN, C_NS(350), C_NS(350), C_NS(450), RGB_ORDER> {};

// WS2811 - 320ns, 320ns, 640ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2811Controller800Khz : public ClocklessController<DATA_PIN, C_NS(320), C_NS(320), C_NS(640), RGB_ORDER> {};

// WS2813 - 320ns, 320ns, 640ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2813Controller : public ClocklessController<DATA_PIN, C_NS(320), C_NS(320), C_NS(640), RGB_ORDER> {};

// WS2812 - 250ns, 625ns, 375ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2812Controller800Khz : public ClocklessController<DATA_PIN, C_NS(250), C_NS(625), C_NS(375), RGB_ORDER> {};

// WS2811@400khz - 800ns, 800ns, 900ns
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2811Controller400Khz : public ClocklessController<DATA_PIN, C_NS(800), C_NS(800), C_NS(900), RGB_ORDER> {};

// 750NS, 750NS, 750NS
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1803Controller400Khz : public ClocklessController<DATA_PIN, C_NS(700), C_NS(1100), C_NS(700), RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller800Khz : public ClocklessController<DATA_PIN, C_NS(340), C_NS(340), C_NS(550), RGB_ORDER, 0, true, 500> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller1600Khz : public ClocklessController<DATA_PIN, C_NS(100), C_NS(300), C_NS(200), RGB_ORDER, 0, true, 500> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz : public ClocklessController<DATA_PIN, C_NS(200), C_NS(400), C_NS(200), RGB_ORDER, 4> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz_8bit : public ClocklessController<DATA_PIN, C_NS(200), C_NS(400), C_NS(200), RGB_ORDER> {};


template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6822Controller : public ClocklessController<DATA_PIN, C_NS(375), C_NS(1000), C_NS(375), RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6812Controller : public ClocklessController<DATA_PIN, C_NS(300), C_NS(300), C_NS(600), RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16703Controller : public ClocklessController<DATA_PIN, C_NS(300), C_NS(600), C_NS(300), RGB_ORDER> {};

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class PL9823Controller : public ClocklessController<DATA_PIN, C_NS(350), C_NS(1010), C_NS(350), RGB_ORDER> {};
#endif
///@}

#endif
///@}
FASTLED_NAMESPACE_END

#endif
