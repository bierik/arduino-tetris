#include "MD_MAX72xx.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Transformer.h"
#include "Point.h"

long delays = 0;
short delay_ = 100;
long bdelay = 0;
short buttondelay = 30;
short btdowndelay = 10;
short btsidedelay = 20;
unsigned char blocktype;
unsigned char blockrotation;

#define WIDTH 16
#define HEIGHT 32

int lines = 0;
boolean  block[WIDTH][HEIGHT + 2]; //2 extra for rotation
boolean  pile[WIDTH][HEIGHT];
boolean disp[WIDTH][HEIGHT];

#define MAX_DEVICES 8

#define CLK_PIN   SCK  // or SCK
#define DATA_PIN  MOSI  // or MOSI
#define CS_PIN    MISO  // or SS

MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
Transformer transformer = Transformer();

void setup() {

  int seed =
  (analogRead(0)+1)*
  (analogRead(1)+1)*
  (analogRead(2)+1)*
  (analogRead(3)+1);
  randomSeed(seed);
  random(10,9610806);
  seed = seed *random(3336,15679912)+analogRead(random(4)) ;
  randomSeed(seed);
  random(10,98046);


  cli();//stop interrupts
  
  //set timer0 interrupt at 2kHz
  TCCR1A = 0;// set entire TCCR0A register to 0
  TCCR1B = 0;// same for TCCR0B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR1A = 259;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR1A |= (1 << WGM01);
  // Set CS11 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);   
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE0A);

  sei();//allow interrupts  

  pinMode(5, INPUT); //up
  pinMode(2, INPUT); //right
  pinMode(3, INPUT); //left
  pinMode(4, INPUT); //down
  

  newBlock();
  mx.begin();
  mx.clear();
  
  updateLED();

  Serial.begin(9600);
}

void loop() {

  if (delays < millis())
   {
     delays = millis() + delay_;
     movedown();
   }

   //buttun actions
  int button = readBut();

  if (button == 1) //up=rotate
    rotate();
  if (button == 2) //right=moveright
    moveright();
  if (button == 3) //left=moveleft
    moveleft();
  if (button == 4) //down=movedown
    movedown();
}

boolean moveleft()
{
  if (space_left())
  {
    int i;
    int j;
    for (i=0;i<WIDTH-1;i++)
    {
      for (j=0;j<HEIGHT;j++)
      {
        block[i][j]=block[i+1][j];
      }
    }

    for (j=0;j<HEIGHT;j++)
    {
      block[WIDTH-1][j]=0;
    }

    updateLED();
    return 1;
  }

  return 0;
}

boolean moveright()
{
  if (space_right())
  {
    int i;
    int j;
    for (i=WIDTH-1;i>0;i--)
    {
      for (j=0;j<HEIGHT;j++)
      {
        block[i][j]=block[i-1][j];
      }
    }

    for (j=0;j<HEIGHT;j++)
    {
      block[0][j]=0;
    }

   updateLED();
   return 1;

  }
  return 0;
}

int readBut()
{
  if (bdelay > millis())
  {
    return 0;
  }
  if (digitalRead(3) == HIGH)
  {
    //left
    bdelay = millis() + btsidedelay;
    return 3;
  }

  if (digitalRead(4) == HIGH)
  {
    //down
    bdelay = millis() + btdowndelay;
    return 4;
  }
  if (digitalRead(2) == HIGH)
  {
    //right
    bdelay = millis() + btsidedelay;
    return 2;
  }
  if (digitalRead(5) == HIGH)
  {
    //up
    bdelay = millis() + buttondelay;
    return 1;
  }

  return 0;
}

void updateLED()
{
  int i;
  int j;
  for (i=0;i<WIDTH;i++)
  {
    for (j=0;j<HEIGHT;j++)
    {
      disp[i][j] = block[i][j] | pile[i][j];
    }
  }
}

void rotate()
{

  //skip for square block(3)
  if (blocktype == 3) return;

  int xi;
  int yi;
  int i;
  int j;
  //detect left
  for (i=WIDTH-1;i>=0;i--)
  {
    for (j=0;j<HEIGHT;j++)
    {
      if (block[i][j])
      {
        xi = i;
      }
    }
  }

  //detect up
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
      if (block[j][i])
      {
        yi = i;
      }
    }
  }

  if (blocktype == 0)
  {
    if (blockrotation == 0)
    {


      if (!space_left())
      {
        if (space_right3())
        {
          if (!moveright())
            return;
          xi++;
        }
        else return;
      }
      else if (!space_right())
      {
        if (space_left3())
        {
          if (!moveleft())
            return;
          if (!moveleft())
            return;
          xi--;
          xi--;
        }
        else
          return;
      }
      else if (!space_right2())
      {
        if (space_left2())
        {
          if (!moveleft())
            return;
          xi--;
        }
        else
          return;
      }





      block[xi][yi]=0;
      block[xi][yi+2]=0;
      block[xi][yi+3]=0;

      block[xi-1][yi+1]=1;
      block[xi+1][yi+1]=1;
      block[xi+2][yi+1]=1;

      blockrotation = 1;
    }
    else
    {
      block[xi][yi]=0;
      block[xi+2][yi]=0;
      block[xi+3][yi]=0;

      block[xi+1][yi-1]=1;
      block[xi+1][yi+1]=1;
      block[xi+1][yi+2]=1;

      blockrotation = 0;
    }
  }

  //offset to mid
  xi ++;
  yi ++;

  if (blocktype == 1)
  {
    if (blockrotation == 0)
    {
      block[xi-1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi-1] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi-1] = 0;
      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;

      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi+1][yi+1] = 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi --;

      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;

      block[xi][yi-1] = 1;
      block[xi][yi+1] = 1;
      block[xi-1][yi+1] = 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi+1] = 0;

      block[xi-1][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;

      blockrotation = 0;
    }
  }



  if (blocktype == 2)
  {
    if (blockrotation == 0)
    {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi+1] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi-1] = 0;
      block[xi+1][yi+1] = 0;
      block[xi][yi+1] = 0;

      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi-1][yi+1] = 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi --;

      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi-1][yi+1] = 0;

      block[xi][yi-1] = 1;
      block[xi][yi+1] = 1;
      block[xi-1][yi-1] = 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi-1] = 0;

      block[xi+1][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;

      blockrotation = 0;
    }
  }

  if (blocktype == 4)
  {
    if (blockrotation == 0)
    {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;

      block[xi+1][yi] = 1;
      block[xi+1][yi+1] = 1;

      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;

      block[xi-1][yi] = 1;
      block[xi+1][yi-1] = 1;

      blockrotation = 0;
    }
  }


  if (blocktype == 5)
  {
    if (blockrotation == 0)
    {
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 1;
      block[xi+1][yi] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 1;
    }
    else if (blockrotation == 1)
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi][yi-1] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;

      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 2;
    }
    else if (blockrotation == 2)
    {
      yi --;

      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;

      block[xi][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 3;
    }
    else
    {
      if (!space_right())
      {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi][yi+1] = 0;

      block[xi][yi-1] = 1;
      block[xi-1][yi] = 1;
      block[xi+1][yi] = 1;

      blockrotation = 0;
    }
  }

  if (blocktype == 6)
  {
    if (blockrotation == 0)
    {
      block[xi-1][yi-1] = 0;
      block[xi][yi-1] = 0;

      block[xi+1][yi-1] = 1;
      block[xi][yi+1] = 1;

      blockrotation = 1;
    }
    else
    {
      if (!space_left())
      {
        if (!moveright())
          return;
        xi++;
      }
      xi--;

      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;

      block[xi-1][yi-1] = 1;
      block[xi][yi-1] = 1;

      blockrotation = 0;
    }
  }

  //if rotating made block and pile overlap, push rows up
  while (!check_overlap())
  {
    for (i=0;i<HEIGHT+2;i++)
    {
      for (j=0;j<WIDTH;j++)
      {
         block[j][i] = block[j][i+1];
      }
    }
    delays = millis() + delay_;
  }


  updateLED();
}

void movedown()
{
  if (space_below())
  {
    //move down
    int i;
    for (i=HEIGHT-1;i>=0;i--)
    {
      int j;
      for (j=0;j<WIDTH;j++)
      {
        block[j][i] = block[j][i-1];
      }
    }
    for (i=0;i<WIDTH-1;i++)
    {
      block[i][0] = 0;
    }
  }
  else
  {
    //merge and new block
    int i;
    int j;
    for (i=0;i<WIDTH;i++)
    {
     for(j=0;j<HEIGHT;j++)
     {
       if (block[i][j])
       {
         pile[i][j]=1;
         block[i][j]=0;
       }
     }
    }
    newBlock();
  }
  updateLED();
}

boolean check_overlap()
{
  int i;
  int j;
  for (i=0;i<HEIGHT;i++)
  {
    for (j=0;j<WIDTH-1;j++)
    {
       if (block[j][i])
       {
         if (pile[j][i])
           return false;
       }
    }
  }
  for (i=HEIGHT;i<HEIGHT+2;i++)
  {
    for (j=0;j<WIDTH-1;j++)
    {
       if (block[j][i])
       {
         return false;
       }
    }
  }
  return true;
}

void check_gameover()
{
  int i;
  int j;
  int cnt=0;;

  for(i=HEIGHT-1;i>=0;i--)
  {
    cnt=0;
    for (j=0;j<WIDTH;j++)
    {
      if (pile[j][i])
      {
        cnt ++;
      }
    }
    if (cnt == WIDTH)
    {
        lines++;
      for (j=0;j<WIDTH;j++)
      {
        pile[j][i]=0;
      }
      updateLED();
      delay(50);

      int k;
      for(k=i;k>0;k--)
      {
        for (j=0;j<WIDTH;j++)
        {
          pile[j][k] = pile[j][k-1];
        }
      }
      for (j=0;j<WIDTH;j++)
      {
        pile[j][0] = 0;
      }
      updateLED();
      delay(50);
      i++;



    }
  }


  for(i=0;i<WIDTH;i++)
  {
    if (pile[i][0])
      gameover();
  }
  return;
}

void gameover()
{
  int i;
  int j;
  
  //close blind
  for (i=0;i<WIDTH;i++)
  {
     for (j=0;j<HEIGHT;j++)
     {
       if (j%2)
       {
         disp[i][j]=1;
       }
       else
       {
         disp[WIDTH-i][j]=1;        
       }
     }
     delay(20);
  }

  //close blind
  for (i=0;i<WIDTH;i++)
  {
     for (j=0;j<HEIGHT;j++)
     {
       disp[i][j]=0;
       pile[i][j]=0;
     }
  }
  for (i=0;i<WIDTH;i++)
  {
     for (j=0;j<HEIGHT+2;j++)
     {
       block[i][j]=0;
     }
  }
}

void newBlock()
{
  check_gameover();


  blocktype = random(7);


  if (blocktype == 0)
  // 0
  // 0
  // 0
  // 0
  {
    block[3][0]=1;
    block[3][1]=1;
    block[3][2]=1;
    block[3][3]=1;
  }

  if (blocktype == 1)
  // 0
  // 0 0 0
  {
    block[2][0]=1;
    block[2][1]=1;
    block[3][1]=1;
    block[4][1]=1;
  }

  if (blocktype == 2)
  //     0
  // 0 0 0
  {
    block[4][0]=1;
    block[2][1]=1;
    block[3][1]=1;
    block[4][1]=1;
  }

  if (blocktype == 3)
  // 0 0
  // 0 0
  {
    block[3][0]=1;
    block[3][1]=1;
    block[4][0]=1;
    block[4][1]=1;
  }

  if (blocktype == 4)
  //   0 0
  // 0 0
  {
    block[4][0]=1;
    block[5][0]=1;
    block[3][1]=1;
    block[4][1]=1;
  }

  if (blocktype == 5)
  //   0
  // 0 0 0
  {
    block[4][0]=1;
    block[3][1]=1;
    block[4][1]=1;
    block[5][1]=1;
  }

  if (blocktype == 6)
  // 0 0
  //   0 0
  {
    block[3][0]=1;
    block[4][0]=1;
    block[4][1]=1;
    block[5][1]=1;
  }

  blockrotation = 0;
}

boolean space_below()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (i == HEIGHT-1)
           return false;
         if (pile[j][i+1])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_left2()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (j == 0 || j == 1)
           return false;
         if (pile[j-1][i] | pile[j-2][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_left3()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (j == 0 || j == 1 ||j == 2 )
           return false;
         if (pile[j-1][i] | pile[j-2][i]|pile[j-3][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_left()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (j == 0)
           return false;
         if (pile[j-1][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_right()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (j == WIDTH-1)
           return false;
         if (pile[j+1][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_right3()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<WIDTH;j++)
    {
       if (block[j][i])
       {
         if (j == WIDTH-1||j == WIDTH-2||j == WIDTH-3)
           return false;
         if (pile[j+1][i] |pile[j+2][i] | pile[j+3][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

boolean space_right2()
{
  int i;
  int j;
  for (i=HEIGHT-1;i>=0;i--)
  {
    for (j=0;j<HEIGHT;j++)
    {
       if (block[j][i])
       {
         if (j == WIDTH-1 || j == WIDTH-2)
           return false;
         if (pile[j+1][i] |pile[j+2][i])
         {
           return false;
         }
       }
    }
  }
  return true;
}

ISR(TIMER1_COMPA_vect){  //change the 0 to 1 for timer1 and 2 for timer2
    LEDRefresh();
}

void LEDRefresh()
{
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  for (int j=0;j<HEIGHT;j++)
  {
    for(int i=0;i<WIDTH;i++)
    {
      Point point = transformer.transform(i, j);
      mx.setPoint(point.x, point.y, disp[i][j]);
    }
  }

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}









