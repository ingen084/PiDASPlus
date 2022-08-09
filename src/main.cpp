#include <Arduino.h>

#include "ArduinoSort.h"

#include "JmaIntensity.hpp"
#include "LED.hpp"
#include "MCP3204.hpp"
#include "Filter.hpp"

Led LED = Led();
MCP3204 ADC = MCP3204(SPISettings(115200, MSBFIRST, SPI_MODE0), D21);
auto ADJUST_PIN = D16;

const int CALC_INTENSITY_RATE = 1;
const int ACC_REPORT_RATE = 5;

const int samplingRate = 100;
const int bufferSize = samplingRate;

Filter FILTER = Filter(samplingRate);

void setup()
{
    pinMode(ADJUST_PIN, INPUT_PULLDOWN);

    Serial.begin(1200);

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
}

// オフセットを計算する
void calcOffset(int *buf, int *off)
{
    float x = 0, y = 0, z = 0;
    for (int i = 0; i < bufferSize; i++)
    {
        x += buf[i * 3 + 0];
        y += buf[i * 3 + 1];
        z += buf[i * 3 + 2];
    }
    off[0] = (int)(x / bufferSize);
    off[1] = (int)(y / bufferSize);
    off[2] = (int)(z / bufferSize);
}

const int RETENTION_MICRO_SECONDS = 60 * 10 * 1000000;
int x = 0, y = 0, z = 0;

// TODO: ここらへんの変数を整理する必要がある
int rawData[bufferSize * 3];
int offsettedData[bufferSize * 3];
float filteredData[bufferSize * 3];
float compositeData[bufferSize];
float compositeSortedData[bufferSize];

// 3-axis adjust value
int offset[3];
// Adujusting time( offset_counter / sampling rate = sec)
int offsetCounter = bufferSize * 5;
bool isOffsetted = false;

unsigned long frame = 0;

unsigned long latestMaxTime;
JmaIntensity latestIntensity;
JmaIntensity maxIntensity;

void loop()
{
    auto startTime = micros();

    auto index = frame % bufferSize;

    // 値を拾ってくる
    uint16_t x = ADC.read(AdcChannel::X), y = ADC.read(AdcChannel::Y), z = ADC.read(AdcChannel::Z);

    // 極値を弾く
    if (x < 0 || x > 4096 || x == 1024 || x == 2048 || x == 3072)
        x = offset[0];
    if (y < 0 || y > 4096 || y == 1024 || y == 2048 || y == 3072)
        y = offset[1];
    if (z < 0 || z > 4096 || z == 1024 || z == 2048 || z == 3072)
        z = offset[2];

    // 計測した値を取得
    rawData[index * 3 + 0] = x;
    rawData[index * 3 + 1] = y;
    rawData[index * 3 + 2] = z;

    // オフセットの計算がされていない場合オフセットを計算する
    if (!isOffsetted)
        calcOffset(rawData, offset);
    else if (frame % (samplingRate / ACC_REPORT_RATE) == 0)
        printNmea("XSACC,%.3f,%.3f,%.3f,", (x - offset[0]) / 1024.0 * 980, (y - offset[1]) / 1024.0 * 980, (z - offset[2]) / 1024.0 * 980);

    // オフセットを加味した値をセット
    offsettedData[index * 3 + 0] = x - offset[0];
    offsettedData[index * 3 + 1] = y - offset[1];
    offsettedData[index * 3 + 2] = z - offset[2];

    // 直近の５フレーム分をフィルタに掛ける(3軸分)
    for (int a = 0; a < 3; a++)
    {
        float offsettedDataset[5];
        for (int i = 0; i < 5; i++)
        {
            auto lIndex = (frame - 5 + i + 1) % bufferSize;
            // G to Gal.
            offsettedDataset[i] = offsettedData[lIndex * 3 + a] / 1024.0 * 980;
        }
        filteredData[index * 3 + a] = FILTER.execFilterAndPop(offsettedDataset, 5);
    }

    // 3軸合成
    compositeData[index] = sqrt(
        filteredData[index * 3 + 0] * filteredData[index * 3 + 0] +
        filteredData[index * 3 + 1] * filteredData[index * 3 + 1] +
        filteredData[index * 3 + 2] * filteredData[index * 3 + 2]);

    if (isOffsetted && frame % (samplingRate / CALC_INTENSITY_RATE) == 0)
    {
        memcpy(compositeSortedData, compositeData, sizeof(compositeData[0]) * bufferSize);
        sortArray(compositeSortedData, bufferSize);
        auto gal = compositeSortedData[-int(samplingRate * 0.3)];
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
        offsetCounter = bufferSize * 5;
        latestMaxTime = micros();
        maxIntensity = JMA_INT_0;
        printNmea("XSOFF,1");
    }
}
