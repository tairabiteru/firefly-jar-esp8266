/*
Author: Taira

This program is designed to emulate the way that fireflies behave, so that
we can put them into a jar and look at them. The implementation is based
mainly on the emission spectrum and flash pattern of Photinus Pyralis,
the Common Eastern Firefly. (Since that's what's most common around
Michigan, where I live.) It's not perfect, primarily owing to the fact
that P. Pyralis makes a characteristic "J" shape when flashing, giving
it the nickname, "The Big Dipper Firefly." However, getting that to happen
with a matrix of LEDs seems uninspired, and I'm not putting servo motors
into a jar.

This is designed to be used with a set of WS2812B addressable LED 
"fairy lights." (The kind that come attached to really thin wires.)
In theory, other WS28xx family LEDs could be used too, but some things
might need to be changed.

The general gist of this is that we have some defined number of "fireflies,"
each one corresponding to one "pixel" or LED on the strip. There are two
primary objectives with this:

  1. Random, varied behavior.
  2. Independent control.

#1 is pretty easy using the ESP8266TrueRandom library.
#2 is a bit more difficult since we're trying to control multiple LEDs
independently, and at the same time. Multithreading? Sure, but not on
an ESP8266, they're too small and dumb. So instead we have to use
coroutines. If you know what coroutines are, I don't have to explain
myself, and if you don't: good. Avoid them.

(If you really care, look it up. I'm not going to do anyone any
service by explaining it in the comments here.)
*/

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <vector>

#include <firefly.hpp>

// Output pin for NeoPixels. D2 is GPIO4 on the ESP8266.
#define PIN       D2
// We define the number of "fireflies" we have in the jar.
#define NUMPIXELS 10

// Define the pixels. Some of these might need to be changed,
// depending on your specific use. In particular, NEO_BGR defines
// a blue-green-red channel order. Other LEDs might be different.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_BGR + NEO_KHZ800);

// Vector to hold our fireflies.
std::vector<Firefly *> jar;

/*!
    @brief Setup function. This runs once before the microcontroller
           enters the main loop.
*/
void setup() {
    // Initialize pixels.
    pixels.begin();
    
    // Start by turning all of our pixels off, and then
    // create a new Firefly for each one, pushing it
    // into the "jar" vector.
    for (int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();

        jar.push_back(new Firefly(i, pixels));
    }
}


/*!
    @brief Main loop. This runs forever, and basically
           as fast as the microcontroller can run it.
*/
void loop() {
    // For each firefly in our jar, we want to run its coroutine
    // defined in firefly.hpp.
    for (auto firefly : jar) {
        firefly->runCoroutine();
    }
}