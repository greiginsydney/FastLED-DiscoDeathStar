/*
 * Effect implementation for the GauntletII project.
 * https://bitbucket.org/ratkins/ledeffects/src/default/
 * 
 * Modified by Greig for the Disco Death Star's XY (serpentine) layout.
 */

#include "Effect.h"

class Snake : public Effect {
private:
    
    static const byte SNAKE_LENGTH = 60;
    
    enum Direction {
        goUP, goDOWN, goLEFT, goRIGHT
    };
    
    struct Pixel {
        uint8_t x;
        uint8_t y;
    };
    
    CRGB colours[SNAKE_LENGTH];
    uint8_t initialHue;
    
    Pixel pixels[SNAKE_LENGTH];
    
    Direction direction;
    
    void newDirection() {
        switch (direction) {
            case goUP:
            case goDOWN:
                direction = random(0, 2) == 1 ? goRIGHT : goLEFT;
                break;
                
            case goLEFT:
            case goRIGHT:
                direction = random(0, 2) == 1 ? goDOWN : goUP;
                
            default:
                break;
        }
    }
    
    void shuffleDown() {
        for (byte i = SNAKE_LENGTH - 1; i > 0; i--) {
            pixels[i] = pixels[i - 1];
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
        uint8_t reverseY = (height - 1) - y;
        i = (x * height) + reverseY;
      }
      else
      {
        // Even columns run top to bottom
        i = (x * height) + y;
      }
      return i;
    }
    
public:
    Snake(CRGB *leds, int width, int height) : Effect(leds, width, height), initialHue(0) {
        direction = goUP;
        for (int i = 0; i < SNAKE_LENGTH; i++) {
            pixels[i].x = 0;
            pixels[i].y = 0;
        }
    }
    
    void start() {
        clearLeds();
       // for (int frameNo = 0; frameNo < 1000; frameNo++) {
            shuffleDown();
            if (random(10) > 6) {
                newDirection();
            }
            switch (direction) {
                case goDOWN:
                    pixels[0].y = (pixels[0].y + 1) % height;
                    break;
                case goLEFT:
                    pixels[0].x = (pixels[0].x + 1) % width;
                    break;
                case goUP:
                    pixels[0].y = pixels[0].y == 0 ? height - 1 : pixels[0].y - 1;
                    break;
                case goRIGHT:
                    pixels[0].x = pixels[0].x == 0 ? width - 1 : pixels[0].x - 1;
                    break;
            }
            fill_rainbow(colours, SNAKE_LENGTH, initialHue++);
            for (byte i = 0; i < SNAKE_LENGTH; i++) {
                leds[XY(pixels[i].x, pixels[i].y)] = colours[i] %= (255 - i * (255 / SNAKE_LENGTH));
            }
       //     LEDS.show();
       //     FastLED.delay(30); 
        //    clearLeds();
      //  }
    }
};
