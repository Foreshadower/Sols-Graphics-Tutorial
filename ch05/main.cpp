#include <string.h>
#include <math.h>
#include <stdlib.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#define TREECOUNT 64

float gTreeCoord[TREECOUNT * 2];


int* gFrameBuffer;
int *gTempBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

bool update()
{
  SDL_Event e;
  if (SDL_PollEvent(&e))
  {
    if (e.type == SDL_EVENT_QUIT)
    {
      return false;
    }
    if (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE)
    {
      return false;
    }
  }

  char* pix;
  int pitch;
  
  SDL_LockTexture(gSDLTexture, NULL, (void**)&pix, &pitch);
  for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
    memcpy(pix + sp, gFrameBuffer + dp, WINDOW_WIDTH * 4);

  SDL_UnlockTexture(gSDLTexture);  
  SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL);
  SDL_RenderPresent(gSDLRenderer);
  SDL_Delay(1);
  return true;
}

#define PI 3.1415926535897932384626433832795

void drawcircle(int x, int y, int r, int c)
{
  for (int i = 0; i < 2 * r; i++)
  {
    // vertical clipping: (top and bottom)
    if ((y - r + i) >= 0 && (y - r + i) < WINDOW_HEIGHT)
    {
      int len = (int)(sqrt(r * r - (r - i) * (r - i)) * 2);
      int xofs = x - len / 2;
      
      // left border
      if (xofs < 0)
      {
        len += xofs;
        xofs = 0;
      }
      
      // right border
      if (xofs + len >= WINDOW_WIDTH)
      {
        len -= (xofs + len) - WINDOW_WIDTH;
      }
      int ofs = (y - r + i) * WINDOW_WIDTH + xofs;
      
      // note that len may be 0 at this point, 
      // and no pixels get drawn!
      for (int j = 0; j < len; j++)
        gFrameBuffer[ofs + j] = c;
    }
  }
}


unsigned int blend_avg(int source, int target)
{
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

  targetr = (sourcer + targetr) / 2;
  targetg = (sourceg + targetg) / 2;
  targetb = (sourceb + targetb) / 2;

  return (targetr << 0) |
    (targetg << 8) |
    (targetb << 16) |
    0xff000000;
}

unsigned int blend_mul(int source, int target)
{
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

  targetr = (sourcer * targetr) >> 8;
  targetg = (sourceg * targetg) >> 8;
  targetb = (sourceb * targetb) >> 8;

  return (targetr << 0) |
    (targetg << 8) |
    (targetb << 16) |
    0xff000000;
}

unsigned int blend_add(int source, int target)
{
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

  targetr += sourcer;
  targetg += sourceg;
  targetb += sourceb;

  if (targetr > 0xff) targetr = 0xff;
  if (targetg > 0xff) targetg = 0xff;
  if (targetb > 0xff) targetb = 0xff;

  return (targetr << 0) |
    (targetg << 8) |
    (targetb << 16) |
    0xff000000;
}

void scaleblit()
{
  int yofs = 0;
  for (int i = 0; i < WINDOW_HEIGHT; i++)
  {
    for (int j = 0; j < WINDOW_WIDTH; j++)
    {
      int c =
        (int)((i * 0.95) + (WINDOW_HEIGHT * 0.025)) * WINDOW_WIDTH +
        (int)((j * 0.95) + (WINDOW_WIDTH * 0.025));
      gFrameBuffer[yofs + j] =
        blend_avg(gFrameBuffer[yofs + j], gTempBuffer[c]);
    }
    yofs += WINDOW_WIDTH;
  }
}

void putpixel(int x, int y, int color)
{
  if (x < 0 ||
      y < 0 ||
      x >= WINDOW_WIDTH ||
      y >= WINDOW_HEIGHT)
  {
      return;
  }
  gFrameBuffer[y * WINDOW_WIDTH + x] = color;
}

const unsigned char sprite[] =
{
0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0
};

void drawsprite(int x, int y, unsigned int color)
{
  int scale = 4;
  int i, j, c, yofs;
  yofs = y * WINDOW_WIDTH + x;
  for (i = 0, c = 0; i < 16; i++)
  {
    for (j = 0; j < 16; j++, c++)
    {
      if (sprite[c])
      {
        for (int k = 0; k < scale; k++) {
            if ((y + (i*(scale-1)) + i + k) >= WINDOW_HEIGHT) break;
            for (int l = 0; l < scale; l++) {
                if ((x + (scale*j) + l) >= WINDOW_WIDTH) break;
                gFrameBuffer[(yofs + ((k+(i*(scale-1)))*WINDOW_WIDTH) + ((scale*j)+l))] = color;
            }
        }
      }
    }
    yofs += WINDOW_WIDTH;
  }
}



void init()
{
  srand(0x7aa7);
  int i;
  for (i = 0; i < TREECOUNT; i++)
  {   
    int x = rand();
    int y = rand();
    gTreeCoord[i * 2 + 0] = ((x % 10000) - 5000) / 1000.0f;
    gTreeCoord[i * 2 + 1] = ((y % 10000) - 5000) / 1000.0f;
  }
}


void newsnow()
{
  for (int i = 0; i < 8; i++)
    gFrameBuffer[rand() % (WINDOW_WIDTH - 2) + 1] = 0xffffffff;
}

void snowfall()
{
  for (int j = WINDOW_HEIGHT - 2; j >= 0; j--)
  {
    int ypos = j * WINDOW_WIDTH;
    for (int i = 1; i < WINDOW_WIDTH - 1; i++)
    {
      if (gFrameBuffer[ypos + i] == 0xffffffff)
      {
        if (gFrameBuffer[ypos + i + WINDOW_WIDTH] == 0xff000000)
{
  gFrameBuffer[ypos + i + WINDOW_WIDTH] = 0xffffffff;
  gFrameBuffer[ypos + i] = 0xff000000;
}
else
if (gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000)
{
  gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffffffff;
  gFrameBuffer[ypos + i] = 0xff000000;
}
else
if (gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000)
{
  gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] = 0xffffffff;
  gFrameBuffer[ypos + i] = 0xff000000;
}

      }
    }
  }
}

void render(Uint64 aTicks)
{
  // Clear the screen with a green color
  for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
    gFrameBuffer[i] = 0xff005f00;

  float pos_x = (float)sin(aTicks * 0.00037234) * 2;
  float pos_y = (float)cos(aTicks * 0.00057234) * 2;

float shadow_x = (float)sin(aTicks * 0.0002934872) + 3;
float shadow_y = (float)cos(aTicks * 0.0001813431) + 8;

for (int j = 0; j < TREECOUNT; j++)
{   
  float x = gTreeCoord[j * 2 + 0] + pos_x;
  float y = gTreeCoord[j * 2 + 1] + pos_y;
    
  for (int i = 0; i < 8; i++)
  {   
    drawcircle((int)(x * 200 + WINDOW_WIDTH / 2 + (i + 1) * shadow_x),
               (int)(y * 200 + WINDOW_HEIGHT / 2 + (i + 1) * shadow_y),
               (10 - i) * 5,
               0xff1f4f1f);
  }
}

  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < TREECOUNT; j++)
    {
      float x = gTreeCoord[j * 2 + 0] + pos_x;
      float y = gTreeCoord[j * 2 + 1] + pos_y;
      drawcircle((int)(x * (200 + i * 4) + WINDOW_WIDTH / 2),
          (int)(y * (200 + i * 4) + WINDOW_HEIGHT / 2),
          (9 - i) * 5,
          (i * 0x030906 + 0x1f671f) | 0xff000000);
    }
  }
}


void loop()
{
  if (!update())
  {
    gDone = 1;
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
  }
  else
  {
    render(SDL_GetTicks());
  }
}

int main(int argc, char** argv)
{
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
  {
    return -1;
  }

  gFrameBuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
  gSDLWindow = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
  gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

  if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture)
    return -1;
init();
  gDone = 0;
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
#else
  while (!gDone)
  {
    loop();
  }
#endif

  SDL_DestroyTexture(gSDLTexture);
  SDL_DestroyRenderer(gSDLRenderer);
  SDL_DestroyWindow(gSDLWindow);
  SDL_Quit();

  return 0;
}
