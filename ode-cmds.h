#ifndef ODE_CMDS_H
#define ODE_CMDS_H

#define ODE_BLINK_LED_505L_CMD 4
#define ODE_BLINK_LED_505L_RESP (ODE_BLINK_LED_505L_CMD | 0x80)

#define ODE_BLINK_LED_645L_CMD 5
#define ODE_BLINK_LED_645L_RESP (ODE_BLINK_LED_645L_CMD | 0x80)

#define ODE_BLINK_LED_851L_CMD 6
#define ODE_BLINK_LED_851L_RESP (ODE_BLINK_LED_851L_CMD | 0x80)

#define ODE_BLINK_CREE_CMD 7
#define ODE_BLINK_CREE_RESP (ODE_BLINK_CREE_CMD | 0x80)

#define ODE_BURN_SM_BALL_CMD 8
#define ODE_BURN_SM_BALL_RESP (ODE_BURN_SM_BALL_CMD | 0x80)

#define ODE_BURN_LG_BALL_CMD 12
#define ODE_BURN_LG_BALL_RESP (ODE_BURN_LG_BALL_CMD | 0x80)

#define ODE_BURN_MELTWIRE_CMD 13
#define ODE_BURN_MELTWIRE_RESP (ODE_BURN_MELTWIRE_CMD | 0x80)

#define ODE_READ_MW_FB_CMD 1
#define ODE_READ_MW_FB_LOW_RESP (ODE_READ_MW_FB_CMD | 0x80)
#define ODE_READ_MW_FB_HIGH_RESP (ODE_READ_MW_FB_CMD+0x08 | 0x80) //Can be 1 or 9

#define ODE_READ_SM_BALL_FB_CMD 2
#define ODE_READ_SM_BALL_FB_LOW_RESP (ODE_READ_SM_BALL_FB_CMD | 0x80)
#define ODE_READ_SM_BALL_FB_HGH_RESP (ODE_READ_SM_BALL_FB_CMD+0x08 | 0x80) //can be 2 or 10

#define ODE_READ_LG_BALL_FB_CMD 3
#define ODE_READ_LG_BALL_FB_LOW_RESP (ODE_READ_LG_BALL_FB_CMD | 0x80)
#define ODE_READ_SM_BALL_FB_HIGH_RESP (ODE_READ_SM_BALL_FB_CMD+0x08 | 0x80) //can be 3 or 11

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
