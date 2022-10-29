/*
   board M5StickC RollerCounter
*/
#define LGFX_AUTODETECT
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "M5StickCPlus.h"
#include "Ticker.h"

static LGFX lcd;
int push_count = 0;
int chat_count = 0;
bool redraw;
int guard_msec = 0;

Ticker tick;
int tick_wait_sec = 0;
bool flag_100msec = false;
bool flag_1sec = true;
int cnt_1sec = 0;

bool flag_req_power_off = false;
bool flag_req_beep = false;

int bat_percent;

#define TIME_TO_POWER_OFF  (5*60)

#define PIN_SW GPIO_NUM_33

void tick_100msec(int val)
{
  flag_100msec = true;

  cnt_1sec += 100;
  if (cnt_1sec >= 1000) {
    cnt_1sec = 0;
    flag_1sec = true;

    // 1秒の処理
    if (tick_wait_sec) {
      tick_wait_sec--;
      redraw = true;
      if (tick_wait_sec == 60) {
        flag_req_beep = true;
      } else if (tick_wait_sec == 0) {
        flag_req_power_off = true;
      }
    }
  }
}

// 4.2V = 100%
// 3.0V = 0%
int getBatteryPercent()
{
  float vbat = M5.Axp.GetVbatData() * (1.1 / 1000);
  int bat_p = int(((vbat - 3.0) / (4.2 - 3.0)) * 100);
  if (bat_p > 100) {
    bat_p = 100;
  } else if (bat_p < 0) {
    bat_p = 0;
  }

  return bat_p;
}

void beep()
{
  M5.Beep.tone(2000);
  delay(100);
  M5.Beep.mute();
}

void setup(void)
{
  M5.begin();

  lcd.init();
  lcd.setRotation(1);
  pinMode(PIN_SW, INPUT_PULLUP);

  tick_wait_sec = TIME_TO_POWER_OFF;
  tick.attach_ms(100, tick_100msec, 0);
  redraw = true;
}

void loop(void)
{
  bool flag_push_sw = false;

  if (digitalRead(PIN_SW) == LOW) {
    if (chat_count < 5) {
      chat_count++;
      if (chat_count == 5) {
        //event key press
        flag_push_sw = true;
      }
    }
  } else {
    if (chat_count == 5) {
      //event key release
    }
    chat_count = 0;
  }

  M5.update();
  if (M5.BtnB.wasReleased()) {
    flag_push_sw = true;
  }

  if (M5.BtnA.wasReleased()) {
    push_count = 0;
    redraw = true;
  }

  if (flag_push_sw) {
    if (guard_msec == 0) {
      guard_msec = 3000;//3秒間は入力禁止にする
      flag_req_beep = true;
      push_count++;
      redraw = true;
      tick_wait_sec = TIME_TO_POWER_OFF;
    }
  }

  if (flag_100msec) {
    flag_100msec = false;
    if (guard_msec) {
      guard_msec -= 100;
      if (guard_msec <= 0) {
        guard_msec = 0;
      }
    }
  }

  if (flag_1sec) {
    flag_1sec = false;
    bat_percent = getBatteryPercent();
  }

  if (redraw) {
    redraw = false;
    lcd.startWrite();

    //------------------------------
    float wait_ratio;
    wait_ratio = (float)tick_wait_sec / TIME_TO_POWER_OFF;
    const int bar_height = 2;
    lcd.fillRect(0, 9, lcd.width() - 1, bar_height, GREEN);
    lcd.fillRect(0, 9, (lcd.width() - 1) * wait_ratio, bar_height, RED);

    //------------------------------
    lcd.setCursor(0, 0);
    lcd.setTextSize(1, 1);
    lcd.printf("Roller Counter V003");

    //------------------------------
    lcd.setCursor(32, 16);
    lcd.setTextSize(16, 16);
    lcd.printf("%02d", push_count);

    //------------------------------
    lcd.setCursor(lcd.width() - (16 * 3), lcd.height() - 16);
    lcd.setTextSize(2, 2);
    lcd.printf("%3d%%", bat_percent);

    lcd.endWrite();
  }

  //--------------------------------------------------
  // 音を鳴らす
  //--------------------------------------------------
  if (flag_req_beep) {
    flag_req_beep = false;
    beep();
  }

  //--------------------------------------------------
  // 電源オフ
  //--------------------------------------------------
  if (flag_req_power_off) {
    flag_req_power_off = false;
    beep();
    M5.Axp.PowerOff();
  }
}
