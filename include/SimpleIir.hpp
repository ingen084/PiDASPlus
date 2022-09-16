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
        dlyY = new float[coefBLen];
        /* init parameters */
        this->coefBLen = coefBLen;
        this->coefALen = coefALen;
        this->coefsB = coefsB;
        this->coefsA = coefsA;
        for (int i = 0; i < coefBLen; i++)
            dlyX[i] = 0.0;
        for (int j = 0; j < coefALen; j++)
            dlyY[0] = 0.0;
    }

    float filter(float input)
    {
        float acc1 = 0.0;
        float acc2 = 0.0;
        /* b coeficients*/
        dlyX[0] = input;
        for (int i = 0; i < coefBLen; i++)
            acc1 += coefsB[i] * dlyX[i];
        for (int i = (coefBLen) - 1; i > 0; i--)
            dlyX[i] = dlyX[i - 1];    
        /* a coeficients*/
        
        for (int i = 1; i < coefALen; i++)
            acc1 -= coefsA[i] * dlyY[i];

        dlyY[0] = (acc1+acc2)/coefsA[0];
        
        for (int i = (coefALen) - 1; i > 0; i--)
            dlyY[i] = dlyY[i - 1];  

        return dlyY[0];
    }
};
