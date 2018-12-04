#ifndef ODE_CMDS_H
#define ODE_CMDS_H

#define ODE_BLINK_LED_505L_CMD 4
#define ODE_BLINK_LED_505L_RESP (ODE_BLINK_LED_505L_CMD | 0x80)

#define ODE_BLINK_CREE_CMD 7
#define ODE_BLINK_CREE_RESP (ODE_BLINK_CREE_CMD | 0x80)

#define ODE_BURN_BALL1_CMD 10
#define ODE_BURN_BALL1_RESP (ODE_BURN_BALL1_CMD | 0x80)

struct ODEStatus {
   uint8_t ball1_sw;
   uint8_t ball2_sw;
   uint8_t MW_sw;
   uint8_t ball1_fb;
   uint8_t ball2_fb;
   uint8_t MW_fb;
   uint8_t cree_led;
   uint8_t led_505L;
   uint8_t led_645L;
   uint8_t led_851L;
} __attribute__((packed));

struct ODEBlinkData {
   uint32_t period;
   int32_t duration;
} __attribute__((packed));

struct ODEDeployData {
   uint32_t duration;
} __attribute__((packed));
 #endif
