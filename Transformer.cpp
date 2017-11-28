#include "Transformer.h"

Transformer::Transformer() {}

int div(int n, int d) {
  if (n == 0) {
    return n;
  }
  return n/d;
}

int yshift(int y) {
  return (8 - 1) + div(y,8) * 8 - (y % 8);
}

int xshift(int x) {
  return x % 8;
}

int xoverflow(int x, int y) {
  return y + div(x,8) * 32;
}

Point Transformer::transform(int x, int y)
{
  int xd = xshift(x);
  int yd = yshift(xoverflow(x, y));
  Point point(xd, yd);
  return point;
}

