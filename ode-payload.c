/**
 * @file paylaod.c Example libproc process
 *
 */

#include <polysat/polysat.h>
#include <polysat_drivers/drivers/gpio.h>
#include <stdio.h>
#include <string.h>
#include "ode-cmds.h"

#define FEEDBACK_POLL_INTV_MS 1000
#define DFL_BALL_TIME_MS (5*1000)
#define DFL_DOOR_TIME_MS (10*1000)
#define DFL_SMALL_BALL_TIME (60*60*24*135)
#define DFL_LARGE_BALL_TIME (60*60*24*120)
#define DFL_DOOR_TIME (60*60*24*60)

struct ODECriticalParams {
   uint32_t small_ball_time;
   uint32_t large_ball_time;
   uint32_t door_time;
   uint8_t small_ball_deployed;
   uint8_t large_ball_deployed;
   uint8_t door_deployed;
} __attribute__((packed));

struct ODEPayloadState {
	ProcessData *proc;
	
	struct GPIOSensor *enable_5V;
	int enable_5V_active;
	void *enable_5V_evt;
	void *enable_5V_finish_evt;
	
	struct GPIOSensor *cree;
	int cree_active;
	void *cree_blink_evt;
	void *cree_finish_evt;

	struct GPIOSensor *led_505L;
	int led_505L_active;
	void *led_505L_blink_evt;
	void *led_505L_finish_evt;
	
	struct GPIOSensor *led_645L;
	int led_645L_active;
	void *led_645L_blink_evt;
	void *led_645L_finish_evt;
	
	struct GPIOSensor *led_851L;
	int led_851L_active;
	void *led_851L_blink_evt;
	void *led_851L_finish_evt;
	
	struct GPIOSensor *led_IR;
	int led_IR_active;
	void *led_IR_blink_evt;
	void *led_IR_finish_evt;
	
	void *small_ball_evt, *small_ball_delay_evt;
   time_t small_ball_delay_time;
	struct GPIOSensor *deploy_small_ball;
	
	void *large_ball_evt, *large_ball_delay_evt;
   time_t large_ball_delay_time;
	struct GPIOSensor *deploy_large_ball;
	
	void *door_evt, *door_delay_evt;
   time_t door_delay_time;
	struct GPIOSensor *deploy_door;
	
	// struct GPIOSensor *Large_Ball_Feedback;
	void *feedback_evt;
};

static void setup_delayed_events(struct ODEPayloadState *ode);

static struct ODEPayloadState *state = NULL;
static char codes_for_status[12]={0};
static time_t times_for_status[3] = { 0, 0, 0 };

// Function called when a status command is sent
void payload_status(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEStatus status;
   time_t now;

   now = time(NULL);
   
   status.small_ball_sw=codes_for_status[0];
   status.large_ball_sw=codes_for_status[1];
   status.MW_sw=codes_for_status[2];
   status.small_ball_fb=codes_for_status[3];
   status.large_ball_fb=codes_for_status[4];
   status.MW_fb=codes_for_status[5];
   status.cree_led=codes_for_status[6];
   status.led_505L=codes_for_status[7];
   status.led_645L=codes_for_status[8];
   status.led_851L=codes_for_status[9];
   status.led_IR=codes_for_status[10];
   status.enable_5V=codes_for_status[11];
   status.small_ball_fb_time = htonl(times_for_status[0]);
   status.large_ball_fb_time = htonl(times_for_status[1]);
   status.MW_fb_time = htonl(times_for_status[2]);
   status.curr_time = htonl(time(NULL));

   status.time_until_small = 0;
   status.time_until_large = 0;
   status.time_until_door = 0;

   if (state->small_ball_delay_time > 0)
      status.time_until_small = htonl(state->small_ball_delay_time - now);
   if (state->large_ball_delay_time > 0)
      status.time_until_large = htonl(state->large_ball_delay_time - now);
   if (state->door_delay_time > 0)
      status.time_until_door = htonl(state->door_delay_time - now);

   // Send the response
   PROC_cmd_sockaddr(state->proc, CMD_STATUS_RESPONSE, &status,
        sizeof(status), src);
}

//__________________________________________________________________
//Blink LED call back functions
static void enable_5V(struct ODEPayloadState *state)
{
   if (state->enable_5V)
      state->enable_5V->sensor.close((struct Sensor**)&state->enable_5V);
   if (!state->enable_5V)
      state->enable_5V = create_named_gpio_device("ENABLE_5V");
   if (!state->enable_5V)
      return;

   // Turn on the 5V regualtor
   state->enable_5V_active = 1;

   // Change the GPIO
   if (state->enable_5V && state->enable_5V->set)
      state->enable_5V->set(state->enable_5V, state->enable_5V_active);
  
   codes_for_status[11]=1;
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

static int blink_led_645L_cb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Invert our LED state
   state->led_645L_active = !state->led_645L_active;

   // Change the GPIO
   if (state->led_645L && state->led_645L->set)
      state->led_645L->set(state->led_645L, state->led_645L_active);
  
   // Reschedule the event
   return EVENT_KEEP;
}

static int blink_led_851L_cb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Invert our LED state
   state->led_851L_active = !state->led_851L_active;

   // Change the GPIO
   if (state->led_851L && state->led_851L->set)
      state->led_851L->set(state->led_851L, state->led_851L_active);
  
   // Reschedule the event
   return EVENT_KEEP;
}

//__________________________________________________________________
//Stop LED call back functions
static void disable_5V(struct ODEPayloadState *state)
{
   // Turn off the 5V regualtor
   state->enable_5V_active = 0;

   // Change the GPIO
   if (state->enable_5V && state->enable_5V->set)
      state->enable_5V->set(state->enable_5V, state->enable_5V_active);
  
   codes_for_status[11]=0;

   if (state->enable_5V)
      state->enable_5V->sensor.close((struct Sensor**)&state->enable_5V);
}

static int stop_cree(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
 //  struct ODEStatus *sc_status = (struct ODEStatus*)arg;

   // Turn off the LED
   if (state->cree && state->cree->set)
      state->cree->set(state->cree, 0);
   if (state->cree)
      state->cree->sensor.close((struct Sensor**)&state->cree);

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
   if (state->led_505L)
      state->led_505L->sensor.close((struct Sensor**)&state->led_505L);

   // Remove the blink callback
   if (state->led_505L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_505L_blink_evt);
      state->led_505L_blink_evt = NULL;
   }
  
   disable_5V(state);
   codes_for_status[7]=0;

   // Do not reschedule this event
   state->led_505L_finish_evt = NULL;
   return EVENT_REMOVE;
}

static int stop_led_645L(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Turn off the LED
   if (state->led_645L && state->led_645L->set)
      state->led_645L->set(state->led_645L, 0);
   if (state->led_645L)
      state->led_645L->sensor.close((struct Sensor**)&state->led_645L);

   // Remove the blink callback
   if (state->led_645L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_645L_blink_evt);
      state->led_645L_blink_evt = NULL;
   }
  
   disable_5V(state);
   codes_for_status[8]=0;

   // Do not reschedule this event
   state->led_645L_finish_evt = NULL;
   return EVENT_REMOVE;
}

static int stop_led_851L(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   // Turn off the LED
   if (state->led_851L && state->led_851L->set)
      state->led_851L->set(state->led_851L, 0);
   if (state->led_851L)
      state->led_851L->sensor.close((struct Sensor**)&state->led_851L);

   // Remove the blink callback
   if (state->led_851L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_851L_blink_evt);
      state->led_851L_blink_evt = NULL;
   }
  
   disable_5V(state);  
   codes_for_status[9]=0;

   // Do not reschedule this event
   state->led_851L_finish_evt = NULL;
   return EVENT_REMOVE;
}

//__________________________________________________________________
//Blink LED functions

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
   if (state->cree)
      state->cree->sensor.close((struct Sensor**)&state->cree);
   codes_for_status[6]=0;

   if (!state->cree)
      state->cree = create_named_gpio_device("CREE");

   // Only drive the LED if the period and duration are > 0
   if (state->cree && ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
      codes_for_status[6]=1;
      state->cree_active = 0;

      if (state->cree && state->cree->set)
         state->cree->set(state->cree, state->cree_active);

      // Create the blink event
        state->cree_blink_evt = EVT_sched_add_with_timestep(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->delay)), EVT_ms2tv(ntohl(params->period)), &blink_cree_cb, state);	   

      // Create the event to stop blinking
      state->cree_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration) + ntohl(params->delay)), &stop_cree, state);
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

   if (state->led_505L)
      state->led_505L->sensor.close((struct Sensor**)&state->led_505L);
   codes_for_status[7]=0;
   if (!state->led_505L)
      state->led_505L = create_named_gpio_device("LED_505L");

   // Only drive the LED if the period and duration are > 0
   if (state->led_505L && ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
	
      enable_5V(state);   
	   
      codes_for_status[7]=1;

      state->led_505L_active = 0;
      if (state->led_505L && state->led_505L->set)
         state->led_505L->set(state->led_505L, state->led_505L_active);

      // Create the blink event
      state->led_505L_blink_evt = EVT_sched_add_with_timestep(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->delay)), EVT_ms2tv(ntohl(params->period)), &blink_led_505L_cb, state);	   
					   
      // Create the event to stop blinking
      state->led_505L_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration) + ntohl(params->delay)), &stop_led_505L, state);
   }

   PROC_cmd_sockaddr(state->proc, ODE_BLINK_LED_505L_RESP, &resp,
        sizeof(resp), src);
}

void blink_led_645L(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEBlinkData *params = (struct ODEBlinkData*)data;
   uint8_t resp = 0;

   if (dataLen != sizeof(*params))
      return;

   // Clean up from previous events, if any
   if (state->led_645L_finish_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_645L_finish_evt);
      state->led_645L_finish_evt = NULL;
   }
   if (state->led_645L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_645L_blink_evt);
      state->led_645L_blink_evt = NULL;
   }

   if (state->led_645L)
      state->led_645L->sensor.close((struct Sensor**)&state->led_645L);
   codes_for_status[8]=0;
   if (!state->led_645L)
      state->led_645L = create_named_gpio_device("LED_645L");

   // Only drive the LED if the period and duration are > 0
   if (state->led_645L && ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
	
      enable_5V(state);   
	   
      codes_for_status[8]=1;
      state->led_645L_active = 0;
      if (state->led_645L && state->led_645L->set)
         state->led_645L->set(state->led_645L, state->led_645L_active);

      // Create the blink event
      state->led_645L_blink_evt = EVT_sched_add_with_timestep(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->delay)), EVT_ms2tv(ntohl(params->period)), &blink_led_645L_cb, state);	   
					       
      // Create the event to stop blinking
      state->led_645L_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration) + ntohl(params->delay)), &stop_led_645L, state);
   }

   PROC_cmd_sockaddr(state->proc, ODE_BLINK_LED_645L_RESP, &resp,
        sizeof(resp), src);
}

void blink_led_851L(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEBlinkData *params = (struct ODEBlinkData*)data;
   uint8_t resp = 0;

   if (dataLen != sizeof(*params))
      return;

   // Clean up from previous events, if any
   if (state->led_851L_finish_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_851L_finish_evt);
      state->led_851L_finish_evt = NULL;
   }
   if (state->led_851L_blink_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->led_851L_blink_evt);
      state->led_851L_blink_evt = NULL;
   }

   if (state->led_851L)
      state->led_851L->sensor.close((struct Sensor**)&state->led_851L);
   codes_for_status[9]=0;
   if (!state->led_851L)
      state->led_851L = create_named_gpio_device("LED_851L");

   // Only drive the LED if the period and duration are > 0
   if (ntohl(params->period) > 0 && ntohl(params->duration) > 0) {
	
      enable_5V(state);   
	   
      codes_for_status[9]=1;
      state->led_851L_active = 0;
      if (state->led_851L && state->led_851L->set)
         state->led_851L->set(state->led_851L, state->led_851L_active);

      // Create the blink event
      state->led_851L_blink_evt = EVT_sched_add_with_timestep(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->delay)), EVT_ms2tv(ntohl(params->period)), &blink_led_851L_cb, state);	   
					       
      // Create the event to stop blinking
      state->led_851L_finish_evt = EVT_sched_add(PROC_evt(state->proc),
            EVT_ms2tv(ntohl(params->duration) + ntohl(params->delay)), &stop_led_851L, state);
   }

   PROC_cmd_sockaddr(state->proc, ODE_BLINK_LED_851L_RESP, &resp,
        sizeof(resp), src);
}

//__________________________________________________________________
//Deployment functions
//__________________________________________________________________
//Stop deployment LEDs

static int stop_small_ball(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
   struct ODECriticalParams cs;

   // Turn off GPIO
   if (state->deploy_small_ball && state->deploy_small_ball->set)
      state->deploy_small_ball->set(state->deploy_small_ball, 0);

   // Zero out our event state
   state->small_ball_evt = NULL;
  
   codes_for_status[0]=0;

   if (state->deploy_small_ball)
      state->deploy_small_ball->sensor.close((struct Sensor**)&state->deploy_small_ball);

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return EVENT_REMOVE;
   cs.small_ball_deployed = 1;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));
   state->small_ball_delay_time = 0;

   // Tell the event system to not reschedule this event
   return EVENT_REMOVE;
}

static int stop_large_ball(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
   struct ODECriticalParams cs;

   // Turn off GPIO
   if (state->deploy_large_ball && state->deploy_large_ball->set)
      state->deploy_large_ball->set(state->deploy_large_ball, 0);

   // Zero out our event state
   state->large_ball_evt = NULL;
  
   codes_for_status[1]=0;

   if (state->deploy_large_ball)
      state->deploy_large_ball->sensor.close((struct Sensor**)&state->deploy_large_ball);

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return EVENT_REMOVE;
   cs.large_ball_deployed = 1;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));
   state->large_ball_delay_time = 0;

   // Tell the event system to not reschedule this event
   return EVENT_REMOVE;
}


static int stop_door(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;
   struct ODECriticalParams cs;

   // Turn off GPIO
   if (state->deploy_door && state->deploy_door->set)
      state->deploy_door->set(state->deploy_door, 0);

   // Zero out our event state
   state->door_evt = NULL;
  
   codes_for_status[2]=0;   

   if (state->deploy_door)
      state->deploy_door->sensor.close((struct Sensor**)&state->deploy_door);
   // Tell the event system to not reschedule this event

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return EVENT_REMOVE;
   cs.door_deployed = 1;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));
   state->door_delay_time = 0;

   return EVENT_REMOVE;
}

//__________________________________________________________________
//deployment functions

static void deploy_small_ball_dur(uint32_t duration)
{
   // Remove any preexisting small_ball deployment events
   if (state->small_ball_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->small_ball_evt);
      state->small_ball_evt = NULL;
   }

   if (state->deploy_small_ball)
      state->deploy_small_ball->sensor.close((struct Sensor**)&state->deploy_small_ball);
   if (!state->deploy_small_ball)
      state->deploy_small_ball = create_named_gpio_device("DEPLOY_SMALL_BALL");

   // Drive the GPIO
   if (state->deploy_small_ball && state->deploy_small_ball->set){
	   
      state->deploy_small_ball->set(state->deploy_small_ball, 1);  
	  codes_for_status[0]=1;
   }

   // Register async callback to disable GPIO
   state->small_ball_evt = EVT_sched_add(PROC_evt(state->proc),
         EVT_ms2tv(duration), &stop_small_ball, state);
}

void deploy_small_ball(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEDeployData *param = (struct ODEDeployData*)data;
   uint8_t resp = 1;

   if (dataLen != sizeof(*param))
      return;

   deploy_small_ball_dur(ntohl(param->duration));

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_SMALL_BALL_RESP, &resp,
        sizeof(resp), src);
}

static void deploy_large_ball_dur(uint32_t duration)
{
   // Remove any preexisting large_ball deployment events
   if (state->large_ball_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->large_ball_evt);
      state->large_ball_evt = NULL;
   }

   if (state->deploy_large_ball)
      state->deploy_large_ball->sensor.close((struct Sensor**)&state->deploy_large_ball);
   if (!state->deploy_large_ball)
      state->deploy_large_ball = create_named_gpio_device("DEPLOY_LARGE_BALL");

   // Drive the GPIO
   if (state->deploy_large_ball && state->deploy_large_ball->set){
      state->deploy_large_ball->set(state->deploy_large_ball, 1);  
      codes_for_status[1]=1;
   }

   // Register async callback to disable GPIO
   state->large_ball_evt = EVT_sched_add(PROC_evt(state->proc),
         EVT_ms2tv(duration), &stop_large_ball, state);
}

void deploy_large_ball(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEDeployData *param = (struct ODEDeployData*)data;
   uint8_t resp = 1;

   if (dataLen != sizeof(*param))
      return;

   deploy_large_ball_dur(ntohl(param->duration));

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_LARGE_BALL_RESP, &resp,
        sizeof(resp), src);
}

void deploy_door_dur(uint32_t duration)
{
   // Remove any preexisting door deployment events
   if (state->door_evt) {
      EVT_sched_remove(PROC_evt(state->proc), state->door_evt);
      state->door_evt = NULL;
   }

   if (state->deploy_door)
      state->deploy_door->sensor.close((struct Sensor**)&state->deploy_door);
   if (!state->deploy_door)
      state->deploy_door = create_named_gpio_device("DEPLOY_DOOR");

   // Drive the GPIO
   if (state->deploy_door && state->deploy_door->set){
      state->deploy_door->set(state->deploy_door, 1);
	  codes_for_status[2]=1;
   }  

   // Register async callback to disable GPIO
   state->door_evt = EVT_sched_add(PROC_evt(state->proc),
         EVT_ms2tv(duration), &stop_door, state);
}

void deploy_door(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   struct ODEDeployData *param = (struct ODEDeployData*)data;
   uint8_t resp = 1;

   if (dataLen != sizeof(*param))
      return;

   deploy_door_dur(ntohl(param->duration));

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_DOOR_RESP, &resp,
        sizeof(resp), src);
}

//__________________________________________________________________
//Feedback start functions

static void poll_gpio(const char *name, int index, int time_index, struct ODEPayloadState *state)
{
   struct GPIOSensor *gpio;

   gpio = create_named_gpio_device(name);
   if (!gpio || !gpio->read)
      return;

   codes_for_status[index] = gpio->read(gpio);
   if (codes_for_status[index] && !times_for_status[time_index])
      times_for_status[time_index] = time(NULL);

   gpio->sensor.close((struct Sensor**)&gpio);
}

static int feedback_cb(void *arg)
{
   struct ODEPayloadState *state = (struct ODEPayloadState*)arg;

   poll_gpio("SM_BALL_FB", 3, 0, state);
   poll_gpio("LG_BALL_FB", 4, 1, state);
   poll_gpio("DOOR_FEEDBACK", 5, 2, state);

   return EVENT_KEEP;
}

//__________________________________________________________________
//Feedback commands

   // Create the event to check the Small_Ball
//   codes_for_status[5] = state->Small_Ball_Feedback->read(state->Small_Ball_Feedback);

static int deploy_small_ball_evt(void *arg)
{
   deploy_small_ball_dur(DFL_BALL_TIME_MS);
   return EVENT_REMOVE;
}

static int deploy_large_ball_evt(void *arg)
{
   deploy_large_ball_dur(DFL_BALL_TIME_MS);
   return EVENT_REMOVE;
}

static int deploy_door_evt(void *arg)
{
   deploy_door_dur(DFL_DOOR_TIME_MS);
   return EVENT_REMOVE;
}

static void setup_delayed_events(struct ODEPayloadState *ode)
{
   struct ODECriticalParams cs;
   struct timeval diff;
   time_t now;

   if (!ode)
      return;

   if (ode->small_ball_delay_evt)
      EVT_sched_remove(PROC_evt(ode->proc), ode->small_ball_delay_evt);
   if (ode->large_ball_delay_evt)
      EVT_sched_remove(PROC_evt(ode->proc), ode->large_ball_delay_evt);
   if (ode->door_delay_evt)
      EVT_sched_remove(PROC_evt(ode->proc), ode->door_delay_evt);
   ode->small_ball_delay_evt = NULL;
   ode->large_ball_delay_evt = NULL;
   ode->door_delay_evt = NULL;
   ode->small_ball_delay_time = 0;
   ode->large_ball_delay_time = ode->door_delay_time = 0;

   now = time(NULL);

   if (sizeof(cs) != PROC_read_critical_state(ode->proc, &cs, sizeof(cs))) {
      memset(&cs, 0, sizeof(cs));
      if (DFL_SMALL_BALL_TIME > 0)
         cs.small_ball_time = now + DFL_SMALL_BALL_TIME;
      if (DFL_LARGE_BALL_TIME > 0)
         cs.large_ball_time = now + DFL_LARGE_BALL_TIME;
      if (DFL_DOOR_TIME > 0)
         cs.door_time = now + DFL_DOOR_TIME;

      PROC_save_critical_state(ode->proc, &cs, sizeof(cs));
   }

   if (cs.small_ball_time > 0) {
      if (cs.small_ball_time < now) {
         if (!cs.small_ball_deployed)
            deploy_small_ball_evt(ode);
      }
      else {
         diff.tv_usec = 0;
         diff.tv_sec = cs.small_ball_time - now;
         ode->small_ball_delay_time = cs.small_ball_time;
         ode->small_ball_delay_evt = EVT_sched_add(PROC_evt(ode->proc),
               diff, &deploy_small_ball_evt, ode);
      }
   }

   if (cs.large_ball_time > 0) {
      if (cs.large_ball_time < now) {
         if (!cs.large_ball_deployed)
            deploy_large_ball_evt(ode);
      }
      else {
         diff.tv_usec = 0;
         diff.tv_sec = cs.large_ball_time - now;
         ode->large_ball_delay_time = cs.large_ball_time;
         ode->large_ball_delay_evt = EVT_sched_add(PROC_evt(ode->proc),
               diff, &deploy_large_ball_evt, ode);
      }
   }

   if (cs.door_time > 0) {
      if (cs.door_time < now) {
         if (!cs.door_deployed)
            deploy_door_evt(ode);
      }
      else {
         diff.tv_usec = 0;
         ode->door_delay_time = cs.door_time;
         diff.tv_sec = ode->door_delay_time - now;
         ode->door_delay_evt = EVT_sched_add(PROC_evt(ode->proc),
               diff, &deploy_door_evt, ode);
      }
   }

}

void deploy_door_delay(int socket, unsigned char cmd, void * data,
      size_t dataLen, struct sockaddr_in * src)
{
   struct ODEDeployDelayData *param = (struct ODEDeployDelayData*)data;
   uint8_t resp = 1;
   struct timeval depl_time;
   struct ODECriticalParams cs;

   if (dataLen != sizeof(*param))
      return;

   gettimeofday(&depl_time, NULL);

   if (ntohl(param->mode) == 1)
      depl_time.tv_sec = ntohl(param->delay);
   else if (ntohl(param->mode) == 2)
      depl_time.tv_sec += ntohl(param->delay);
   else
      return;

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return;
   cs.door_time = depl_time.tv_sec;
   cs.door_deployed = 0;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));

   setup_delayed_events(state);

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_DOOR_DELAY_RESP, &resp,
        sizeof(resp), src);
}

void deploy_small_ball_delay(int socket, unsigned char cmd, void * data,
      size_t dataLen, struct sockaddr_in * src)
{
   struct ODEDeployDelayData *param = (struct ODEDeployDelayData*)data;
   uint8_t resp = 1;
   struct timeval depl_time;
   struct ODECriticalParams cs;

   if (dataLen != sizeof(*param))
      return;

   gettimeofday(&depl_time, NULL);

   if (ntohl(param->mode) == 1)
      depl_time.tv_sec = ntohl(param->delay);
   else if (ntohl(param->mode) == 2)
      depl_time.tv_sec += ntohl(param->delay);
   else
      return;

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return;
   cs.small_ball_time = depl_time.tv_sec;
   cs.small_ball_deployed = 0;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));

   setup_delayed_events(state);

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_SMALL_BALL_DELAY_RESP, &resp,
        sizeof(resp), src);
}

void deploy_large_ball_delay(int socket, unsigned char cmd, void * data,
      size_t dataLen, struct sockaddr_in * src)
{
   struct ODEDeployDelayData *param = (struct ODEDeployDelayData*)data;
   uint8_t resp = 1;
   struct timeval depl_time;
   struct ODECriticalParams cs;

   if (dataLen != sizeof(*param))
      return;

   gettimeofday(&depl_time, NULL);

   if (ntohl(param->mode) == 1)
      depl_time.tv_sec = ntohl(param->delay);
   else if (ntohl(param->mode) == 2)
      depl_time.tv_sec += ntohl(param->delay);
   else
      return;

   if (sizeof(cs) != PROC_read_critical_state(state->proc, &cs, sizeof(cs)))
      return;
   cs.large_ball_time = depl_time.tv_sec;
   cs.large_ball_deployed = 0;
   PROC_save_critical_state(state->proc, &cs, sizeof(cs));

   setup_delayed_events(state);

   PROC_cmd_sockaddr(state->proc, ODE_DEPLOY_LARGE_BALL_DELAY_RESP, &resp,
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
   state->led_IR = create_named_gpio_device("LED_IR");
   if (state->led_IR && state->led_IR->set)
      state->led_IR->set(state->led_IR, 1);

   // Add a signal handler call back for SIGINT signal
   PROC_signal(state->proc, SIGINT, &sigint_handler, PROC_evt(state->proc));

   state->feedback_evt = EVT_sched_add(PROC_evt(state->proc),
      EVT_ms2tv(FEEDBACK_POLL_INTV_MS), &feedback_cb, state);
	
   setup_delayed_events(state);
   // Enter the main event loop
   EVT_start_loop(PROC_evt(state->proc));

   // Clean up, whenever we exit event loop
   DBG_print(DBG_LEVEL_INFO, "Cleaning up\n");
	
   // Clean up the input pins
   if (state->small_ball_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->small_ball_evt);
  
   if (state->deploy_small_ball) {
      // Turn off the ball1 GPIO if able
      if (state->deploy_small_ball->set)
         state->deploy_small_ball->set(state->deploy_small_ball, 0);
      // Delete the ball1 GPIO sensor
      state->deploy_small_ball->sensor.close((struct Sensor **)&state->deploy_small_ball);
   }   	

   if (state->feedback_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->feedback_evt);
  
   // Clean up the deployment events
   if (state->small_ball_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->small_ball_evt);
  
   if (state->deploy_small_ball) {
      // Turn off the ball1 GPIO if able
      if (state->deploy_small_ball->set)
         state->deploy_small_ball->set(state->deploy_small_ball, 0);
      // Delete the ball1 GPIO sensor
      state->deploy_small_ball->sensor.close((struct Sensor **)&state->deploy_small_ball);
   }   
   
   if (state->large_ball_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->large_ball_evt);
  
  if (state->deploy_large_ball) {
      // Turn off the ball1 GPIO if able
      if (state->deploy_large_ball->set)
         state->deploy_large_ball->set(state->deploy_large_ball, 0);
      // Delete the ball1 GPIO sensor
      state->deploy_large_ball->sensor.close((struct Sensor **)&state->deploy_large_ball);
   }   
     
   if (state->door_evt)
      EVT_sched_remove(PROC_evt(state->proc), state->door_evt);
  
  if (state->deploy_door) {
      // Turn off the ball1 GPIO if able
      if (state->deploy_door->set)
         state->deploy_door->set(state->deploy_door, 0);
      // Delete the ball1 GPIO sensor
      state->deploy_door->sensor.close((struct Sensor **)&state->deploy_door);
   } 

   // Clean up the LED events
   if (state->enable_5V) {
      // Turn off the cree GPIO if able
      if (state->enable_5V->set)
         state->enable_5V->set(state->enable_5V, 0);
	   codes_for_status[11]=0;
      // Delete the cree GPIO sensor
      state->enable_5V->sensor.close((struct Sensor **)&state->enable_5V);
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
   
   if (state->led_645L) {
      // Turn off the led_645L GPIO if able
      if (state->led_645L->set)
         state->led_645L->set(state->led_645L, 0);
      // Delete the led_645L GPIO sensor
      state->led_645L->sensor.close((struct Sensor **)&state->led_645L);
   }
      
   if (state->led_851L) {
      // Turn off the led_851L GPIO if able
      if (state->led_851L->set)
         state->led_851L->set(state->led_851L, 0);
      // Delete the led_851L GPIO sensor
      state->led_851L->sensor.close((struct Sensor **)&state->led_851L);
   }
      
   if (state->led_IR) {
      // Turn off the led_IR GPIO if able
      if (state->led_IR->set)
         state->led_IR->set(state->led_IR, 0);
      // Delete the led_IR GPIO sensor
      state->led_IR->sensor.close((struct Sensor **)&state->led_IR);
   }
   PROC_cleanup(state->proc);

   return 0;
}
