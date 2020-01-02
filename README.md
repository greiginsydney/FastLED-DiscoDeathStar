# FastLED - the Disco Death Star
My first FastLED project

This is a works in progress. Here it is on its side just after the LED strips have all been added:

![image](https://user-images.githubusercontent.com/11004787/61619872-097fba80-acb3-11e9-891c-ef28d879c3e3.png)

There are 2880 LEDs there - 96 metres of WS2812 LED strip.

8 IO pins from a Teensy 3.6 are feeding a total of 64 "columns" (horizontal in the image), each 1.5m or 45 LEDs deep. Each IO pin is driving 8 columns in a serpentine config, thus 360 LEDs.

The code contains example chases from a range of sources and hopefully I have fully attributed all authors.

It started as a hybrid of
- https://github.com/FastLED/FastLED/wiki/Parallel-Output
- https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples
- https://github.com/FastLED/FastLED/tree/master/examples/DemoReel100
- https://github.com/FastLED/FastLED/tree/master/examples/XYMatrix

... to which were added patterns, animations and chases from:
- https://github.com/evilgeniuslabs/torch/blob/master/torch.ino
- https://gist.github.com/StefanPetrick/c856b6d681ec3122e5551403aabfcc68
- https://bitbucket.org/ratkins/ledeffects/src/default/SinzianaPanel.ino

<br>

The whole saga is described in painful detail on the blog:
- [DHMG2019 – the Disco Death Star – Pt.1](https://greiginsydney.com/dhmg2019-dds-pt1/)
- [DHMG2019 – the Disco Death Star – Pt.2](https://greiginsydney.com/dhmg2019-dds-pt2/)

<br>
\- Greig.
