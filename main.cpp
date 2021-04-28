#include "mbed.h"
#include "rtos.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include <list>
#include <string>

#define BTN_HEIGTH 50

TS_StateTypeDef TS_State;
Semaphore sem(1);

volatile uint32_t colors [] = {LCD_COLOR_GREEN, LCD_COLOR_LIGHTBLUE, LCD_COLOR_LIGHTMAGENTA, LCD_COLOR_ORANGE};
Thread threadsArray[77];
int currentThread = 0;
int touchX = 0, touchY = 0;

struct but{
    int posX;
    int posY;
    string info;
    int width;
} onBut, offBut, radioBut, mp3But, mobilBut;

typedef enum setBut{ 
  on,
  off,
  radio,
  mp3,
  mobil  
} setBut;

setBut currentState;
setBut previusState = off;

void butDis (int posX, int posY, bool isSelected, string text, int width){
    if (isSelected){
        BSP_LCD_FillRect(posX, posY, width, BTN_HEIGTH);
        BSP_LCD_SetBackColor(LCD_COLOR_MAGENTA);
        BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
        BSP_LCD_DisplayStringAt(posX + 3, posY + 7, (uint8_t *)text.c_str(), LEFT_MODE);
        BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
        BSP_LCD_SetBackColor(LCD_COLOR_MAGENTA);
    }
    else {
        BSP_LCD_DrawRect(posX, posY, width, BTN_HEIGTH);
        BSP_LCD_DisplayStringAt(posX + 7, posY + 10, (uint8_t *)text.c_str(), LEFT_MODE);
    }
}

bool touch(){
    BSP_TS_GetState(&TS_State);
    if(TS_State.touchDetected) {
        return true;
    }
    return false;   
}

void threadRun(string *modeName){
    sem.acquire();
    BSP_LCD_ClearStringLine(5);
    BSP_LCD_DisplayStringAtLine(5, (uint8_t *)modeName);
}

void startAutoMusic(){
    uint8_t text[30];
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);
    
    uint8_t status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
    
    if (status == TS_OK){
        BSP_LCD_Clear(LCD_COLOR_LIGHTCYAN);
        BSP_LCD_SetBackColor(LCD_COLOR_MAGENTA);
        BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
        BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t *)"Starting AutoMusic", CENTER_MODE);
    } 
    HAL_Delay(1000);

    BSP_LCD_SetFont(&Font24);
    BSP_LCD_Clear(LCD_COLOR_LIGHTCYAN);
    BSP_LCD_SetBackColor(LCD_COLOR_MAGENTA);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
}

bool touched(){
    BSP_TS_GetState(&TS_State);
    if (TS_State.touchDetected) {
        touchX = TS_State.touchX[0];
        touchY = TS_State.touchY[0];
        return true;
    }
    return false;
}

bool onOffTh(int x, int y){
    if (x >= offBut.posX && x <= offBut.posX + offBut.width && y >= offBut.posY && y <= offBut.posY + BTN_HEIGTH){
        return true;
    }
    return false;    
}

bool radioTh(int x, int y){
    if (x >= radioBut.posX && x <= radioBut.posX + radioBut.width && y >= radioBut.posY && y <= radioBut.posY + BTN_HEIGTH){
        return true;
    }
    return false;
}

bool mp3Th(int x, int y){
    if (x >= mp3But.posX && x <= mp3But.posX + mp3But.width && y >= mp3But.posY && y <= mp3But.posY + BTN_HEIGTH){
        return true;
    }
    return false;
}

bool mobilTh(int x, int y){
    if (x >= mobilBut.posX && x <= mobilBut.posX + mobilBut.width && y >= mobilBut.posY && y <= mobilBut.posY + BTN_HEIGTH){
        return true;
    }
    return false;
}

void userI(){
    BSP_LCD_Clear(LCD_COLOR_LIGHTCYAN);
    switch (currentState){
      case off:
        butDis(onBut.posX, onBut.posY, false, onBut.info, onBut.width);
        break;
      case on:
        butDis(offBut.posX, offBut.posY, false, offBut.info, offBut.width);
        butDis(radioBut.posX, radioBut.posY, false, radioBut.info, radioBut.width);
        butDis(mp3But.posX, mp3But.posY, false, mp3But.info, mp3But.width);
        butDis(mobilBut.posX, mobilBut.posY, false, mobilBut.info, mobilBut.width);
        break;
      case radio:
        butDis(offBut.posX, offBut.posY, false, offBut.info, offBut.width);
        butDis(radioBut.posX, radioBut.posY, true, radioBut.info, radioBut.width);
        butDis(mp3But.posX, mp3But.posY, false, mp3But.info, mp3But.width);
        butDis(mobilBut.posX, mobilBut.posY, false, mobilBut.info, mobilBut.width);
        break;
      case mp3:
        butDis(offBut.posX, offBut.posY, false, offBut.info, offBut.width);
        butDis(radioBut.posX, radioBut.posY, false, radioBut.info, radioBut.width);
        butDis(mp3But.posX, mp3But.posY, true, mp3But.info, mp3But.width);
        butDis(mobilBut.posX, mobilBut.posY, false, mobilBut.info, mobilBut.width);
        break;
      case mobil:
        butDis(offBut.posX, offBut.posY, false, offBut.info, offBut.width);
        butDis(radioBut.posX, radioBut.posY, false, radioBut.info, radioBut.width);
        butDis(mp3But.posX, mp3But.posY, false, mp3But.info, mp3But.width);
        butDis(mobilBut.posX, mobilBut.posY, true, mobilBut.info, mobilBut.width);
        break;
    }
}

void changeState(setBut newSetBut) {
    sem.release();
  switch (newSetBut) {
  case on:
    threadsArray[currentThread].terminate();
    threadsArray[currentThread].join();
    previusState = currentState;
    currentState = newSetBut;
    userI();
    currentThread++;
    threadsArray[currentThread].start(
        callback(threadRun, (string *)"ON"));
    break;
  case off:
    threadsArray[currentThread].terminate();
    threadsArray[currentThread].join();
    previusState = currentState;
    currentState = newSetBut;
    userI();
    currentThread++;
    threadsArray[currentThread].start(callback(threadRun, (string *)""));
    break;
  case radio:
    threadsArray[currentThread].terminate();
    threadsArray[currentThread].join();
    previusState = currentState;
    currentState = newSetBut;
    userI();
    currentThread++;
    threadsArray[currentThread].start(
        callback(threadRun, (string *)"Radio"));
    break;
  case mp3:
    threadsArray[currentThread].terminate();
    threadsArray[currentThread].join();
    previusState = currentState;
    currentState = newSetBut;
    userI();
    currentThread++;
    threadsArray[currentThread].start(
        callback(threadRun, (string *)"MP3"));
    break;
  case mobil:
    threadsArray[currentThread].terminate();
    threadsArray[currentThread].join();
    previusState = currentState;
    currentState = newSetBut;
    userI();
    currentThread++;
    threadsArray[currentThread].start(
        callback(threadRun, (string *)"Mobil"));
    break;
  }
}

void initButtons(){
    onBut.posX = 20;
    onBut.posY = 20;
    onBut.info = "Play";
    onBut.width = 80;
    offBut.posX = 20;
    offBut.posY = 20;
    offBut.info = "OFF";
    offBut.width = 80;

    radioBut.posX = 120;
    radioBut.posY = 20;
    radioBut.info = "Radio";
    radioBut.width = 100;

    mp3But.posX = 240;
    mp3But.posY = 20;
    mp3But.info = "MP3";
    mp3But.width = 70;

    mobilBut.posX = 330;
    mobilBut.posY = 20;
    mobilBut.info = "Mobil";
    mobilBut.width = 100;
}

int main()
{
    startAutoMusic();
    initButtons();
    userI();
    changeState(on);

    while (true) {
        if (touched()){
            HAL_Delay(300);
            if (radioTh(touchX, touchY)){
                if (currentState != radio){
                    changeState(radio);
                }
            }
            if (mp3Th(touchX, touchY)){
                if (currentState != mp3){
                    changeState(mp3);
                }
            }
            if (mobilTh(touchX, touchY)){
                if (currentState != mobil){
                    changeState(mobil);
                }
            }
            if (onOffTh(touchX, touchY)){
                if (currentState == off){
                  changeState(previusState);
                }
                else {
                  changeState(off);
                }
            }
        }
    }
}












