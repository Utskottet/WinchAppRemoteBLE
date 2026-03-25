#pragma once
/*----------------------------------------------------------
 *  Seeed XIAO nRF52840 + Wio SX1262 (SKU 102010710)
 *  Pin mapping: SEEED_XIAO_NRF_KIT_DEFAULT variant
 *---------------------------------------------------------*/

// LoRa SX1262 pins (B2B connector, kit default)
#define LORA_CS_PIN     D4
#define LORA_DIO1_PIN   D1
#define LORA_BUSY_PIN   D3
#define LORA_RST_PIN    D2
#define LORA_RXEN_PIN   D5    // RF switch: HIGH=RX, LOW=TX (Wio SX1262 B2B pin)

// SPI bus pins
#define LORA_SCK_PIN    D8
#define LORA_MOSI_PIN   D10
#define LORA_MISO_PIN   D9
