#pragma once
/*----------------------------------------------------------
 *  Seeed XIAO nRF52840 + Wio SX1262 (SKU 102010710)
 *  Pin mapping: SEEED_XIAO_NRF_KIT_DEFAULT variant
 *  Source: Meshtastic firmware variant.h (v2.6.8+)
 *---------------------------------------------------------*/

// ═══════════════════════════════════════════════════════
// LoRa SX1262 pins (via B2B connector, kit default)
// ═══════════════════════════════════════════════════════
#define LORA_CS_PIN     D4    // SPI Chip Select
#define LORA_DIO1_PIN   D1    // IRQ / interrupt
#define LORA_BUSY_PIN   D3    // Busy signal
#define LORA_RST_PIN    D2    // Reset
#define LORA_RXEN_PIN   D5    // TX/RX switch (corrected from D4 in fw v2.6.8)

// SPI bus pins (standard XIAO SPI)
#define LORA_SCK_PIN    D8
#define LORA_MOSI_PIN   D10
#define LORA_MISO_PIN   D9

// ═══════════════════════════════════════════════════════
// Button & Buzzer
// ═══════════════════════════════════════════════════════
#define BTN_UP_PIN      D6    // UP: increment state
#define BTN_DN_PIN      D0    // DOWN: decrement / emergency stop
#define BUZZER_PIN      D7
