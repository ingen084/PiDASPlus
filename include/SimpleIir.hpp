#pragma once

class SimpleIir
{
private:
    int coefALen;
    float *coefsA;
    int coefBLen;
    float *coefsB;
    float *dlyX;
    float *dlyY;

public:
    SimpleIir() {}
    SimpleIir(int coefBLen, float *coefsB, int coefALen, float *coefsA)
    {
        /* memory allocation for H and delay values */
        dlyX = new float[coefBLen];
        dlyY = new float[coefALen];
        /* init parameters */
        this->coefBLen = coefBLen;
        this->coefALen = coefALen;
        this->coefsB = coefsB;
        this->coefsA = coefsA;
        for (int i = 0; i < coefBLen; i++)
            dlyX[i] = 0.0;
        for (int j = 0; j < coefALen; j++)
        {
            dlyY[j] = 0.0;
#ifdef DEBUG
            Serial.printf("dlyY:%f \r\n", dlyY);
#endif
        }
    }

    void reset()
    {
        for (int i = 0; i < coefBLen; i++)
            dlyX[i] = 0.0;
        for (int j = 0; j < coefALen; j++)
            dlyY[j] = 0.0;
    }

    float filter(float input)
    {
        float acc1 = 0.0;
        float acc2 = 0.0;
        /* b coeficients*/
        dlyX[0] = input;

#ifdef DEBUG
        Serial.printf("input:%f \r\n", input);
#endif
        for (int i = 0; i < coefBLen; i++)
        {
            acc1 += coefsB[i] * dlyX[i];

#ifdef DEBUG
            Serial.printf("coefsB[%d]:%f \r\n", i, coefsB[i]);
            Serial.printf("dlyx[%d]:%f \r\n", i, dlyX[i]);
            Serial.printf("acc1_incremental:%f \r\n", acc1);
#endif
        }
        for (int i = (coefBLen)-1; i > 0; i--)
            dlyX[i] = dlyX[i - 1];
        /* a coeficients*/
        for (int i = 1; i < coefALen; i++)
        {
            acc1 -= coefsA[i] * dlyY[i];
#ifdef DEBUG
            Serial.printf("coefsA[%d]:%f \r\n", i, coefsA[i]);
            Serial.printf("dlyY[%d]:%f \r\n", i, dlyY[i]);
            Serial.printf("acc1_decremental:%f \r\n", acc1);
#endif
        }
#ifdef DEBUG
        Serial.printf("FINAL***\r\n acc1:%f \r\n", acc1);
        Serial.printf("acc2:%f \r\n", acc2);
        Serial.printf("coefsA[%d]:%f \r\n", 0, coefsA[0]);
#endif
        dlyY[0] = (acc1 + acc2) / coefsA[0];

        for (int i = (coefALen)-1; i > 0; i--)
            dlyY[i] = dlyY[i - 1];
#ifdef DEBUG
        Serial.printf("dlyY[0]:%f \r\n", dlyY[0]);
#endif
        return dlyY[0];
    }
};
