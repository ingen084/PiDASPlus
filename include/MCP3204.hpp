#pragma once

#include <Arduino.h>
#include <SPI.h>

class MCP3204
{
    SPISettings spiSettings;
    uint8_t csPin;

    uint16_t readChannel(const uint8_t ch, const uint16_t def)
    {
        union
        {
            uint16_t val;
            struct
            {
                uint8_t lsb;
                uint8_t msb;
            };
        } t;

        digitalWrite(csPin, LOW);
        SPI.beginTransaction(spiSettings);
        (void)SPI.transfer(0x06 | (ch >> 2));
        t.msb = SPI.transfer(0xff & (ch << 6));
        t.lsb = SPI.transfer(0);
        SPI.endTransaction();
        digitalWrite(csPin, HIGH);

        // 極値の場合はデフォルトの値を返す
        if (t.val < 0 || t.val > 4096 || t.val == 1024 || t.val == 2048 || t.val == 3072)
            return def;

        return t.val;
    }
public:
    MCP3204(SPISettings spiSettings, uint8_t csPin)
    {
        this->spiSettings = spiSettings;
        this->csPin = csPin;
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
    }

    void begin() {
        SPI.setRX(D20);
        SPI.setCS(D21);
        SPI.setSCK(D18);
        SPI.setTX(D19);
        SPI.begin();
    }

    void read(uint16_t* data, const uint16_t* def)
    {
        for (auto a = 0; a < 3; a++)
            data[a] = readChannel(a, def[a]);
    }
};
