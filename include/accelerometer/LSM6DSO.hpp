#pragma once

#include <Arduino.h>
#include <LSM6DSOSensor.h>

class LSM6DSO {
    LSM6DSOSensor *sensor;
    float sensitivity;

public:
    void begin() {
        SPI.setTX(D10);
        SPI.setRX(D9);
        SPI.setSCK(D8);
        SPI.begin();
        this->sensor = new LSM6DSOSensor(&SPI, D3);
        if (this->sensor->begin() == LSM6DSOStatusTypeDef::LSM6DSO_ERROR)
        {
            while (true) {
                Serial.println("begin failed!");
                delay(1000);
            }
        }

        this->sensor->Enable_X();
        this->sensor->Set_X_ODR(104);
        this->sensor->Get_X_Sensitivity(&this->sensitivity);
    }

    void read(float* data, const float* def)
    {
        int16_t rawData[3];
        this->sensor->Get_X_AxesRaw(rawData);

        data[0] = (float)rawData[0] * this->sensitivity;
        data[1] = (float)rawData[1] * this->sensitivity;
        data[2] = (float)rawData[2] * this->sensitivity;
    }
};
