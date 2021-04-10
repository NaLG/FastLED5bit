#ifndef __INC_CONTROLLER_H
#define __INC_CONTROLLER_H

///@file controller.h
/// base definitions used by led controllers for writing out led data

#include "FastLED.h"
#include "led_sysdefs.h"
#include "pixeltypes.h"
#include "color.h"
#include <stddef.h>

FASTLED_NAMESPACE_BEGIN

#define RO(X) RGB_BYTE(RGB_ORDER, X)
#define RGB_BYTE(RO,X) (((RO)>>(3*(2-(X)))) & 0x3)

#define RGB_BYTE0(RO) ((RO>>6) & 0x3)
#define RGB_BYTE1(RO) ((RO>>3) & 0x3)
#define RGB_BYTE2(RO) ((RO) & 0x3)

// operator byte *(struct CRGB[] arr) { return (byte*)arr; }

#define DISABLE_DITHER 0x00
#define BINARY_DITHER 0x01
typedef uint8_t EDitherMode;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LED Controller interface definition
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Base definition for an LED controller.  Pretty much the methods that every LED controller object will make available.
/// Note that the showARGB method is not impelemented for all controllers yet.   Note also the methods for eventual checking
/// of background writing of data (I'm looking at you, teensy 3.0 DMA controller!).  If you want to pass LED controllers around
/// to methods, make them references to this type, keeps your code saner.  However, most people won't be seeing/using these objects
/// directly at all
class CLEDController {
protected:
    friend class CFastLED;
    CRGB *m_Data;
    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
      uint8_t *b_Data;
      #if FASTLED_USE_16_BIT_PIXELS == 1
        uint8_t *b0_Data;
        uint8_t *b1_Data;
        uint8_t *b2_Data;
        CRGB *m2_Data;
      #endif
    #endif
    CLEDController *m_pNext;
    CRGB m_ColorCorrection;
    CRGB m_ColorTemperature;
    EDitherMode m_DitherMode;
    int m_nLeds;
    static CLEDController *m_pHead;
    static CLEDController *m_pTail;

    /// set all the leds on the controller to a given color
    ///@param data the crgb color to set the leds to
    ///@param nLeds the numner of leds to set to this color
    ///@param scale the rgb scaling value for outputting color
    virtual void showColor(const struct CRGB & data, int nLeds, CRGB scale) = 0;

  /// write the passed in rgb data out to the leds managed by this controller
  ///@param data the rgb data to write out to the strip
  ///@param nLeds the number of leds being written out
  ///@param scale the rgb scaling to apply to each led before writing it out
    virtual void show(const struct CRGB *data, int nLeds, CRGB scale) = 0;

    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
      virtual void show(const struct CRGB *data, const uint8_t *bdata, int nLeds, CRGB scale) = 0;
    #if FASTLED_USE_16_BIT_PIXELS == 1
      virtual void show(const struct CRGB *data, const struct CRGB *dataLo, const uint8_t *b0data, const uint8_t *b1data, const uint8_t *b2data, int nLeds, CRGB scale) = 0;
    #endif
    #endif

public:
  /// create an led controller object, add it to the chain of controllers
    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
      CLEDController() : m_Data(NULL), b_Data(NULL), m_ColorCorrection(UncorrectedColor), m_ColorTemperature(UncorrectedTemperature), m_DitherMode(BINARY_DITHER), m_nLeds(0) 
    #else
      CLEDController() : m_Data(NULL), m_ColorCorrection(UncorrectedColor), m_ColorTemperature(UncorrectedTemperature), m_DitherMode(BINARY_DITHER), m_nLeds(0) 
    #endif
    {
        m_pNext = NULL;
        if(m_pHead==NULL) { m_pHead = this; }
        if(m_pTail != NULL) { m_pTail->m_pNext = this; }
        m_pTail = this;
    }

  ///initialize the LED controller
  virtual void init() = 0;

  ///clear out/zero out the given number of leds.
  virtual void clearLeds(int nLeds) { showColor(CRGB::Black, nLeds, CRGB::Black); }

    /// show function w/integer brightness, will scale for color correction and temperature
    void show(const struct CRGB *data, int nLeds, uint8_t brightness) {
        show(data, nLeds, getAdjustment(brightness));
    }

    /// show function w/integer brightness, will scale for color correction and temperature
    void showColor(const struct CRGB &data, int nLeds, uint8_t brightness) {
        showColor(data, nLeds, getAdjustment(brightness));
    }

    /// show function using the "attached to this controller" led data
    void showLeds(uint8_t brightness=255) {
      #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
        #if FASTLED_USE_16_BIT_PIXELS == 1
          // Serial.write("showledingPP!\n");
          show(m_Data, m2_Data, b0_Data, b1_Data, b2_Data, m_nLeds, getAdjustment(brightness));
        #else
          show(m_Data, b_Data, m_nLeds, getAdjustment(brightness));
        #endif
      #else
        // Serial.write("showledingnotpp!\n");
        show(m_Data, m_nLeds, getAdjustment(brightness));
      #endif
    }


  /// show the given color on the led strip
    void showColor(const struct CRGB & data, uint8_t brightness=255) {
        showColor(data, m_nLeds, getAdjustment(brightness));
    }

    /// get the first led controller in the chain of controllers
    static CLEDController *head() { return m_pHead; }
    /// get the next controller in the chain after this one.  will return NULL at the end of the chain
    CLEDController *next() { return m_pNext; }

  /// set the default array of leds to be used by this controller
    CLEDController & setLeds(CRGB *data, int nLeds) {
        m_Data = data;
        m_nLeds = nLeds;
        return *this;
    }

    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
      CLEDController & setLeds(CRGB *data, uint8_t *bdata, int nLeds) {
          m_Data = data;
          b_Data = bdata;
          m_nLeds = nLeds;
          return *this;
      }
    #if FASTLED_USE_16_BIT_PIXELS == 1
      CLEDController & setLeds(CRGB *data, CRGB *dataLo, uint8_t *b0data, uint8_t *b1data, uint8_t *b2data, int nLeds) {
          m_Data = data;
          m2_Data = dataLo;
          b0_Data = b0data;
          b1_Data = b1data;
          b2_Data = b2data;
          m_nLeds = nLeds;
          return *this;
      }
    #endif
    #endif

  /// zero out the led data managed by this controller
    void clearLedData() {
        if(m_Data) {
            memset8((void*)m_Data, 0, sizeof(struct CRGB) * m_nLeds);
        }
    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
        if(b_Data) {
            memset8((void*)b_Data, 0, sizeof(uint8_t) * m_nLeds);
        }
    #endif
    }

    /// How many leds does this controller manage?
    virtual int size() { return m_nLeds; }

    /// Pointer to the CRGB array for this controller
    CRGB* leds() { return m_Data; }

    #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
      /// Pointer to the CRGB array for this controller
      uint8_t* led5bitBrights() { return b_Data; }
    #endif

    /// Reference to the n'th item in the controller
    CRGB &operator[](int x) { return m_Data[x]; }

  /// set the dithering mode for this controller to use
    inline CLEDController & setDither(uint8_t ditherMode = BINARY_DITHER) { m_DitherMode = ditherMode; return *this; }
    /// get the dithering option currently set for this controller
    inline uint8_t getDither() { return m_DitherMode; }

  /// the the color corrction to use for this controller, expressed as an rgb object
    CLEDController & setCorrection(CRGB correction) { m_ColorCorrection = correction; return *this; }
    /// set the color correction to use for this controller
    CLEDController & setCorrection(LEDColorCorrection correction) { m_ColorCorrection = correction; return *this; }
    /// get the correction value used by this controller
    CRGB getCorrection() { return m_ColorCorrection; }

  /// set the color temperature, aka white point, for this controller
    CLEDController & setTemperature(CRGB temperature) { m_ColorTemperature = temperature; return *this; }
    /// set the color temperature, aka white point, for this controller
    CLEDController & setTemperature(ColorTemperature temperature) { m_ColorTemperature = temperature; return *this; }
    /// get the color temperature, aka whipe point, for this controller
    CRGB getTemperature() { return m_ColorTemperature; }

  /// Get the combined brightness/color adjustment for this controller
    CRGB getAdjustment(uint8_t scale) {
        // Serial.write("getadj\n");        
        return computeAdjustment(scale, m_ColorCorrection, m_ColorTemperature);
    }

    static CRGB computeAdjustment(uint8_t scale, const CRGB & colorCorrection, const CRGB & colorTemperature) {
      #if defined(NO_CORRECTION) && (NO_CORRECTION==1)
              return CRGB(scale,scale,scale);
      #else
              CRGB adj(0,0,0);

              if(scale > 0) {
                  for(uint8_t i = 0; i < 3; i++) {
                      uint8_t cc = colorCorrection.raw[i];
                      uint8_t ct = colorTemperature.raw[i];
                      if(cc > 0 && ct > 0) {
                          uint32_t work = (((uint32_t)cc)+1) * (((uint32_t)ct)+1) * scale;
                          work /= 0x10000L;
                          adj.raw[i] = work & 0xFF;
                      }
                  }
              }

              return adj;
      #endif
    }
    virtual uint16_t getMaxRefreshRate() const { return 0; }
};

// Pixel controller class.  This is the class that we use to centralize pixel access in a block of data, including
// support for things like RGB reordering, scaling, dithering, skipping (for ARGB data), and eventually, we will
// centralize 8/12/16 conversions here as well.
template<EOrder RGB_ORDER, int LANES=1, uint32_t MASK=0xFFFFFFFF>
struct PixelController {
        const uint8_t *mData;
         #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
          #if FASTLED_USE_16_BIT_PIXELS == 1  // nlg
           const uint8_t *m2Data; //lo byte val
           const uint8_t *b0Data; //brightness 0
           const uint8_t *b1Data; //brightness 1
           const uint8_t *b2Data; //brightness 2
           int8_t m2Advance;
          
          const uint8_t *bData; //brightness
          #endif
          int8_t bAdvance;
         #endif
        int mLen,mLenRemaining;
        uint8_t d[3];
        uint8_t e[3];
        CRGB mScale;
        int8_t mAdvance;
        int mOffsets[LANES];

        PixelController(const PixelController & other) {
            d[0] = other.d[0];
            d[1] = other.d[1];
            d[2] = other.d[2];
            e[0] = other.e[0];
            e[1] = other.e[1];
            e[2] = other.e[2];
            mData = other.mData;
            mScale = other.mScale;
            mAdvance = other.mAdvance;
            #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
              bData = other.bData;
              #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
                b0Data = other.b0Data;
                b1Data = other.b1Data;
                b2Data = other.b2Data;
                m2Data = other.m2Data;
                m2Advance = other.m2Advance;
              #endif
              bAdvance = other.bAdvance;
            #endif
            mLenRemaining = mLen = other.mLen;
            for(int i = 0; i < LANES; i++) { mOffsets[i] = other.mOffsets[i]; }

        }

        void initOffsets(int len) {
          int nOffset = 0;
          for(int i = 0; i < LANES; i++) {
            mOffsets[i] = nOffset;
            if((1<<i) & MASK) { nOffset += (len * mAdvance); }
          }
        }

        PixelController(const uint8_t *d, int len, CRGB & s, EDitherMode dither = BINARY_DITHER, bool advance=true, uint8_t skip=0) : mData(d), mLen(len), mLenRemaining(len), mScale(s) {
            enable_dithering(dither);
            mData += skip;
            mAdvance = (advance) ? 3+skip : 0;
            initOffsets(len);
        }

        #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
          PixelController(const CRGB *d, const uint8_t *b, int len, CRGB & s, EDitherMode dither = BINARY_DITHER) : mData((const uint8_t*)d), bData(b), mLen(len), mLenRemaining(len), mScale(s) {
              enable_dithering(dither);
              mAdvance = 3;
              bAdvance = sizeof(uint8_t); // 
              initOffsets(len);
          }
        #endif
        #if FASTLED_USE_16_BIT_PIXELS == 1  // nlg
          PixelController(const CRGB *d, const CRGB *dl, const uint8_t *b0, const uint8_t *b1, const uint8_t *b2, int len, CRGB & s, EDitherMode dither = BINARY_DITHER) : mData((const uint8_t*)d), m2Data((const uint8_t*)dl), b0Data(b0), b1Data(b1), b2Data(b2), mLen(len), mLenRemaining(len), mScale(s) {
              enable_dithering(dither);
              mAdvance = 3;
              m2Advance = 3;
              bAdvance = sizeof(uint8_t); // 
              initOffsets(len);
          }
        #endif
        PixelController(const CRGB *d, int len, CRGB & s, EDitherMode dither = BINARY_DITHER) : mData((const uint8_t*)d), mLen(len), mLenRemaining(len), mScale(s) {
            enable_dithering(dither);
            mAdvance = 3;
            initOffsets(len);
        }

        PixelController(const CRGB &d, int len, CRGB & s, EDitherMode dither = BINARY_DITHER) : mData((const uint8_t*)&d), mLen(len), mLenRemaining(len), mScale(s) {
            enable_dithering(dither);
            mAdvance = 0;
            initOffsets(len);
        }

        void init_binary_dithering() {
#if !defined(NO_DITHERING) || (NO_DITHERING != 1)

            // Set 'virtual bits' of dithering to the highest level
            // that is not likely to cause excessive flickering at
            // low brightness levels + low update rates.
            // These pre-set values are a little ambitious, since
            // a 400Hz update rate for WS2811-family LEDs is only
            // possible with 85 pixels or fewer.
            // Once we have a 'number of milliseconds since last update'
            // value available here, we can quickly calculate the correct
            // number of 'virtual bits' on the fly with a couple of 'if'
            // statements -- no division required.  At this point,
            // the division is done at compile time, so there's no runtime
            // cost, but the values are still hard-coded.
#define MAX_LIKELY_UPDATE_RATE_HZ     400
#define MIN_ACCEPTABLE_DITHER_RATE_HZ  50
#define UPDATES_PER_FULL_DITHER_CYCLE (MAX_LIKELY_UPDATE_RATE_HZ / MIN_ACCEPTABLE_DITHER_RATE_HZ)
#define RECOMMENDED_VIRTUAL_BITS ((UPDATES_PER_FULL_DITHER_CYCLE>1) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>2) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>4) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>8) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>16) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>32) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>64) + \
                                  (UPDATES_PER_FULL_DITHER_CYCLE>128) )
#define VIRTUAL_BITS RECOMMENDED_VIRTUAL_BITS

            // R is the digther signal 'counter'.
            static uint8_t R = 0;
            R++;

            // R is wrapped around at 2^ditherBits,
            // so if ditherBits is 2, R will cycle through (0,1,2,3)
            uint8_t ditherBits = VIRTUAL_BITS;
            R &= (0x01 << ditherBits) - 1;

            // Q is the "unscaled dither signal" itself.
            // It's initialized to the reversed bits of R.
            // If 'ditherBits' is 2, Q here will cycle through (0,128,64,192)
            uint8_t Q = 0;

            // Reverse bits in a byte
            {
                if(R & 0x01) { Q |= 0x80; }
                if(R & 0x02) { Q |= 0x40; }
                if(R & 0x04) { Q |= 0x20; }
                if(R & 0x08) { Q |= 0x10; }
                if(R & 0x10) { Q |= 0x08; }
                if(R & 0x20) { Q |= 0x04; }
                if(R & 0x40) { Q |= 0x02; }
                if(R & 0x80) { Q |= 0x01; }
            }

            // Now we adjust Q to fall in the center of each range,
            // instead of at the start of the range.
            // If ditherBits is 2, Q will be (0, 128, 64, 192) at first,
            // and this adjustment makes it (31, 159, 95, 223).
            if( ditherBits < 8) {
                Q += 0x01 << (7 - ditherBits);
            }

            // D and E form the "scaled dither signal"
            // which is added to pixel values to affect the
            // actual dithering.

            // Setup the initial D and E values
            for(int i = 0; i < 3; i++) {
                    uint8_t s = mScale.raw[i];
                    e[i] = s ? (256/s) + 1 : 0;
                    d[i] = scale8(Q, e[i]);
#if (FASTLED_SCALE8_FIXED == 1)
                    if(d[i]) (d[i]--);
#endif
                    if(e[i]) e[i]--;
            }
#endif
        }

        // Do we have n pixels left to process?
        __attribute__((always_inline)) inline bool has(int n) {
            return mLenRemaining >= n;
        }

        // toggle dithering enable
        void enable_dithering(EDitherMode dither) {
            switch(dither) {
                case BINARY_DITHER: init_binary_dithering(); break;
                default: d[0]=d[1]=d[2]=e[0]=e[1]=e[2]=0; break;
            }
        }

        __attribute__((always_inline)) inline int size() { return mLen; }

        // get the amount to advance the pointer by
        __attribute__((always_inline)) inline int advanceBy() { return mAdvance; }

        // advance the data pointer forward, adjust position counter
         #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
           #if FASTLED_USE_16_BIT_PIXELS == 1  // nlg
             __attribute__((always_inline)) inline void advanceData() { mData += mAdvance; m2Data += m2Advance; b0Data += bAdvance; b1Data += bAdvance; b2Data += bAdvance; mLenRemaining--;}
           #else
             __attribute__((always_inline)) inline void advanceData() { mData += mAdvance; bData += bAdvance; mLenRemaining--;}
           #endif
         #else
           __attribute__((always_inline)) inline void advanceData() { mData += mAdvance; mLenRemaining--;}
         #endif

        // step the dithering forward
         __attribute__((always_inline)) inline void stepDithering() {
             // IF UPDATING HERE, BE SURE TO UPDATE THE ASM VERSION IN
             // clockless_trinket.h!
                d[0] = e[0] - d[0];
                d[1] = e[1] - d[1];
                d[2] = e[2] - d[2];
        }

        // Some chipsets pre-cycle the first byte, which means we want to cycle byte 0's dithering separately
        __attribute__((always_inline)) inline void preStepFirstByteDithering() {
            d[RO(0)] = e[RO(0)] - d[RO(0)];
        }

        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadByte(PixelController & pc) { return pc.mData[RO(SLOT)]; }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadByte(PixelController & pc, int lane) { return pc.mData[pc.mOffsets[lane] + RO(SLOT)]; }
        #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1  // nlg
          __attribute__((always_inline)) inline uint8_t get5bitBright(PixelController & pc) { return pc.bData[0]; }
          __attribute__((always_inline)) inline uint8_t get5bitBright() { return get5bitBright(*this); }
          __attribute__((always_inline)) inline uint8_t get5bitBright0(PixelController & pc) { return pc.b0Data[0]; }
          __attribute__((always_inline)) inline uint8_t get5bitBright0() { return get5bitBright0(*this); }
          __attribute__((always_inline)) inline uint8_t get5bitBright1(PixelController & pc) { return pc.b1Data[0]; }
          __attribute__((always_inline)) inline uint8_t get5bitBright1() { return get5bitBright1(*this); }
          __attribute__((always_inline)) inline uint8_t get5bitBright2(PixelController & pc) { return pc.b2Data[0]; }
          __attribute__((always_inline)) inline uint8_t get5bitBright2() { return get5bitBright2(*this); }
        #if FASTLED_USE_16_BIT_PIXELS == 1  // nlg
          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadByteLo(PixelController & pc) { return pc.m2Data[RO(SLOT)]; }
          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadByteLo(PixelController & pc, int lane) { return pc.m2Data[pc.mOffsets[lane] + RO(SLOT)]; }

          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScaleLo(PixelController & pc) { return scale<SLOT>(pc, pc.dither<SLOT>(pc, pc.loadByteLo<SLOT>(pc))); }
          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScaleLo(PixelController & pc, int lane) { return scale<SLOT>(pc, pc.dither<SLOT>(pc, pc.loadByteLo<SLOT>(pc, lane))); }
          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScaleLo(PixelController & pc, int lane, uint8_t d, uint8_t scale) { return scale8(pc.dither<SLOT>(pc, pc.loadByteLo<SLOT>(pc, lane), d), scale); }
          template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScaleLo(PixelController & pc, int lane, uint8_t scale) { return scale8(pc.loadByteLo<SLOT>(pc, lane), scale); }

          __attribute__((always_inline)) inline uint8_t loadAndScale0Lo(int lane, uint8_t scale) { return loadAndScaleLo<0>(*this, lane, scale); }
          __attribute__((always_inline)) inline uint8_t loadAndScale1Lo(int lane, uint8_t scale) { return loadAndScaleLo<1>(*this, lane, scale); }
          __attribute__((always_inline)) inline uint8_t loadAndScale2Lo(int lane, uint8_t scale) { return loadAndScaleLo<2>(*this, lane, scale); }
          __attribute__((always_inline)) inline uint8_t loadAndScale0Lo(int lane) { return loadAndScaleLo<0>(*this, lane); }
          __attribute__((always_inline)) inline uint8_t loadAndScale1Lo(int lane) { return loadAndScaleLo<1>(*this, lane); }
          __attribute__((always_inline)) inline uint8_t loadAndScale2Lo(int lane) { return loadAndScaleLo<2>(*this, lane); }
        #endif
        #endif

        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t dither(PixelController & pc, uint8_t b) { return b ? qadd8(b, pc.d[RO(SLOT)]) : 0; }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t dither(PixelController & , uint8_t b, uint8_t d) { return b ? qadd8(b,d) : 0; }

        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t scale(PixelController & pc, uint8_t b) { return scale8(b, pc.mScale.raw[RO(SLOT)]); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t scale(PixelController & , uint8_t b, uint8_t scale) { return scale8(b, scale); }

        // composite shortcut functions for loading, dithering, and scaling
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScale(PixelController & pc) { return scale<SLOT>(pc, pc.dither<SLOT>(pc, pc.loadByte<SLOT>(pc))); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScale(PixelController & pc, int lane) { return scale<SLOT>(pc, pc.dither<SLOT>(pc, pc.loadByte<SLOT>(pc, lane))); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScale(PixelController & pc, int lane, uint8_t d, uint8_t scale) { return scale8(pc.dither<SLOT>(pc, pc.loadByte<SLOT>(pc, lane), d), scale); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t loadAndScale(PixelController & pc, int lane, uint8_t scale) { return scale8(pc.loadByte<SLOT>(pc, lane), scale); }

        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t advanceAndLoadAndScale(PixelController & pc) { pc.advanceData(); return pc.loadAndScale<SLOT>(pc); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t advanceAndLoadAndScale(PixelController & pc, int lane) { pc.advanceData(); return pc.loadAndScale<SLOT>(pc, lane); }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t advanceAndLoadAndScale(PixelController & pc, int lane, uint8_t scale) { pc.advanceData(); return pc.loadAndScale<SLOT>(pc, lane, scale); }

        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t getd(PixelController & pc) { return pc.d[RO(SLOT)]; }
        template<int SLOT>  __attribute__((always_inline)) inline static uint8_t getscale(PixelController & pc) { return pc.mScale.raw[RO(SLOT)]; }

        // Helper functions to get around gcc stupidities
        __attribute__((always_inline)) inline uint8_t loadAndScale0(int lane, uint8_t scale) { return loadAndScale<0>(*this, lane, scale); }
        __attribute__((always_inline)) inline uint8_t loadAndScale1(int lane, uint8_t scale) { return loadAndScale<1>(*this, lane, scale); }
        __attribute__((always_inline)) inline uint8_t loadAndScale2(int lane, uint8_t scale) { return loadAndScale<2>(*this, lane, scale); }
        __attribute__((always_inline)) inline uint8_t advanceAndLoadAndScale0(int lane, uint8_t scale) { return advanceAndLoadAndScale<0>(*this, lane, scale); }
        __attribute__((always_inline)) inline uint8_t stepAdvanceAndLoadAndScale0(int lane, uint8_t scale) { stepDithering(); return advanceAndLoadAndScale<0>(*this, lane, scale); }

        __attribute__((always_inline)) inline uint8_t loadAndScale0(int lane) { return loadAndScale<0>(*this, lane); }
        __attribute__((always_inline)) inline uint8_t loadAndScale1(int lane) { return loadAndScale<1>(*this, lane); }
        __attribute__((always_inline)) inline uint8_t loadAndScale2(int lane) { return loadAndScale<2>(*this, lane); }
        __attribute__((always_inline)) inline uint8_t advanceAndLoadAndScale0(int lane) { return advanceAndLoadAndScale<0>(*this, lane); }
        __attribute__((always_inline)) inline uint8_t stepAdvanceAndLoadAndScale0(int lane) { stepDithering(); return advanceAndLoadAndScale<0>(*this, lane); }

        __attribute__((always_inline)) inline uint8_t loadAndScale0() { return loadAndScale<0>(*this); }
        __attribute__((always_inline)) inline uint8_t loadAndScale1() { return loadAndScale<1>(*this); }
        __attribute__((always_inline)) inline uint8_t loadAndScale2() { return loadAndScale<2>(*this); }
        __attribute__((always_inline)) inline uint8_t advanceAndLoadAndScale0() { return advanceAndLoadAndScale<0>(*this); }
        __attribute__((always_inline)) inline uint8_t stepAdvanceAndLoadAndScale0() { stepDithering(); return advanceAndLoadAndScale<0>(*this); }

        __attribute__((always_inline)) inline uint8_t getScale0() { return getscale<0>(*this); }
        __attribute__((always_inline)) inline uint8_t getScale1() { return getscale<1>(*this); }
        __attribute__((always_inline)) inline uint8_t getScale2() { return getscale<2>(*this); }
};

template<EOrder RGB_ORDER, int LANES=1, uint32_t MASK=0xFFFFFFFF> class CPixelLEDController : public CLEDController {
protected:
  virtual void showPixels(PixelController<RGB_ORDER,LANES,MASK> & pixels) = 0;

  /// set all the leds on the controller to a given color
  ///@param data the crgb color to set the leds to
  ///@param nLeds the numner of leds to set to this color
  ///@param scale the rgb scaling value for outputting color
  virtual void showColor(const struct CRGB & data, int nLeds, CRGB scale) {
    PixelController<RGB_ORDER, LANES, MASK> pixels(data, nLeds, scale, getDither());
    showPixels(pixels);
  }

  #if FASTLED_USE_PER_PIXEL_BRIGHTNESS == 1
/// write the passed in rgb data out to the leds managed by this controller
///@param data the rgb data to write out to the strip
///@param bdata the brightness data (5 bits) to write out to the strip
///@param nLeds the number of leds being written out
///@param scale the rgb scaling to apply to each led before writing it out
    virtual void show(const struct CRGB *data, const uint8_t *bdata, int nLeds, CRGB scale) {
      PixelController<RGB_ORDER, LANES, MASK> pixels(data, bdata, nLeds, scale, getDither());
      showPixels(pixels);
    }
  #endif
  #if FASTLED_USE_16_BIT_PIXELS == 1
/// write the passed in rgb data out to the leds managed by this controller
///@param data the rgb data to write out to the strip
///@param bdata the brightness data (5 bits) to write out to the strip
///@param nLeds the number of leds being written out
///@param scale the rgb scaling to apply to each led before writing it out
    virtual void show(const struct CRGB *data, const struct CRGB *dataLo, const uint8_t *b0data, const uint8_t *b1data, const uint8_t *b2data, int nLeds, CRGB scale) {
      PixelController<RGB_ORDER, LANES, MASK> pixels(data, dataLo, b0data, b1data, b2data, nLeds, scale, getDither());
      showPixels(pixels);
    }
  #endif
/// write the passed in rgb data out to the leds managed by this controller
///@param data the rgb data to write out to the strip
///@param nLeds the number of leds being written out
///@param scale the rgb scaling to apply to each led before writing it out
  virtual void show(const struct CRGB *data, int nLeds, CRGB scale) {
    PixelController<RGB_ORDER, LANES, MASK> pixels(data, nLeds, scale, getDither());
    showPixels(pixels);
  }

public:
  CPixelLEDController() : CLEDController() {}
};


FASTLED_NAMESPACE_END

#endif
