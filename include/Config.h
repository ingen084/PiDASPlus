// ボード定義ファイル
#pragma once

#include <Arduino.h>

/**
 * adjustピン
 */
// adjust ピンを無効化する場合コメントアウトを外す
// #define DISABLE_ADJUST_PIN
// adjust ピンの番号
#define ADJUST_PIN D16

/**
 * 震度表記LED
 */
// 震度表記のLEDを使用しない場合はコメントアウトを外す
// #define DISABLE_INTENSITY_LED
// 震度表示LEDのピンオフセット
// オフセットを震度0として連続で10個のピンが使用される
#define INTENSITY_LED_PIN_OFFSET D6

/**
 * ADC
 * 別のADCを利用する場合はコンストラクタなどと併せて差し替えてください
 * 尚フィルタ特性の都合上、 100Hz 固定です
 */
#ifdef USE_MCP3204
#include "MCP3204.hpp"
// ADCクラス
#define ADC_CLASS MCP3204
// ADCクラスのコンストラクタ
#define ADC_CONSTRUCTOR MCP3204(SPISettings(115200, MSBFIRST, SPI_MODE0), D21)
// ADCの返り値に使用する型
#define ADC_RESULT_TYPE uint16_t
// ADCの返り値を gal に変換する式
#define ADC_RAW_TO_GAL(i) (i) / 1024.0f * 980.665f
#endif

#ifdef USE_MMA8451
#include "MMA8451.hpp"
// 加速度計クラス
#define ADC_CLASS MMA8451
// 加速度計クラスのコンストラクタ
#define ADC_CONSTRUCTOR MMA8451()
// 加速度の返り値に使用する型
#define ADC_RESULT_TYPE float
// ADCの返り値を gal に変換する式
#define ADC_RAW_TO_GAL(i) (i)
#endif