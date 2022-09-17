#include <Arduino.h>

#include "ArduinoSort.h"

#include "JmaIntensity.hpp"
#include "LED.hpp"
#include "Filter.hpp"
#include "Config.h"

// 震度を計算する頻度(1 ~ 100/秒)
const int CALC_INTENSITY_RATE = 10;
// 加速度を出力する頻度(0 ~ 100/秒)
const int ACC_REPORT_RATE = 100;

Led LED = Led();
ADC_CLASS ADC = ADC_CONSTRUCTOR;

const int samplingRate = 100;
const int bufferSize = samplingRate;

Filter FILTER = Filter(samplingRate);

void setup()
{
#ifndef DISABLE_ADJUST_PIN
    pinMode(ADJUST_PIN, INPUT_PULLDOWN);
#endif

    Serial.begin(115200);

    ADC.begin();

    LED.wakeup();
}

// メモ: $はつけない
void printNmea(const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char *buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1)
    {
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
    if (buffer != temp)
    {
        delete[] buffer;
    }
    Serial.flush();
}

// 3-axis adjust value
ADC_RESULT_TYPE offset[3];
// Adujusting time( offset_counter / sampling rate = sec)
int offsetCounter = bufferSize * 1;
bool isOffsetted = false;

float computePGA(float *HPFilteredData)
{
    float pga = 0;
    float acc = 0;
    for (int i = 0; i < samplingRate; i++)
    {
        acc = sqrt(HPFilteredData[i] * HPFilteredData[i] + HPFilteredData[i + 1] * HPFilteredData[i + 1] + HPFilteredData[i + 2] * HPFilteredData[i + 2]);
        if (acc > pga)
            pga = acc;
    }

    return pga;
}

const int RETENTION_MICRO_SECONDS = 60 * 10 * 1000000;
int x = 0, y = 0, z = 0;

ADC_RESULT_TYPE rawData[3];
float compositeData[bufferSize];
float compositeSortedData[bufferSize];
float HPFilteredData[bufferSize * 3];

unsigned long frame = 0;

unsigned long latestMaxTime;
JmaIntensity latestIntensity;
JmaIntensity maxIntensity;

void loop()
{
    auto startTime = micros();

    uint16_t index = frame % bufferSize;

    float offsetSample[3];
    float newHPFilteredSample[3];
    float filteredDataSample[3];

    ADC.read(rawData, offset);
    // 計測した値を取得
    for (auto a = 0; a < 3; a++)
    {
        // オフセット計算
        if (!isOffsetted)
            offset[a] = rawData[a];

        offsetSample[a] = ADC_RAW_TO_GAL(rawData[a] - offset[a]);
        newHPFilteredSample[a] = offsetSample[a];
        filteredDataSample[a] = offsetSample[a];
    }

    FILTER.filterHP(newHPFilteredSample);
    FILTER.filterForShindo(filteredDataSample);

    for (int i = 0; i < 3; i++)
        HPFilteredData[index * 3 + i] = newHPFilteredSample[i];

    if (isOffsetted && ACC_REPORT_RATE > 0 && frame % (samplingRate / ACC_REPORT_RATE) == 0)
        printNmea("XSACC,%.3f,%.3f,%.3f", HPFilteredData[index * 3], HPFilteredData[index * 3 + 1], HPFilteredData[index * 3 + 2]);

    // 3軸合成
    compositeData[index] = sqrt(
        filteredDataSample[0] * filteredDataSample[0] +
        filteredDataSample[1] * filteredDataSample[1] +
        filteredDataSample[2] * filteredDataSample[2]);

    if (isOffsetted && frame % (samplingRate / CALC_INTENSITY_RATE) == 0)
    {
        memcpy(compositeSortedData, compositeData, sizeof(compositeData[0]) * bufferSize);
        sortArray(compositeSortedData, bufferSize);
        auto gal = compositeSortedData[(int)(samplingRate * 0.7)];
        float pga = computePGA(HPFilteredData);
        printNmea("XSPGA,%.3f", pga);

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
        delayMicroseconds(sleepTime);

    // if (frame % samplingRate == 0)
    // {
    //     printNmea("XTIME,%d,%d", workTime, sleepTime);
    // }

#ifndef DISABLE_ADJUST_PIN
    if (digitalRead(ADJUST_PIN) && isOffsetted)
    {
        isOffsetted = false;
        offsetCounter = samplingRate * 2;
        latestMaxTime = micros();
        printNmea("XSOFF,1");
    }
    else
#endif
    if (!isOffsetted && offsetCounter-- <= 0)
    {
        maxIntensity = JMA_INT_0;
        FILTER.reset();
        isOffsetted = true;
        printNmea("XSOFF,0");
    }
}
