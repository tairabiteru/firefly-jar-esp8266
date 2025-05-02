#include <AceRoutine.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266TrueRandom.h>


/*!
  @brief  Convert Red Green and Blue values based on a single
          brightness value. This allows us to use a single value
          to compute brightness of all LEDs maintaining the same
          color. The multipliers are all values from 0 - 1 which
          correspond to a color. They can be found by finding the
          RGB values of a color, and dividing each value by 255.
  @param  brightness  The brightness value from 0 to 255.
  @param  r_mult  The red channel multiplier.
  @param  g_mult  The green channel multiplier.
  @param  b_mult  The blue channel multiplier.
  @return uint32_t color value, compatible with 
          Adafruit_NeoPixel.setColor().
*/
uint32_t compute_rgb(uint8_t brightness, float r_mult, float g_mult, float b_mult) {
    int r = brightness * r_mult;
    int g = brightness * g_mult;
    int b = brightness * b_mult;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}


/*!
  @brief  Find the correct RGB value given the brightness
          corresponding to the peak emission spectrum of
          562 nm. This is meant to closely match the color
          of Photinus Pyralis, the Common Eastern Firefly.
  @param  brightness  The brightness value from 0 to 255.
  @return uint32_t color value, compatible with 
          Adafruit_NeoPixel.setColor().
*/
uint32_t p_pyralis_brightness(uint8_t brightness) {
    // Corresponds to approx 562 nm, rgb(201, 255, 0)
    return compute_rgb(brightness, 0.788, 1.0, 0.0);
}


/*!
    @brief  Class extending ace_routine::Coroutine implementing
            a single Firefly. The entire purpose of doing this that
            we want the microcontroller to be able to control all of
            the LEDs at independently. We want the fireflies to be
            unique, you know? The ESP8266 isn't capable of true
            multithreading, so we use coroutines instead.
*/
class Firefly: public ace_routine::Coroutine {
    private:
      int brightness;
      bool rising;

      int dark_delay;
      int rising_delay;
      int falling_delay;

    public:
      int number;
      Adafruit_NeoPixel pixels;

      // Constructor: Taking the number in sequence of the LED to control,
      // and the instance of Adafruit_NeoPixel that it belongs to.
      // This is zero indexed, by the way.
      Firefly(int number, Adafruit_NeoPixel& pixels) {
        this->number = number;
        this->pixels = pixels;
        this->brightness = 0;
        this->rising = true;
      }

      /*!
        @brief Re-rolls the randomness values of the firefly.
      */
      void roll() {
        this->dark_delay = ESP8266TrueRandom.random(4000, 7000);
        this->rising_delay = ESP8266TrueRandom.random(1000, 1300);
        this->falling_delay = ESP8266TrueRandom.random(1500, 2000);
      }

      /*!
        @brief The coroutine to be run. 
               This method is called in the main loop.
      */
      int runCoroutine() override {
        // Each call of this method results in its execution one time.
        // To understand how this works, you have to wrap your head
        // around that. What this is implementing, when called over
        // and over, is essentially a very specialized fade.
        COROUTINE_LOOP() {
            // Each iteration, we check to see if we're in the rising phase.
            // If we are, the brightness value is incremented. Otherwise, 
            // it's decremented.
            if (this->rising) {
                this->brightness++;
            } else {
                this->brightness--;
            }

            // Here we compute the brightness value and display it.
            // The function which does this is of course specific to P. Pyralis, 
            // but it could be swapped out for another kind color. Additionally, 
            // we only actually show the output if (arbitrarily) the dark_delay
            //value is even. This introduces a bit more variability.
            if (dark_delay % 2 == 0) {
              this->pixels.setPixelColor(this->number, p_pyralis_brightness(this->brightness));
              this->pixels.show();
            }
            
            // Next, we check to see if we've reached the maximum brightness
            // value during the rising phase. If we have, the rising phase is
            // over, and we will now begin falling.
            if (this->brightness == 255 && this->rising) {
                this->rising = false;
            // Otherwise we check to see if we've reached the end of the
            // falling phase. If we have, we re-roll the random values,
            // and the rising phase begins again after a random delay.
            } else if (this->brightness == 0 && this->rising == false) {
                this->rising = true;
                this->roll();
                COROUTINE_DELAY(this->dark_delay);
            }
            
            // Finally we introduce a shot-delay based on whether or not
            // we're rising or falling. This is because P. Pyralis does not
            // spend the same amount of time rising as it does falling.
            // To more closely match it, we also do the same.
            if (this->rising) {
                COROUTINE_DELAY_MICROS(this->rising_delay);
            } else {
                COROUTINE_DELAY_MICROS(this->falling_delay);
            }
        }
      }
};