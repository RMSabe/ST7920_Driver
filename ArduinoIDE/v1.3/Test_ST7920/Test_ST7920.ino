/*
  Test ST7920 ArduinoIDE library.
  Version 1.3

  Author: Rafael Sabe
  Email: rafaelmsabe@gmail.com
*/

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include <st7920.hpp>

#define LCD_DB0 16U
#define LCD_DB1 17U
#define LCD_DB2 18U
#define LCD_DB3 19U
#define LCD_DB4 20U
#define LCD_DB5 21U
#define LCD_DB6 22U
#define LCD_DB7 23U
#define LCD_RS 14U
#define LCD_E 15U

ST7920 st7920(LCD_DB0, LCD_DB1, LCD_DB2, LCD_DB3, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7, LCD_RS, LCD_E);

extern void draw_proc1(void);
extern void draw_proc2(void);
extern void draw_proc3(void);
extern void draw_proc4(void);

void setup(void)
{
  st7920.begin();
  st7920.clearDisplay();
  st7920.enableGraphicDisplay(true);
  return;
}

void loop(void)
{
_l_loop_runtimeloop:

  st7920.bufferSetAll(false);
  draw_proc1();
  st7920.enableGraphicDisplay(true);
  delay(5000);
  st7920.bufferSetAll(false);
  draw_proc2();
  delay(5000);
  st7920.enableGraphicDisplay(false);
  draw_proc3();
  delay(5000);
  st7920.clearText();
  draw_proc4();
  delay(5000);
  st7920.clearText();

  goto _l_loop_runtimeloop;
  return;
}

void draw_proc1(void)
{
  uintptr_t cx = 0u;
  uintptr_t cy = 0u;
  
  float x = 0.0f;
  float y = 0.0f;

  for(x = 0.0f; x <= 6.28f; x += 0.01f)
  {
    y = sinf(x);

    cx = (uintptr_t) roundf(20.0f*x);
    cy = (uintptr_t) (roundf(32.0f - 28.0f*y));

    st7920.bufferSetPixel(cx, cy, true);
  }

  st7920.bufferPaintAll();
  return;
}

void draw_proc2(void)
{
  uintptr_t cx = 0u;
  uintptr_t cy = 0u;
  
  float x = 0.0f;

  for(x = 0.0f; x <= 6.28f; x += 0.01f)
  {
    cx = (uintptr_t) (roundf(64.0f - 28.0f*cosf(x)));
    cy = (uintptr_t) (roundf(32.0f - 28.0f*sinf(x)));

    st7920.bufferSetPixel(cx, cy, true);
  }

  for(x = 3.4f; x <= 6.02f; x += 0.01f)
  {
    cx = (uintptr_t) (roundf(64.0f - 18.0f*cosf(x)));
    cy = (uintptr_t) (roundf(34.0f - 14.0f*sinf(x)));

    st7920.bufferSetPixel(cx, cy, true);
  }

  st7920.bufferSetPixel(53, 22, true);
  st7920.bufferSetPixel(53, 23, true);
  st7920.bufferSetPixel(54, 22, true);
  st7920.bufferSetPixel(54, 23, true);

  st7920.bufferSetPixel(73, 22, true);
  st7920.bufferSetPixel(73, 23, true);
  st7920.bufferSetPixel(74, 22, true);
  st7920.bufferSetPixel(74, 23, true);

  st7920.bufferPaintAll();
  return;
}

void draw_proc3(void)
{
  st7920.setTextCursorPosition(0u, 0u);
  st7920.printText("This is line 00");
  st7920.setTextCursorPosition(0u, 1u);
  st7920.printText("This is line 01");
  st7920.setTextCursorPosition(0u, 2u);
  st7920.printText("This is line 02");
  st7920.setTextCursorPosition(0u, 3u);
  st7920.printText("This is line 03");
  
  return;
}

void draw_proc4(void)
{
  const uint16_t wtext[] = {0xa5e9, 0xa5d5, 0xa5a1, 0xa5a8, 0xa5eb, 0xa4b5, 0xa4f3, 0x0};

  st7920.setWTextCursorPosition(0u, 0u);
  st7920.printWText(wtext);
  
  return;
}
