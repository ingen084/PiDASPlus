#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT    64
#define OLED_RESET       -1
#define SCREEN_ADDRESS 0x3C

class SSD1306 {
    Adafruit_SSD1306 *display;

public:
    // 初期化
    void begin() {
        this->display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
        this->display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
        this->display->clearDisplay();
        this->display->setTextColor(SSD1306_WHITE);
        this->display->display();
    }

    // 起動時の状態表示
    void wakeup() {
        this->display->clearDisplay();
        this->display->setCursor(0, 0);
        this->display->println("Hello, world!");
        this->display->display();
    }
    void update() {}
    void updateIntensity() {}
};
