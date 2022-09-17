#pragma once

#include <Arduino.h>

#include "Config.h"
#include "JmaIntensity.hpp"

#define PIDAS_PLUS_LED_COUNT 10
#define PIDAS_PLUS_LED_CHECK_COUNT(i)           \
    if ((i) < 0 || (i) >= PIDAS_PLUS_LED_COUNT) \
    return

/**
 * LEDのドライバもどき
 */
class Led
{
private:
    bool ledState[PIDAS_PLUS_LED_COUNT];

    void on(int led)
    {
#ifndef DISABLE_INTENSITY_LED
        PIDAS_PLUS_LED_CHECK_COUNT(led);
        digitalWrite(INTENSITY_LED_PIN_OFFSET + led, HIGH);
        ledState[led] = true;
#endif
    }

    void off(int led)
    {
#ifndef DISABLE_INTENSITY_LED
        PIDAS_PLUS_LED_CHECK_COUNT(led);
        digitalWrite(INTENSITY_LED_PIN_OFFSET + led, LOW);
        ledState[led] = false;
#endif
    }

public:
    Led()
    {
#ifndef DISABLE_INTENSITY_LED
        for (auto i = 0; i < PIDAS_PLUS_LED_COUNT; i++)
            pinMode(INTENSITY_LED_PIN_OFFSET + i, OUTPUT);
#endif
    }

    /**
     * 起動時のアニメーションを行う
     * 1秒かかる
     */
    void wakeup()
    {
#ifndef DISABLE_INTENSITY_LED
        for (auto i = 0; i < PIDAS_PLUS_LED_COUNT; i++)
            on(i);
        sleep_ms(1000);
        for (auto i = 0; i < PIDAS_PLUS_LED_COUNT; i++)
            off(i);
#endif
    }

    void blinkScale(JmaIntensity scale, JmaIntensity max)
    {
        PIDAS_PLUS_LED_CHECK_COUNT(scale);
        PIDAS_PLUS_LED_CHECK_COUNT(max);
        for (auto i = 0; i < PIDAS_PLUS_LED_COUNT; i++)
        {
            if (i <= scale || i == max)
                on(i);
            else
                off(i);
        }
    }

    void toggle(int led)
    {
        PIDAS_PLUS_LED_CHECK_COUNT(led);
        if (ledState[led])
            off(led);
        else
            on(led);
    }

    void clear()
    {
        for (auto i = 0; i < PIDAS_PLUS_LED_COUNT; i++)
            off(i);
    }
};
