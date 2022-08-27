#include <Arduino.h>
#include <SPI.h>

class MCP3204
{
public:
    MCP3204(SPISettings spiSettings, uint8_t csPin)
    {
        this->spiSettings = spiSettings;
        this->csPin = csPin;
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
    }

    uint16_t read(uint8_t ch)
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
        (void) SPI.transfer(0x06 | (ch >> 2));
        t.msb = SPI.transfer(0xff & (ch << 6));
        t.lsb = SPI.transfer(0);
        SPI.endTransaction();
        digitalWrite(csPin, HIGH);
        
        return t.val;
    }

    float toGal(uint16_t val)
    {
        return val / 1024.0f * 980;
    }

private:
    SPISettings spiSettings;
    uint8_t csPin;
};
