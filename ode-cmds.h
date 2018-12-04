#ifndef ODE_CMDS_H
#define ODE_CMDS_H

#define ODE_STOP_ALL_LED_CMD 0
#define ODE_STOP_ALL_LED (ODE_STOP_ALL_LED_CMD | 0x80)

#define ODE_BLINK_LED_505L_CMD 4
#define ODE_BLINK_LED_505L_RESP (ODE_BLINK_LED_505L_CMD | 0x80)

#define ODE_BLINK_CREE_CMD 7
#define ODE_BLINK_CREE_RESP (ODE_BLINK_CREE_CMD | 0x80)

#define ODE_BURN_BALL1_CMD 10
#define ODE_BURN_BALL1_RESP (ODE_BURN_BALL1_CMD | 0x80)

struct ODEStatus {
   uint8_t ball1_sw; //0
   uint8_t ball2_sw; 
   uint8_t MW_sw;    //2
   uint8_t ball1_fb;
   uint8_t ball2_fb; //4
   uint8_t MW_fb;
   uint8_t cree_led; //6
   uint8_t led_505L;
   uint8_t led_645L; //8
   uint8_t led_851L; //9
} __attribute__((packed));

struct ODEBlinkData {
   uint32_t period;
   int32_t duration;
} __attribute__((packed));

struct ODEDeployData {
   uint32_t duration;
} __attribute__((packed));
 #endif
