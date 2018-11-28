#ifndef ODE_CMDS_H
#define ODE_CMDS_H

define ODE_BLINK_LED_505L_CMD 4
define ODE_BLINK_LED_505L_RESP (ODE_BLINK_LED_505L_CMD | 0x80)

define ODE_BLINK_LED_645L_CMD 5
define ODE_BLINK_LED_645L_RESP (ODE_BLINK_LED_645L_CMD | 0x80)

define ODE_BLINK_LED_851L_CMD 6
define ODE_BLINK_LED_851L_RESP (ODE_BLINK_LED_851L_CMD | 0x80)

define ODE_BLINK_LED_CREE_CMD 7
define ODE_BLINK_LED_CREE_RESP (ODE_BLINK_LED_CREE_CMD | 0x80)

define ODE_BURN_SM_BALL_CMD 8
define ODE_BURN_SM_BALL_RESP (ODE_BURN_SM_BALL_CMD | 0x80)

define ODE_BURN_LG_BALL_CMD 12
define ODE_BURN_LG_BALL_RESP (ODE_BURN_LG_BALL_CMD | 0x80)

define ODE_BURN_MELTWIRE_CMD 13
define ODE_BURN_MELTWIRE_RESP (ODE_BURN_MELTWIRE_CMD | 0x80)

define ODE_READ_MW_FB_CMD 1
define ODE_READ_MW_FB_LOW_RESP (ODE_READ_MW_FB_CMD | 0x80)
define ODE_READ_MW_FB_HIGH_RESP (ODE_READ_MW_FB_CMD+0x08 | 0x80) //Can be 1 or 9

define ODE_READ_SM_BALL_FB_CMD 2
define ODE_READ_SM_BALL_FB_LOW_RESP (ODE_READ_SM_BALL_FB_CMD | 0x80)
define ODE_READ_SM_BALL_FB_HGH_RESP (ODE_READ_SM_BALL_FB_CMD+0x08 | 0x80) //can be 2 or 10

define ODE_READ_LG_BALL_FB_CMD 3
define ODE_READ_LG_BALL_FB_LOW_RESP (ODE_READ_LG_BALL_FB_CMD | 0x80)
define ODE_READ_SM_BALL_FB_HIGH_RESP (ODE_READ_SM_BALL_FB_CMD+0x08 | 0x80) //can be 3 or 11

struct ODEStatus {
	uint8_t door_sw;   	//0 for closed, 1 for opened after a successful switch read
	uint8_t sm_ball_sw;	//0 for stowed, 1 for deployed after a successful IR switch read
	uint8_t lg_ball_sw;	//0 for stowed, 1 for deployed after a successful IR switch read
	uint8_t LED_505L;	//0 for on, 1 for off
	uint8_t LED_645L;	//0 for on, 1 for off
	uint8_t LED_851L;	//0 for on, 1 for off
	uint8_t LED_CREE;	//0 for on, 1 for off
	uint8_t LED_IRFB;	//0 for on, 1 for off
	uint8_t Door;		//0 for an unburnt metlwire, 1 for a burnt meltwire
	uint8_t SM_Ball;	//0 for an unfired nitonal, 1 if fired	
	uint8_t LG_Ball		//0 for an unfired nitonal, 1 if fired
} __attribute__((packed));

struct ODEBlinkData {
	uint32_t period_505L;	//half period of a 50% duty cycle flash, set to 0 for continuous operation
	int32_t duration_505L;	//Duration for the LED to be operational
	uint32_t period_645L;
	int32_t duration_645L;
	uint32_t period_851L;
	int32_t duration_851L;
	uint32_t period_CREE;
	int32_t duration_CREE;
	uint32_t period_IRFB;
	int32_t duration_IRFB
} __attribute__((packed));

struct ODEDeployData {
	uint32_t SM_ball_duration; //Duration of operation for deployments
	uint32_t LG_ball_duration;
	uint32_t Door_duration		
} __attribute__((packed));

#endif
