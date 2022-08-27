#include <Arduino.h>

#include "ArduinoSort.h"

#include "JmaIntensity.hpp"
#include "LED.hpp"
#include "MCP3204.hpp"
#include "Filter.hpp"

// 震度を計算する頻度(1 ~ 100/秒)
const int CALC_INTENSITY_RATE = 10;
// 加速度を出力する頻度(0 ~ 100/秒)
const int ACC_REPORT_RATE = 100;

Led LED = Led();
MCP3204 ADC = MCP3204(SPISettings(115200, MSBFIRST, SPI_MODE0), D21);
const uint8_t ADJUST_PIN = D16;

const int samplingRate = 100;
const int bufferSize = samplingRate;

const uint8_t FILTER_LENGTH = 5;
// 3軸分
Filter FILTER[] = {
    Filter(samplingRate, FILTER_LENGTH),
    Filter(samplingRate, FILTER_LENGTH),
    Filter(samplingRate, FILTER_LENGTH)
};

void setup()
{
    pinMode(ADJUST_PIN, INPUT_PULLDOWN);

    Serial.begin(115200);

    // TODO: これクラスの中に持っていったほうが良さそう
    SPI.setRX(D20);
    SPI.setCS(D21);
    SPI.setSCK(D18);
    SPI.setTX(D19);
    SPI.begin();

    LED.wakeup();
}

// メモ: $はつけない
void printNmea(const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer)
            return;
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    byte checkSum = 0;
    for (int i = 0; buffer[i]; i++)
        checkSum ^= (byte)buffer[i];
    Serial.printf("$%s*%02X\r\n", buffer, checkSum);
    if (buffer != temp) {
        delete[] buffer;
    }
    Serial.flush();
}

// 3-axis adjust value
//float offset[3];
// Adujusting time( offset_counter / sampling rate = sec)
uint16_t offsetCounter = samplingRate * 2;
bool isOffsetted = false;

const int RETENTION_MICRO_SECONDS = 60 * 10 * 1000000;
int x = 0, y = 0, z = 0;

// NOTE: 本当は3フレーム*3軸分だけで良い
uint16_t rawData[bufferSize * 3];
float filteredData[bufferSize * 3];
float compositeData[bufferSize];
float compositeSortedData[samplingRate];

unsigned long frame = 0;

unsigned long latestMaxTime;
JmaIntensity latestIntensity;
JmaIntensity maxIntensity;

void loop()
{
    auto startTime = micros();

    uint16_t index = frame % bufferSize;

    // 計測した値を取得
    for (auto a = 0; a < 3; a++) {
        // 値を拾ってくる
        uint16_t v = ADC.read(a);

        auto localIndex = index * 3 + a;

        // 極値の場合前回の値を使い回す
        if (v < 0 || v > 4096 || v == 1024 || v == 2048 || v == 3072)
            v = rawData[index ? (localIndex - 3) : ((bufferSize - 1) * 3 + a)];
        
        rawData[localIndex] = v;

        // 直近の５フレーム分をフィルタに掛ける
        float dataset[FILTER_LENGTH];
        for (int i = 0; i < FILTER_LENGTH; i++)
        {
            auto fIndex = (frame - FILTER_LENGTH + i + 1) % bufferSize;
            // G to Gal.
            dataset[i] = ADC.toGal(rawData[fIndex * 3 + a]);
        }
        filteredData[localIndex] = FILTER[a].execFilterAndPop(dataset);
    }

    if (isOffsetted && ACC_REPORT_RATE > 0 && frame % (samplingRate / ACC_REPORT_RATE) == 0)
        printNmea("XSACC,%.3f,%.3f,%.3f", filteredData[index * 3], filteredData[index * 3 + 1], filteredData[index * 3 + 2]);
    
    // 3軸合成
    compositeData[index] = sqrt(
        filteredData[index * 3 + 0] * filteredData[index * 3 + 0] +
        filteredData[index * 3 + 1] * filteredData[index * 3 + 1] +
        filteredData[index * 3 + 2] * filteredData[index * 3 + 2]);

    if (isOffsetted && frame % (samplingRate / CALC_INTENSITY_RATE) == 0)
    {
        memcpy(compositeSortedData, compositeData, sizeof(compositeData[0]) * bufferSize);
        sortArray(compositeSortedData, samplingRate);
        auto gal = compositeSortedData[(int)(samplingRate * 0.7)];
        if (gal > 0)
        {
            auto rawInt = round((2.0f * log10(gal) + 0.94f) * 10.0f) / 10.0f;
            printNmea("XSINT,%.3f,%.2f", gal, rawInt);
            latestIntensity = getJmaIntensity(rawInt);

            if (micros() - latestMaxTime > RETENTION_MICRO_SECONDS || maxIntensity <= latestIntensity)
            {
                latestMaxTime = micros();
                maxIntensity = latestIntensity;
            }
            LED.blinkScale(latestIntensity, maxIntensity);
        }
        else
        {
            printNmea("XSINT,%.3f,", gal);
            LED.clear();
        }
    }

    if (isOffsetted && frame % (samplingRate / 4) == 0 && latestIntensity < maxIntensity)
        LED.toggle(maxIntensity);
    
    if (!isOffsetted)
    {
        auto v = (JmaIntensity)(offsetCounter * 100 / (samplingRate * 5) % 10);
        LED.blinkScale(v, v);
    }

    if (frame >= samplingRate * 60)
        frame = 0;

    frame++;

    auto workTime = micros() - startTime;
    auto sleepTime = 1000000 / samplingRate - workTime;
    if (sleepTime > 0)
        sleep_us(sleepTime);

    // if (frame % samplingRate == 0)
    // {
    //     printNmea("XTIME,%d,%d", workTime, sleepTime);
    // }

    if (offsetCounter <= 0 && !isOffsetted)
    {
        isOffsetted = true;
        printNmea("XSOFF,0");
    }
    else if (offsetCounter > 0)
    {
        offsetCounter -= 1;
    }

    if (digitalRead(ADJUST_PIN) && isOffsetted)
    {
        isOffsetted = false;
        offsetCounter = samplingRate;
        latestMaxTime = micros();
        maxIntensity = JMA_INT_0;
        printNmea("XSOFF,1");
    }
}
