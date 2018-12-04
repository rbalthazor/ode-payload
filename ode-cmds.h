#ifndef ODE_CMDS_H
#define ODE_CMDS_H

#define ODE_BLINK_LED_505L_CMD 4
#define ODE_BLINK_LED_505L_RESP (ODE_BLINK_LED_505L_CMD | 0x80)

#define ODE_BLINK_CREE_CMD 7
#define ODE_BLINK_CREE_RESP (ODE_BLINK_CREE_CMD | 0x80)

struct ODEStatus {
   uint8_t sw_1;
   uint8_t sw_2;
   uint8_t sw_3;
} __attribute__((packed));

struct ODEBlinkData {
   uint32_t period;
   int32_t duration;
} __attribute__((packed));

struct ODEDeployData {
   uint32_t duration;
} __attribute__((packed));
 #endif
