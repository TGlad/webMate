#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char byte;

typedef struct MD5state
{
  uint len;
  uint state[4];
}MD5state;

MD5state *md5(byte *p, uint len, byte *digest, MD5state *s);