/**
 * @file paylaod.c Example libproc process
 *
 */

#include <polysat/polysat.h>
#include <polysat_drivers/drivers/gpio.h>
#include <stdio.h>
#include <string.h>
#include "ode-cmds.h"

struct ODEPayloadState {
	ProcessData *proc;
	struct GPIOSensor *cree;
	int cree_active;
	void *cree_blink_evt;
	void *cree_finish_evt;

	struct GPIOSensor *led_505L;
	int led_505L_active;
	void *led_505L_blink_evt;
	void *led_505L_finish_evt;
	
	struct GPIOSensor *Door_Feedback;
	int Door_Feedback_value;
	void *Door_Feedback_evt;
	void *Door_Feedback_finish;

	void *ball1_evt;
	struct GPIOSensor *deploy_ball1;
};

static struct ODEPayloadState *state = NULL;
//static struct ODEStatus *sc_status = NULL;
static char codes_for_status[10]={0};

// Function called when a status command is sent
void payload_status(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEStatus status;
   
	status.ball1_sw=codes_for_status[0];
	status.ball2_sw=codes_for_status[1];
	status.MW_sw=codes_for_status[2];
	status.ball1_fb=codes_for_status[3];
	status.ball2_fb=codes_for_status[4];
	status.MW_fb=codes_for_status[5];
	status.cree_led=codes_for_status[6];
	status.led_505L=codes_for_status[7];
	status.led_645L=codes_for_status[8];
	status.led_851L=codes_for_status[9];
	
   // Send the response
   PROC_cmd_sockaddr(state->proc, CMD_STATUS_RESPONSE, &status,
        sizeof(status), src);
}

static int blink_cree_cb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
//   struct ODEStatus *sc_status = (struct ODEStatus*)arg;

   // Invert our LED state
   state->cree_active = !state->cree_active;

   // Change the GPIO
   if (state->cree && state->cree->set)
      state->cree->set(state->cree, state->cree_active);
  
   codes_for_status[6]=1;
	 
   // Reschedule the event
   return EVENT_KEEP;
}

static int start_mw_fb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
   codes_for_status[5] = -1;
	
   // Read the GPIO
  state->Door_Feedback->read(state->Door_Feedback);
  codes_for_status[5]=state.Door_Feedback;
	
   // Reschedule the event
   return EVENT_KEEP;
}

static int blink_led_505L_cb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Invert our LED state
   state->led_505L_active = !state->led_505L_active;

   // Change the GPIO
   if (state->led_505L && state->led_505L->set)
      state->led_505L->set(state->led_505L, state->led_505L_active);

   // Reschedule the event
   return EVENT_KEEP;
}

static int stop_cree(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
 //  struct ODEStatus *sc_status = (struct ODEStatus*)arg;

   // Turn off the LED
   if (state->cree && state->cree->set)
      state->cree->set(state->cree, 0);

   // Remove the blink callback
   if (state->cree_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->cree_blink_evt);
      state->cree_blink_evt = NULL;
   }

   codes_for_status[6]=0;

   // Do not reschedule this event
   state->cree_finish_evt = NULL;
   return EVENT_REMOVE;
}

static int stop_led_505L(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Turn off the LED
   if (state->led_505L && state->led_505L->set)
      state->led_505L->set(state->led_505L, 0);

   // Remove the blink callback
   if (state->led_505L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_505L_blink_evt);
      state->led_505L_blink_evt = NULL;
   }

   // Do not reschedule this event
   state->led_505L_finish_evt = NULL;
   return EVENT_REMOVE;
}

void mw_status(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEFeedBackData *params = (struct ODEFeedBackData*)data;
   uint8_t resp = 0;

   if (dataLen != sizeof(*params))
      return;

   // Clean up from previous events, if any
   if (state->Door_Feedback_finish) {
      EVT_sched_remove(PROC_evt(state->proc), state->Door_Feedback_finish);
      state->Door_Feedback_finish = NULL;
   }
   if (state->Door_Feedback_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->Door_Feedback_evt);
      state->Door_Feedback_evt = NULL;
   }

   // Only check the LED if the period and duration are > 0
   if (ntohl(params->duration) > 0) {
  
      // Create the event to check the door
      state->Door_Feedback_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration)), &start_mw_fb, state);
   }
	
   PROC_cmd_sockaddr(state->proc, ODE_MW_STATUS_RESP , &resp,
        sizeof(resp), src);
}

void blink_cree(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEBlinkData *params = (struct ODEBlinkData*)data;
   uint8_t resp = 0;

   if (dataLen != sizeof(*params))
      return;

   // Clean up from previous events, if any
   if (state->cree_finish_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->cree_finish_evt);
      state->cree_finish_evt = NULL;
   }
   if (state->cree_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->cree_blink_evt);
      state->cree_blink_evt = NULL;
   }

   // Only drive the LED if the period and duration are > 0
   if (ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
      // Turn the LED on
      state->cree_active = 1;
      if (state->cree && state->cree->set)
         state->cree->set(state->cree, state->cree_active);

      // Create the blink event
      state->cree_blink_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->period)), &blink_cree_cb, state);

      // Create the event to stop blinking
      state->cree_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration)), &stop_cree, state);
   }

   PROC_cmd_sockaddr(state->proc, ODE_BLINK_CREE_RESP, &resp,
        sizeof(resp), src);
}

void blink_led_505L(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEBlinkData *params = (struct ODEBlinkData*)data;
   uint8_t resp = 0;

   if (dataLen != sizeof(*params))
      return;

   // Clean up from previous events, if any
   if (state->led_505L_finish_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_505L_finish_evt);
      state->led_505L_finish_evt = NULL;
   }
   if (state->led_505L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_505L_blink_evt);
      state->led_505L_blink_evt = NULL;
   }

   // Only drive the LED if the period and duration are > 0
   if (ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
      // Turn the LED on
      state->led_505L_active = 1;
      if (state->led_505L && state->led_505L->set)
         state->led_505L->set(state->led_505L, state->led_505L_active);

      // Create the blink event
      state->led_505L_blink_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->period)), &blink_led_505L_cb, state);

      // Create the event to stop blinking
      state->led_505L_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration)), &stop_led_505L, state);
   }

   PROC_cmd_sockaddr(state->proc, ODE_BLINK_LED_505L_RESP, &resp,
        sizeof(resp), src);
}

static int stop_ball1(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Turn off GPIO
   if (state->deploy_ball1 && state->deploy_ball1->set)
      state->deploy_ball1->set(state->deploy_ball1, 0);

   // Zero out our event state
   state->ball1_evt = NULL;

   // Tell the event system to not reschedule this event
   return EVENT_REMOVE;
}

void deploy_ball1(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEDeployData *param = (struct ODEDeployData*)data;
   uint8_t resp = 1;

   if (dataLen != sizeof(*param))
      return;

   // Remove any preexisting ball1 deployment events
   if (state->ball1_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->ball1_evt);
      state->ball1_evt = NULL;
   }

   // Drive the GPIO
   if (state->deploy_ball1 && state->deploy_ball1->set)
      state->deploy_ball1->set(state->deploy_ball1, 1);

   // Register async callback to disable GPIO
   state->ball1_evt = EVT_sched_add(PROC_evt(state->proc),
         EVT_ms2tv(ntohl(param->duration)), &stop_ball1, state);

   PROC_cmd_sockaddr(state->proc, ODE_BURN_BALL1_RESP, &resp,
        sizeof(resp), src);
}

// Simple SIGINT handler for cleanup
static int sigint_handler(int signum, void *arg)
{
   EVT_exit_loop(arg);
   return EVENT_KEEP;
}

int usage(const char *name)
{
   printf("Usage: %s\n"
          ""
          , name);

   return 0;
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

   // Initialize GPIOs
   state->deploy_ball1 = create_named_gpio_device("DEPLOY_BALL1");
   state->cree = create_named_gpio_device("CREE");
   state->led_505L = create_named_gpio_device("LED_505L");
   state->Door_Feedback = create_named_gpio_device("DOOR_FEEDBACK");

   // Add a signal handler call back for SIGINT signal
   PROC_signal(state->proc, SIGINT, &sigint_handler, PROC_evt(state->proc));

   // Enter the main event loop
   EVT_start_loop(PROC_evt(state->proc));

   // Clean up, whenever we exit event loop
   DBG_print(DBG_LEVEL_INFO, "Cleaning up\n");

   // Clean up the ball1 deployment event
   if (state->ball1_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->ball1_evt);

   if (state->deploy_ball1) {
      // Turn off the ball1 GPIO if able
      if (state->deploy_ball1->set)
         state->deploy_ball1->set(state->deploy_ball1, 0);
      // Delete the ball1 GPIO sensor
      state->deploy_ball1->sensor.close((struct Sensor **)&state->deploy_ball1);
   }

   if (state->cree) {
      // Turn off the cree GPIO if able
      if (state->cree->set)
         state->cree->set(state->cree, 0);
	 codes_for_status[6]=0;
      // Delete the cree GPIO sensor
      state->cree->sensor.close((struct Sensor **)&state->cree);
   }
   
   if (state->led_505L) {
      // Turn off the led_505L GPIO if able
      if (state->led_505L->set)
         state->led_505L->set(state->led_505L, 0);
      // Delete the led_505L GPIO sensor
      state->led_505L->sensor.close((struct Sensor **)&state->led_505L);
   }

   PROC_cleanup(state->proc);

   return 0;
}
