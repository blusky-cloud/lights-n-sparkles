#include <Adafruit_CircuitPlayground.h>

//---STRUCTURE FOR NEO-PIXELS
struct nPixel
{
  float angle;
  int index;
};

void setup() {
  CircuitPlayground.begin(); //you gotta have this to use CP functions
  Serial.begin(9600);        //serial monitor initialization
  CircuitPlayground.setBrightness(10); //this sets the brightness of all the neos (0-255 scale)
}

void loop()
{
  float X, Y, Z; //passed by reference to seXYZ to be updated each cycle
  double angle;  //polarAngle will store a value here
  double hypotenuse; //passed by reference to polarAngle to be updated each cycle
  int q;         //q -> quadrant
  int nearest;   //keep track of lowest point 
  int neo;       //for which light 0-9 is on, (10 means position 0, 11 is pos 6, neither of which 
                 //-actually have lights since they're the charging ports)

//---INITIAL FUNCTIONS, GETTING XYZ DATA (PASSED BY REFERENCE) AND QUADRANT
  setXYZ(X, Y, Z);       //This just retrieves accelerometer data to work with
  q = findQuadrant(X,Y); //this finds which quadrant the CP is tilted towards (1-4, like cartesian coordinate plane)
  
//---CREATE AN ARRAY OF PIXELS, THEN POPULATE IT WITH DATA (ANGLE, INDEX), THEN FIND THE POLAR COORDINATES AND PASS HYPOTENUSE BY REFERENCE
  nPixel pixels[12];           //create an array of structures
  nPixelAssemble(pixels);      //populate the array with the unit circle position of each neopixel light in radians
  angle = polarAngle(X, Y, q, hypotenuse); //calculate the angle of tilt (as the 2D projection of the board's plane relative to positive x axis, 
                               //-which is the side opposite the USB charging cable).

  nearest = findNearestPixel(pixels, angle, X, Y);  //JUST FIND THE PIXEL NEAREST THE LOWEST POINT ON THE CIRCUIT PLAYGROUND
  //Serial.print("  Nearest is: ");Serial.println(nearest);


  tiltColor(nearest, neo, q, X, Y, hypotenuse, angle, pixels); //THIS IS A FINAL CONTROL FUNCTION
 
  //delay(250); //loop delay for everything which mostly justs makes serial data readable if > 200
  
}

//-----Update X, Y, Z variables with accelerometer data
void setXYZ(float & X, float & Y, float & Z)
{
  X = CircuitPlayground.motionX();
  Y = CircuitPlayground.motionY();
  Z = CircuitPlayground.motionZ();

  //Serial.print("X: "); Serial.print(X);
  //Serial.print("  Y: "); Serial.print(Y);
}

//-----Figure out which quadrant the CP is tilted towards based on sign of X and Y
int findQuadrant(float X, float Y)
{
  float minus = -0.5;
  float plus = 0.5;
  int q = 0;
  
  if (X<=minus && Y>=plus)
    q = 1;
  if (X<=minus && Y<=minus)
    q = 2;
  if (X>=plus && Y<=minus)
    q = 3;
  if (X>=plus && Y>=plus)
    q = 4;
  //Serial.print("  Quadrant: "); Serial.println(q);
  return q;
}

//-----This goes through an array of 'pixels' and populates them with their angle and 
//-----the index of their associated neoPixel (if any)
void nPixelAssemble(nPixel pixels[]) 
{
  for (int i = 0; i<6; i++) //for neos 0-4
  { 
    pixels[i].angle = 3.14 + (i * 0.52);
  }
  for (int q = 6; q<12; q++) //for neos 5-9
  {
    pixels[q].angle = ((q - 6) * 0.52); //0.5236 is about equal to pi/6
  }
  for (int i = 1; i<6; i++) //0-4
  {
    pixels[i].index = i - 1;
  }
  for (int i = 7; i<12; i++)//5-9
  {
    pixels[i].index = i - 2;
  }
  pixels[0].index = 10; //not neos, these are the chargers on the edges
  pixels[6].index = 11;
}

//-----This finds which pixel is nearest the lowest point on the board
int findNearestPixel(nPixel pixels[], double angle, float X, float Y)
{
  int mark;
  
  if (angle == 3.14)                    //handling the edge cases, pi 
    mark = 0;
  if (angle == 0.00 || angle == 6.28)   //and also 0 or 2pi
    mark = 6;
  for (int i = 0; i<12; i++)
  {
    if (pixels[i].angle <= (angle + 0.26) && pixels[i].angle >= (angle - 0.26)) // 0.26 is 0.5236/2, which is pi/12
    {
      mark = i;
    }
  }
  return mark;
}

//-----This calculates the angle of the projection of the board's plane using inverse cosine
//-----and also updates the 'hypotenuse' of the projected triangle formed by X and Y 
//-----accelerometer data, to use as a measure of how steep the tilt of the board is
double polarAngle(float X, float Y, int q, double & hypotenuse)
{
  double angle;
  //double hypotenuse;
  float cosRatio;
  
  hypotenuse = sqrt((X*X)+(Y*Y));
  cosRatio = X/hypotenuse; 
  //Serial.print("hypotenuse: ");Serial.print(hypotenuse);

/* this approximates cosine inverse to find the angle in radians
(using taylor series approximation of inverse cosine)
because computing taylor series approx of arccosine loses accuracy at 
0 and pi radians, where there are no lights, whereas arcsin approximations 
lose accuracy at pi/2 and 3pi/2, where there are lights. */
  angle = (1.570796327) - (cosRatio + ((cosRatio * cosRatio * cosRatio)/6)
                                    + ((3 * cosRatio * cosRatio * cosRatio *
                                        cosRatio * cosRatio)/40)
                                    + ((15 * cosRatio * cosRatio * cosRatio *
                                        cosRatio * cosRatio * cosRatio *
                                        cosRatio)/336));

/* lets do some more math to compensate for computation inaccuracy near 0 and pi
also for reference that this margin is eyeballed from accelerometer data near the edges */
  if (Y < 0.5 && Y > -0.5) //because near the axis the estimation loses accuracy (0.7)
  {
    if (X < 0)
      angle = 3.14;
    if (X > 0)
      angle = 0.00;
  }

  //now we want to get a 0 - 2 pi range rather then 0 - pi on the top and bottom
    //this will be based on y - axis data
  if (Y < 0)
  {
    angle = 3.14 + (3.14 - angle); 
  }
  
  //Serial.print(" angle:  "); Serial.print(angle);
  return angle;
}

//-----First attempt at a control function for every light on the board, there are many problems
//-----this is still very much in progress
void tiltColor(int nearest, int neo, int q, float X, float Y, float hypotenuse, double angle, nPixel pixels[])
{
  int color = 0;
  int fade = 0;

  if (hypotenuse > 0.5)
    color = int(hypotenuse * 28.8421); //this constant is 255/9, for 0-9 range
  
  color = constrain(color, 0, 255);
  //Serial.print("  color: ");Serial.print(color);
  
  //for (int i = 0; i < 10; i++) //this controls every neo simultaneously
    //CircuitPlayground.setPixelColor(i, CircuitPlayground.colorWheel(color));
    
  fade = color/10; //to provide increments by which to set the colors around the circle
  

/*
    THIS IS A BRUTE FORCE SOLUTION I ENDED UP WITH BECAUSE I GOT VERY FRUSTRATED 
 */
  
  CircuitPlayground.setPixelColor(pixels[nearest].index, CircuitPlayground.colorWheel(color));
  
  CircuitPlayground.setPixelColor(pixels[((nearest + 1) % 12)].index, CircuitPlayground.colorWheel(color - fade));
  CircuitPlayground.setPixelColor(pixels[((nearest + 11) % 12)].index, CircuitPlayground.colorWheel(color - fade));

  CircuitPlayground.setPixelColor(pixels[((nearest + 2) % 12)].index, CircuitPlayground.colorWheel(color - 2*fade));
  CircuitPlayground.setPixelColor(pixels[((nearest + 10) % 12)].index, CircuitPlayground.colorWheel(color - 2*fade));
  
  CircuitPlayground.setPixelColor(pixels[((nearest + 3) % 12)].index, CircuitPlayground.colorWheel(color - 3*fade));
  CircuitPlayground.setPixelColor(pixels[((nearest + 9) % 12)].index, CircuitPlayground.colorWheel(color - 3*fade));

  CircuitPlayground.setPixelColor(pixels[((nearest + 4) % 12)].index, CircuitPlayground.colorWheel(color - 4*fade));
  CircuitPlayground.setPixelColor(pixels[((nearest + 8) % 12)].index, CircuitPlayground.colorWheel(color - 4*fade));

  CircuitPlayground.setPixelColor(pixels[((nearest + 5) % 12)].index, CircuitPlayground.colorWheel(color - 5*fade));
  CircuitPlayground.setPixelColor(pixels[((nearest + 7) % 12)].index, CircuitPlayground.colorWheel(color - 5*fade));

  CircuitPlayground.setPixelColor(pixels[((nearest + 6) % 12)].index, CircuitPlayground.colorWheel(color - 6*fade));
  
  
  
  // THIS HAD GLITCHY COLOR INDEXING AND I GAVE UP BECAUSE I JUST WANTED A VERSION THAT WORKED PERFECTLY
  /* for (int i = 0; i<12; i++)
  {
    if (i == nearest)
      CircuitPlayground.setPixelColor(pixels[nearest].index, CircuitPlayground.colorWheel(color));
    
    else
    {
      if (i>0 && i<7)
      {
        if (i != nearest)
        CircuitPlayground.setPixelColor(pixels[((nearest + i) % 12)].index, CircuitPlayground.colorWheel(color - (i * fade)));
      }
      if (i>6 && i<12) 
      {
        if (i != nearest)
        CircuitPlayground.setPixelColor(pixels[((nearest + i) % 12)].index, CircuitPlayground.colorWheel(i * fade));
      }
    }
  } */

}
