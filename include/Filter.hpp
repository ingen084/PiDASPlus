#include <Arduino.h>

#if !defined(PIDAS_FILTER_HPP)
#define PIDAS_FILTER_HPP

/**
 * このクラスの処理は特許を使用しています
 * @see https://github.com/nrck/PiDAS/blob/main/tool/filter.py
 */
class Filter
{
private:
    float dt;
    int len;
    const float f0 = 0.45, f1 = 7.0, f2 = 0.5, f3 = 12.0, f4 = 20.0, f5 = 30.0, h2a = 1.0, h2b = 0.75, h3 = 0.9, h4 = 0.6, h5 = 0.6, g = 1.262;
    float* outBuf[6];

    float *func_A15(float a0, float a1, float a2, float b0, float b1, float b2, const float *inData, float *outData)
    {
        outData[0] = outData[len - 2];
        outData[1] = outData[len - 1];
        for (int k = 2; k < len; k++)
            outData[k] = (-a1 * outData[k - 1] - a2 * outData[k - 2] + b0 * inData[k] + b1 * inData[k - 1] + b2 * inData[k - 2]) / a0;

        return outData;
    }
    float *func_A14(float hc, float fc, float *inData, float *outData)
    {
        // A14
        float omega_c = 2 * PI * fc;
        float a0 = 12 / (dt * dt) + (12 * hc * omega_c) / dt + (omega_c * omega_c);
        float a1 = 10 * (omega_c * omega_c) - 24 / (dt * dt);
        float a2 = 12 / (dt * dt) - (12 * hc * omega_c) / dt + (omega_c * omega_c);
        float b0 = omega_c * omega_c;
        float b1 = 10 * (omega_c * omega_c);
        float b2 = omega_c * omega_c;

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outData);
    }

    float *filter01(const float *inData)
    {
        float fa1 = f0;
        float fa2 = f1;

        // A11
        float omega_a1 = 2 * PI * fa1;
        float omega_a2 = 2 * PI * fa2;

        float a0 = 8 / (dt * dt) + (4 * omega_a1 + 2 * omega_a2) / dt + omega_a1 * omega_a2;
        float a1 = 2 * omega_a1 * omega_a2 - 16 / (dt * dt);
        float a2 = 8 / (dt * dt) - (4 * omega_a1 + 2 * omega_a2) / dt + omega_a1 * omega_a2;
        float b0 = 4 / (dt * dt) + 2 * omega_a2 / dt;
        float b1 = -8 / (dt * dt);
        float b2 = 4 / (dt * dt) - 2 * omega_a2 / dt;

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outBuf[0]);
    }
    float *filter02()
    {
        float fa3 = f1;

        // A12
        float omega_a3 = 2 * PI * fa3;
        float a0 = 16 / (dt * dt) + 17 * omega_a3 / dt + (omega_a3 * omega_a3);
        float a1 = 2 * omega_a3 * omega_a3 - 32 / (dt * dt);
        float a2 = 16 / (dt * dt) - 17 * omega_a3 / dt + (omega_a3 * omega_a3);
        float b0 = 4 / (dt * dt) + 8.5 * omega_a3 / dt + (omega_a3 * omega_a3);
        float b1 = 2 * omega_a3 * omega_a3 - 8 / (dt * dt);
        float b2 = 4 / (dt * dt) - 8.5 * omega_a3 / dt + (omega_a3 * omega_a3);

        return func_A15(a0, a1, a2, b0, b1, b2, outBuf[0], outBuf[1]);
    }
    float *filter03()
    {
        float hb1 = h2a;
        float hb2 = h2b;
        float fb = f2;

        // A13
        float omega_b = 2 * PI * fb;
        float a0 = 12 / (dt * dt) + (12 * hb2 * omega_b) / dt + (omega_b * omega_b);
        float a1 = 10 * (omega_b * omega_b) - 24 / (dt * dt);
        float a2 = 12 / (dt * dt) - (12 + hb2 * omega_b) / dt + (omega_b * omega_b);
        float b0 = 12 / (dt * dt) + (12 * hb1 * omega_b) / dt + (omega_b * omega_b);
        float b1 = 10 * (omega_b * omega_b) - 24 / (dt * dt);
        float b2 = 12 / (dt * dt) - (12 * hb1 * omega_b) / dt + (omega_b * omega_b);

        return func_A15(a0, a1, a2, b0, b1, b2, outBuf[1], outBuf[2]);
    }
    float *filter04()
    {
        float hc = h3;
        float fc = f3;

        return func_A14(hc, fc, outBuf[2], outBuf[3]);
    }
    float *filter05()
    {
        float hc = h4;
        float fc = f4;

        return func_A14(hc, fc, outBuf[3], outBuf[4]);
    }
    float *filter06()
    {
        float hc = h5;
        float fc = f5;

        return func_A14(hc, fc, outBuf[4], outBuf[5]);
    }
public:
    Filter(int samplingRate, int filterLength)
    {
        dt = 1.0 / samplingRate;
        len = filterLength;
        for (int i = 0; i < 6; i++)
            outBuf[i] = new float[len]{};
    }
    ~Filter()
    {
        for (int i = 0; i < 6; i++)
            delete outBuf[i];
    }

    // フィルタを掛けて末尾の要素を抜き出す
    float execFilterAndPop(const float *inData)
    {
        filter01(inData);
        filter02();
        filter03();
        filter04();
        filter05();
        filter06();
        return outBuf[5][len - 1] * g;
    }
};

#endif
