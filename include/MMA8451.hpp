#pragma once

#include <Arduino.h>
#include <Adafruit_MMA8451.h>

class MMA8451
{
    Adafruit_MMA8451 mma;

public:
    MMA8451()
    {
        mma = Adafruit_MMA8451();
    };

    void begin()
    {
        if (!mma.begin())
        {
            Serial.println("Couldnt start");
            while (1)
                ;
        }
        mma.setRange(MMA8451_RANGE_2_G);
        mma.writeRegister8(0x2A, 0x00 | 0x01 | 0x04); // LNOISE
        mma.writeRegister8(0x2B, 0x10);
    }

    void read(float *data, const float *def)
    {
        mma.read();
        data[0] = mma.x_g * 100.0;
        data[1] = mma.y_g * 100.0;
        data[2] = mma.z_g * 100.0;
    }

    ~MMA8451(){

    };
};
