#pragma once

// LEDインジケータモジュール初期化
void led_indicator_init(void);

// 周期処理（バッテリ・BLE・レイヤー監視）
void led_indicator_loop(void);
