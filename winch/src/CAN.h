#ifndef CAN_H
#define CAN_H

#include "driver/twai.h"

// CAN General Configuration
extern const twai_general_config_t g_config;
extern const twai_timing_config_t t_config;
extern const twai_filter_config_t f_config;

// Setup function for CAN operations
void can_setup();

// Functions to handle CAN operations
void can_send_message();
void can_receive_message();

// Function to send a custom CAN message
void send_custom_can_message(uint8_t controller_id, int16_t amps);

// CAN packet IDs
#define CAN_PACKET_SET_CURRENT 1

#endif // CAN_H