/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2019.
 *
 * Do what you like with this code as long as you retain this comment.
 */

/* This is the code you will need to replace for Lab 1.
 *
 * It contains two simple implementations that loop over the longest axis adding the gradient to the position on the other axis at each step.
 * The objective is for you to remove the use of floating point variables and work entirely with integers.
 * You should use Bresenhams Line Algorithm to achieve this.
 */

#include <iostream>
#include "linedrawer.h"

int draw_x_line(FrameBuffer *fb, int sx, int sy, int ex, int ey)
{
  int xdir = 1;
  int ydir = 1;

  if(sy > ey)
  {
    ydir = -1;
  }

  if (sx > ex)
  {
    xdir = -1;
  }

  int wy = sy ; 
  int x = sx ; 

  int dx = (ex-sx) * xdir ;
  int dy = (ey-sy) * ydir ;

  int fy = dy/2;
  
  while (x != ex)
  {
    fb->plotPixel(x, wy, 1.0f, 1.0f, 1.0f);

    x += xdir;

    fy += dy;

    if (fy>=dx)
    {
      wy += ydir;
      fy -= dx;
    }
  }

}

int draw_y_line(FrameBuffer *fb, int sx, int sy, int ex, int ey)
{
  int xdir = 1;
  int ydir = 1;

  if(sy > ey)
  {
    ydir = -1;
  }

  if (sx > ex)
  {
    xdir = -1;
  }

  int y = sy ; 
  int wx = sx ; 

  int dx = (ex-sx) * xdir ;
  int dy = (ey-sy) * ydir ;

  int fx = dx/2;
  
  while (y != ey)
  {
    fb->plotPixel(wx, y, 1.0f, 1.0f, 1.0f);
    y+=ydir;

    fx += dx ;

    if (fx>=dy)
    {
      wx += xdir;
      fx -= dy;
    }
  }

}


int draw_line(FrameBuffer *fb, int sx, int sy, int ex, int ey)
{
  if ((sx == ex) && (sy==ey))
  {
    return fb->plotPixel(sx, sy, 1.0f, 1.0f, 1.0f);
    
  } else if (((ex-sx)* (ex-sx)) >= ((ey-sy)* (ey-sy)))
  {
    return draw_x_line(fb, sx, sy, ex, ey);
    
  } else
  {
    return draw_y_line(fb, sx, sy, ex, ey);
  }
}
