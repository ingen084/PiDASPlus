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
    const float f0 = 0.45, f1 = 7.0, f2 = 0.5, f3 = 12.0, f4 = 20.0, f5 = 30.0, h2a = 1.0, h2b = 0.75, h3 = 0.9, h4 = 0.6, h5 = 0.6, g = 1.262, pi = PI;

    float *func_A15(float a0, float a1, float a2, float b0, float b1, float b2, float *inData, float *outData, int len)
    {
        // 1つめ
        float k1 = (b0 * inData[0]) / a0;
        outData[0] = k1;

        // 2つめ
        float k2 = (-a1 * outData[0] + b0 * inData[1] + b1 * inData[0]) / a0;
        outData[1] = k2;

        // 3つめ以降
        for (int k = 2; k < len; k++)
        {
            float value = (-a1 * outData[k - 1] - a2 * outData[k - 2] + b0 * inData[k] + b1 * inData[k - 1] + b2 * inData[k - 2]) / a0;
            outData[k] = value;
        }

        return outData;
    }
    float *func_A14(float hc, float fc, float *inData, float *outData, int len)
    {
        // A14
        float omega_c = 2 * pi * fc;
        float a0 = 12 / (dt * dt) + (12 * hc * omega_c) / dt + (omega_c * omega_c);
        float a1 = 10 * (omega_c * omega_c) - 24 / (dt * dt);
        float a2 = 12 / (dt * dt) - (12 * hc * omega_c) / dt + (omega_c * omega_c);
        float b0 = omega_c * omega_c;
        float b1 = 10 * (omega_c * omega_c);
        float b2 = omega_c * omega_c;

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outData, len);
    }

    float *filter01(float *inData, float *outData, int len)
    {
        float fa1 = f0;
        float fa2 = f1;

        // A11
        float omega_a1 = 2 * pi * fa1;
        float omega_a2 = 2 * pi * fa2;

        float a0 = 8 / (dt * dt) + (4 * omega_a1 + 2 * omega_a2) / dt + omega_a1 * omega_a2;
        float a1 = 2 * omega_a1 * omega_a2 - 16 / (dt * dt);
        float a2 = 8 / (dt * dt) - (4 * omega_a1 + 2 * omega_a2) / dt + omega_a1 * omega_a2;
        float b0 = 4 / (dt * dt) + 2 * omega_a2 / dt;
        float b1 = -8 / (dt * dt);
        float b2 = 4 / (dt * dt) - 2 * omega_a2 / dt;

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outData, len);
    }
    float *filter02(float *inData, float *outData, int len)
    {
        float fa3 = f1;

        // A12
        float omega_a3 = 2 * pi * fa3;
        float a0 = 16 / (dt * dt) + 17 * omega_a3 / dt + (omega_a3 * omega_a3);
        float a1 = 2 * omega_a3 * omega_a3 - 32 / (dt * dt);
        float a2 = 16 / (dt * dt) - 17 * omega_a3 / dt + (omega_a3 * omega_a3);
        float b0 = 4 / (dt * dt) + 8.5 * omega_a3 / dt + (omega_a3 * omega_a3);
        float b1 = 2 * omega_a3 * omega_a3 - 8 / (dt * dt);
        float b2 = 4 / (dt * dt) - 8.5 * omega_a3 / dt + (omega_a3 * omega_a3);

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outData, len);
    }
    float *filter03(float *inData, float *outData, int len)
    {
        float hb1 = h2a;
        float hb2 = h2b;
        float fb = f2;

        // A13
        float omega_b = 2 * pi * fb;
        float a0 = 12 / (dt * dt) + (12 * hb2 * omega_b) / dt + (omega_b * omega_b);
        float a1 = 10 * (omega_b * omega_b) - 24 / (dt * dt);
        float a2 = 12 / (dt * dt) - (12 + hb2 * omega_b) / dt + (omega_b * omega_b);
        float b0 = 12 / (dt * dt) + (12 * hb1 * omega_b) / dt + (omega_b * omega_b);
        float b1 = 10 * (omega_b * omega_b) - 24 / (dt * dt);
        float b2 = 12 / (dt * dt) - (12 * hb1 * omega_b) / dt + (omega_b * omega_b);

        return func_A15(a0, a1, a2, b0, b1, b2, inData, outData, len);
    }
    float *filter04(float *inData, float *outData, int len)
    {
        float hc = h3;
        float fc = f3;
        return func_A14(hc, fc, inData, outData, len);
    }
    float *filter05(float *inData, float *outData, int len)
    {
        float hc = h4;
        float fc = f4;
        return func_A14(hc, fc, inData, outData, len);
    }
    float *filter06(float *inData, float *outData, int len)
    {
        float hc = h5;
        float fc = f5;
        return func_A14(hc, fc, inData, outData, len);
    }
    float *filter07(float *inData, float *outData, int len)
    {
        float gd = g;

        for (int k = 0; k < len; k++)
            outData[k] = inData[k] * gd;

        return outData;
    }
public:
    Filter(int samplingRate)
    {
        dt = 1.0 / samplingRate;
    }

    // フィルタを掛けて末尾の要素を抜き出す
    float execFilterAndPop(float *inData, int len)
    {
        float outData[2][len]; // めっちゃ適当
        filter01(inData, outData[0], len);
        filter02(outData[0], outData[1], len);
        filter03(outData[1], outData[0], len);
        filter04(outData[0], outData[1], len);
        filter05(outData[1], outData[0], len);
        filter06(outData[0], outData[1], len);
        filter07(outData[1], outData[0], len);
        return outData[0][len - 1];
    }
};

#endif
