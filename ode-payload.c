#include <polysat/polysat.h>
#include <polysat_drivers/drivers/gpio.h>
#include <stdio.h>
#include <string.h>
#include "ode-cmds.h"

struct ODEPayloadState {
ProcessData *proc;
struct GPIOSensor *led1;
int led_active;
void *led_blink_evt;
void *led_finish_evt;

void *deploy_evt;
struct GPIOSensor *deploy;

void *feedback_evt;
struct GPIOSensor *feedback;
};

static struct ODEPayloadState *state = NULL;

/* // Function called when a status command is sent
void payload_status(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
	struct ODEStatus status;
	struct ODEBlinkData blink;
	struct ODEDeployData deploy;

	// Fill in the values we want to return to the requestor   
	status.door_sw=0;   	//0 for closed, 1 for opened after a successful switch read
	status.sm_ball_sw=0;	//0 for stowed, 1 for deployed after a successful IR switch read
	status.lg_ball_sw=0;	//0 for stowed, 1 for deployed after a successful IR switch read
	status.LED_505L=0;		//0 for on, 1 for off
	status.LED_645L=0;		//0 for on, 1 for off
	status.LED_851L=0;		//0 for on, 1 for off
	status.LED_CREE=0;		//0 for on, 1 for off
	status.Door=0;			//0 for an unburnt metlwire, 1 for a burnt meltwire
	status.SM_Ball=0;		//0 for an unfired nitonal, 1 if fired	
	status.LG_Ball=0;		//0 for an unfired nitonal, 1 if fired
	
	// Fill in the values we want for the blink period and duration of the LEDs
	blink.period_505L = 10;	//half period of a 50% duty cycle flash, set to 0 for continuous operation
	blink.duration_505L = 500;	//Duration for the LED to be operational
	blink.period_645L = 15;
	blink.duration_645L = 500;
	blink.period_851L = 20;
	blink.duration_851L = 500;
	blink.period_CREE = 25;
	blink.duration_CREE = 500;
	blink.period_IRFB = 0;
	blink.duration_IRFB = 500;
	
	// Fill in the values we want for the deployment durations
	deploy.Door_duration = 500;	//3 seconds	//Duration of operation for deployments
	deploy.SM_ball_duration = 500; 	//
	deploy.LG_ball_duration = 500;    //

	// Send the response
	PROC_cmd_sockaddr(state->proc, CMD_STATUS_RESPONSE, &status,
		sizeof(status), src);
} */

// Blink LED function group
static int blink_led_cb(int8_t LED)
{
	struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

	// Invert our LED state
	state->led_active = !state->led_active;

	// Change the GPIO
	if (state->LED && state->LED->set)
	  state->LED->set(state->LED, state->led_active);

	// Reschedule the event
	return EVENT_KEEP;
}

static int stop_led(int8_t LED)
{
	struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

	// Turn off the LED
	if (state->led && state->led->set)
	  state->led->set(state->led, 0);

	// Remove the blink callback
	if (state->led_blink_evt) {
	  EVT_sched_remove(PROC_evt(state->proc), state->led_blink_evt);
	  state->led_blink_evt = NULL;
	}

	// Do not reschedule this event
	state->led_finish_evt = NULL;
	return EVENT_REMOVE;
}

static int blink_led(int8_t gpio_used, int8_t RESPONSE, uint32_t period, int32_t duration)
{
	struct ODEBlinkData *params = (struct ODEBlinkData*)data;
	uint8_t resp = 0;
	
	if (dataLen != sizeof(*params))
	  return;

	// Clean up from previous events, if any
	if (state->led_finish_evt) {
	  EVT_sched_remove(PROC_evt(state->proc), state->led_finish_evt);
	  state->led_finish_evt = NULL;
	}
	if (state->led_blink_evt) {
	  EVT_sched_remove(PROC_evt(state->proc), state->led_blink_evt);
	  state->led_blink_evt = NULL;
	}

	// Only drive the LED if the period and duration are > 0
	if (ntohl(params->duration) > 0 && ntohl(params->period) > 0) {
	  // Turn the LED on
	  state->led_active = 1;
	  if (state->gpio_used && state->gpio_used->set)
		 state->gpio_used->set(state->gpio_used, state->led_active);

	  // Create the blink event
	  state->led_blink_evt = EVT_sched_add(PROC_evt(state->proc),
			EVT_ms2tv(ntohl(params->period)), &blink_led_cb(gpio_used), state);

	  // Create the event to stop blinking
	  state->led_finish_evt = EVT_sched_add(PROC_evt(state->proc),
			EVT_ms2tv(ntohl(params->duration)), &stop_led(gpio_used), state);
			
	// Just turn on the LED, not blinking, if the period is 0
	}else if(ntohl(params->period)==)){
		if (state->gpio_used && state->gpio_used->set)
			state->gpio_used->set(state->enable_gpio, 1);
		
		// Create the event to stop the LED
		state->led_finish_evt = EVT_sched_add(PROC_evt(state->proc),
			EVT_ms2tv(ntohl(params->duration)), &stop_led(gpio_used), state);	
	}

	PROC_cmd_sockaddr(state->proc, RESPONSE, &resp,
		sizeof(resp), src);
	
	return 1;
}

void blink_led_505L(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
		blink_led(LED_505L,ODE_BLINK_LED_505L_RESP,period_505L,duration_505L);
}

void blink_led_645L(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
		blink_led(LED_645L,ODE_BLINK_LED_645L_RESP,period_645L,duration_645L);	
}

void blink_led_851L(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
		blink_led(LED_851L,ODE_BLINK_LED_851L_RESP,period_851L,duration_851L);	
}

void blink_led_cree(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
		blink_led(LED_CREE,ODE_BLINK_LED_CREE_RESP,period_CREE,duration_CREE);
}

//
//Deploy function group
static int stop_deploy(uint8_t enable_gpio)
{
	struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

	// Turn off GPIO
	if (state->enable_gpio && state->enable_gpio->set)
	  state->enable_gpio->set(state->enable_gpio, 0);

	// Zero out our event state
	state->deploy_evt = NULL;

	// Tell the event system to not reschedule this event
	return EVENT_REMOVE;
}

static_int operate_deployment(int enable_gpio, int RESPONSE, uint32_t duration)
{
	struct ODEDeployData *param = (struct ODEDeployData*)data;
	uint8_t resp = 1;

	if (dataLen != sizeof(*param))
	  return;

	// Remove any preexisting ball1 deployment events
	if (state->deploy_evt) {
	  EVT_sched_remove(PROC_evt(state->proc), state->deploy_evt);
	  state->deploy_evt = NULL;
	}

	// Drive the GPIO
	if (state->enable_gpio && state->enable_gpio->set)
	  state->enable_gpio->set(state->enable_gpio, 1);

	// Register async callback to disable GPIO
	state->deploy_evt = EVT_sched_add(PROC_evt(state->proc),
		 EVT_ms2tv(ntohl(param->duration)), &stop_deploy(enable_gpio), state);

	PROC_cmd_sockaddr(state->proc, RESPONSE, &resp,
		sizeof(resp), src);	
	return 1;
}

void deploy_sm_ball(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
	operate_deployment(SM_ball_deploy, ODE_BURN_SM_BALL_RESP, SM_ball_duration);
}

void deploy_lg_ball(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
	operate_deployment(LG_ball_deploy, ODE_BURN_SM_BALL_RESP, LG_ball_duration);

}

void deploy_mw(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
{
	operate_deployment(MW_en, ODE_BURN_SM_BALL_RESP, Door_duration);
}

//
//Feedback function group
static int stop_feedback_led(void *arg)
{
	struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
	
	// Turn off the IR LED
	if (state->LED_IRFB && state->LED_IRFB->set)
	  state->LED_IRFB->set(state->LED_IRFB, 0);
  
	// Zero out our event state
	state->feedback_evt = NULL;

	// Tell the event system to not reschedule this event
	return EVENT_REMOVE;
}

static int start_feedback_led(void *arg)
{
	struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
	
	// Turn on the IR LED
	  state->LED_IRFB->set(state->LED_IRFB, 1);
	if (state->LED_IRFB && state->LED_IRFB->set)
  
	// Register async callback to disable GPIO
	state->feedback_evt = EVT_sched_add(PROC_evt(state->proc),
		 EVT_ms2tv(ntohl(param->duration)), &stop_deploy(LED_IRFB), state);

	// Tell the event system to not reschedule this event
	return EVENT_REMOVE;
}

static int read_feedback(uint8_t feedback_gpio)
{
	->set(state->LED_IRFB, 1);
	bool READ_VALUE = state->feedback_gpio;	
	return READ_VALUE;
}

void read_mw_fb(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
 {
	uint8_t resp = 1;
		
	int RESPONSE = 0;
	if(read_feedback(MW_FB)){
		RESPONSE=ODE_READ_MW_FB_HIGH_RESP;
	}else{
		RESPONSE=ODE_READ_MW_FB_LOW_RESP;
	}	
	PROC_cmd_sockaddr(state->proc, RESPONSE, &resp,
		sizeof(resp), src);	
}

void read_sm_ball_fb(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
 {
	uint8_t resp = 1;
	int RESPONSE = 0;
	
	start_feedback_led;
		
	if(read_feedback(SM_Ball_FB)){
		RESPONSE=ODE_READ_SM_BALL_FB_HGH_RESP;
	}else{
		RESPONSE=ODE_READ_SM_BALL_FB_LOW_RESP;
	}	
	PROC_cmd_sockaddr(state->proc, RESPONSE, &resp,
		sizeof(resp), src);	
		
	stop_feedback_led;
}

void read_lg_ball_fb(int socket, unsigned char cmd, void * data, size_t dataLen,
				 struct sockaddr_in * src)
 {
	start_feedback_led;
	uint8_t resp = 1;
		
	int RESPONSE = 0;
	if(read_feedback(LG_Ball_FB)){
		RESPONSE=ODE_READ_SM_BALL_FB_HIGH_RESP;
	}else{
		RESPONSE=ODE_READ_LG_BALL_FB_LOW_RESP;
	}	
	PROC_cmd_sockaddr(state->proc, RESPONSE, &resp,
		sizeof(resp), src);	

	stop_feedback_led;
}

// Entry point
int main(int argc, char *argv[])
{
	struct ODEPayloadState payload;

	memset(&payload, 0, sizeof(payload));
	state = &payload;

	// Initialize the process
	state->proc = PROC_init("payload", WD_ENABLED);
	DBG_setLevel(DBG_LEVEL_ALL);

	-------------------------------- // Initialize GPIOs
		//Deployment
	state->SM_ball_deploy = create_named_gpio_device("SM_Ball_Deploy");
	state->LG_ball_deploy = create_named_gpio_device("LG_Ball_Deploy");
	state->MW_en = create_named_gpio_device("Meltwire_Enable");

		//Feedback 
	state->LED_IRFB = create_named_gpio_device("IR_LED_Enable");
	state->SM_Ball_FB = create_named_gpio_device("SM_Ball_Feedback");
	state->LG_Ball_FB = create_named_gpio_device("LG_Ball_Feedback");
	state->MW_FB = create_named_gpio_device("Door_Feedback");

		//LEDs
	state->LED_505L = create_named_gpio_device("LED_505L_Enable");
	state->LED_645L = create_named_gpio_device("LED_645L_Enable");
	state->LED_851L = create_named_gpio_device("LED_851L_Enable");
	state->LED_CREE = create_named_gpio_device("CREE_Enable");

	---------------------------
	
	// Add a signal handler call back for SIGINT signal
	PROC_signal(state->proc, SIGINT, &sigint_handler, PROC_evt(state->proc));

	// Enter the main event loop
	EVT_start_loop(PROC_evt(state->proc));

	// Clean up, whenever we exit event loop
	DBG_print(DBG_LEVEL_INFO, "Cleaning up\n");

	// Clean up the deployment event
	if (state->deploy_evt)
	  EVT_sched_remove(PROC_evt(state->proc), state->deploy_evt);

	if (state->SM_ball_deploy) {
	  // Turn off the SM_ball_deploy GPIO if able
	  if (state->SM_ball_deploy->set)
		 state->SM_ball_deploy->set(state->SM_ball_deploy, 0);
	  // Delete the SM_ball_deploy GPIO sensor
	  state->SM_ball_deploy->sensor.close((struct Sensor **)&state->SM_ball_deploy);
	}
	if (state->LG_ball_deploy) {
	  // Turn off the LG_ball_deploy GPIO if able
	  if (state->LG_ball_deploy->set)
		 state->LG_ball_deploy->set(state->LG_ball_deploy, 0);
	  // Delete the LG_ball_deploy GPIO sensor
	  state->LG_ball_deploy->sensor.close((struct Sensor **)&state->LG_ball_deploy);
	}
	if (state->MW_en) {
	  // Turn off the MW_en GPIO if able
	  if (state->MW_en->set)
		 state->MW_en->set(state->MW_en, 0);
	  // Delete the MW_en GPIO sensor
	  state->MW_en->sensor.close((struct Sensor **)&state->MW_en);
	}

	//Cleanup LED event
	if (state->LED_CREE) {
	  // Turn off the LED_CREE GPIO if able
	  if (state->LED_CREE->set)
		 state->LED_CREE->set(state->LED_CREE, 0);
	  // Delete the LED_CREE GPIO sensor
	  state->LED_CREE->sensor.close((struct Sensor **)&state->LED_CREE);
	}
	if (state->LED_505L) {
	  // Turn off the LED_505L GPIO if able
	  if (state->LED_505L->set)
		 state->LED_505L->set(state->LED_505L, 0);
	  // Delete the LED_505L GPIO sensor
	  state->LED_505L->sensor.close((struct Sensor **)&state->LED_505L);
	}
	if (state->LED_645L) {
	  // Turn off the LED_645L GPIO if able
	  if (state->LED_645L->set)
		 state->LED_645L->set(state->LED_645L, 0);
	  // Delete the LED_645L GPIO sensor
	  state->LED_645L->sensor.close((struct Sensor **)&state->LED_645L);
	}
	if (state->LED_851L) {
	  // Turn off the LED_851L GPIO if able
	  if (state->LED_851L->set)
		 state->LED_851L->set(state->LED_851L, 0);
	  // Delete the LED_851L GPIO sensor
	  state->LED_851L->sensor.close((struct Sensor **)&state->LED_851L);
	}
	
	//Cleanup Feedback event
	if (state->IR_LED) {
	  // Turn off the IR_LED GPIO if able
	  if (state->IR_LED->set)
		 state->IR_LED->set(state->IR_LED, 0);
	  // Delete the IR_LED GPIO sensor
	  state->IR_LED->sensor.close((struct Sensor **)&state->IR_LED);
	}
	if (state->SM_Ball_FB) {
	  // Turn off the SM_Ball_FB GPIO if able
	  if (state->SM_Ball_FB->set)
		 state->SM_Ball_FB->set(state->SM_Ball_FB, 0);
	  // Delete the SM_Ball_FB GPIO sensor
	  state->SM_Ball_FB->sensor.close((struct Sensor **)&state->SM_Ball_FB);
	}
	if (state->LG_Ball_FB) {
	  // Turn off the LG_Ball_FB GPIO if able
	  if (state->LG_Ball_FB->set)
		 state->LG_Ball_FB->set(state->LG_Ball_FB, 0);
	  // Delete the LG_Ball_FB GPIO sensor
	  state->LG_Ball_FB->sensor.close((struct Sensor **)&state->LG_Ball_FB);
	}
	if (state->MW_FB) {
	  // Turn off the MW_FB GPIO if able
	  if (state->MW_FB->set)
		 state->MW_FB->set(state->MW_FB, 0);
	  // Delete the MW_FB GPIO sensor
	  state->MW_FB->sensor.close((struct Sensor **)&state->MW_FB);
	}
	
	PROC_cleanup(state->proc);

	return 0;
}
