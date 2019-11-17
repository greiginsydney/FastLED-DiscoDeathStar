

void testPattern()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
  
  //A static corkscrew around the pole. Confirms column rotation is correct
  //turn everything OFF:
  // This outer loop will go over each column
  for (uint8_t x = 0; x < kMatrixWidth; x++) 
  {
    // This inner loop will go over each row
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      if (x == y)
      {
        leds[ XY( x, y) ] = CRGB::Cyan;//CHSV( random8(), 255, 255);
      }
      else
      {
        leds[ XY( x, y)] = CRGB::Black;
      }
    }
  }
  LEDS.show();
  LEDS.delay(10000/FRAMES_PER_SECOND);
}


void simpleCorkscrew()
{
  if (PATTERN_MODE != 0) { nextPattern(); return; }
  
  //ROTATE the Corkscrew around the pole.
  int bx = 0;
  int by = 0;
  bx += corkScrew_x;
  if (bx >= kMatrixWidth) {bx = 0;}
  int StopAt = 0; //Initalise
  // This loop turns the LEDs on:
  do 
  {
    leds[ XY( bx, by) ] = CHSV( random8(), 255, 255);
    bx++;
    by++;
    if (by >= kMatrixHeight){ StopAt++;}
    if (bx >= kMatrixWidth) {bx = 0;}
    if (by >= kMatrixHeight){by = 0;}
    if (StopAt >= 1) {break ;}
  } while (true);
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND);
  // While this one turns them off:
  bx = corkScrew_x;
  if (bx >= kMatrixWidth) {bx = 0;}
  StopAt = 0; //Reinitalise
  do 
  {
    leds[ XY( bx, by) ] = CRGB::Black;
    bx++;
    by++;
    if (by >= kMatrixHeight){ StopAt++;}
    if (bx >= kMatrixWidth) {bx = 0;}
    if (by >= kMatrixHeight){by = 0;}
    if (StopAt >= 1) {break ;}
  } while (true);
  corkScrew_x++;
  if (corkScrew_x >= kMatrixWidth)
  {
    corkScrew_x = 0;
  }
}


void trailingCorkscrew()
{
  
  //if (PATTERN_MODE != 0) { nextPattern(); return; } //Skip this pattern if mode is not Manual
  
  int bx = 0;
  int by = 0;
  bx += corkScrew_x;
  if (bx >= kMatrixWidth) {bx = 0;}
  int StopAt = 0; //Initalise
  Serial.println("entering ON loop");
  // This loop turns the LEDs on:
  do 
  {
    leds[ XY( bx, by) ] = CHSV( random8(), 255, 255);
    bx++;
    by++;
    if (by >= kMatrixHeight){ StopAt++;}
    if (bx >= kMatrixWidth) {bx = 0;}
    if (by >= kMatrixHeight){by = 0;}
    if (StopAt >= 1) {break ;}
  } while (true);
  Serial.println("exited ON loop");
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND);
  // While this one turns them off:
  StopAt = 0; //Reinitalise
  Serial.println("entering OFF loop");
  do 
  {
    leds[ XY( bx, by) ] = CRGB::Black;
    bx++;
    by++;
    if (by >= kMatrixHeight){ StopAt++;}
    if (bx >= kMatrixWidth) {bx = 0;}
    if (by >= kMatrixHeight){by = 0;}
    if (StopAt >= 1) {break ;}
  } while (true);
  Serial.println("exited OFF loop");
  corkScrew_x++;
  if (corkScrew_x >= kMatrixWidth)
  {
    corkScrew_x = 0;
  }
}


void reverseTrailingCorkscrew()
{
  int x = kMatrixWidth -1;
  int y = 0;
  x -= corkScrew_x;
  if (x < 0 ) {x = kMatrixWidth -1;}
  int StopAt = 0; //Initalise
  // This loop turns the LEDs on:
  Serial.println("24: entering ON loop");
  do 
  {
    leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
    x--;
    y++;
    if (x < 0)
    {
      x = kMatrixWidth -1;
    //  StopAt++;
    }
    if (y >= kMatrixHeight)
    {
      y = 0;
      StopAt++;
    }
    if (StopAt >= 1) {break ;}
  } while (true);
  Serial.println("24: exited ON loop");
  LEDS.show();
  LEDS.delay(1000/FRAMES_PER_SECOND);
  // While this one turns them off:
  StopAt = 0; //Reinitalise
  Serial.println("24: entering OFF loop");
  do 
  {
    leds[ XY( x, y) ] = CRGB::Black;
    x--;
    y++;
    if (x < 0)
    {
      x = kMatrixWidth -1;
      //StopAt++;
    }
    if (y >= kMatrixHeight)
    {
      y = 0;
      StopAt++;
    }
    if (StopAt >= 1) {break ;}
  } while (true);
  Serial.println("24: exited OFF loop");
  corkScrew_x--;
  if (corkScrew_x < 0)
  {
    corkScrew_x = kMatrixWidth -1;
  }
}
