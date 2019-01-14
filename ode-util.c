/** 
 * Test code for process command handling
 * @author 
 *
 */

#include <polysat/polysat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "ode-cmds.h"

#define DFL_BALL_TIME_MS (5*1000)  		// 5 seconds
#define DFL_DOOR_TIME_MS (10*1000)  		// 15 seconds
#define DFL_BLINK_PERIOD_MS 1000		// 1 second
#define DFL_IR_PERIOD_MS (15*60*1000)		// 15 minutes
#define DFL_BLINK_DUR_MS (15*60*1000)		// 15 minutes
#define DFL_FB_DUR_MS (10)  			// 10 ms
#define WAIT_MS (4 * 1000)  			// 4 seconds

struct MulticallInfo;

static int ode_status(int, char**, struct MulticallInfo *);
static int ode_cree(int, char**, struct MulticallInfo *);
static int ode_led_505L(int, char**, struct MulticallInfo *);
static int ode_led_645L(int, char**, struct MulticallInfo *);
static int ode_led_851L(int, char**, struct MulticallInfo *);
static int ode_led_IR(int, char**, struct MulticallInfo *);
static int ode_test(int, char**, struct MulticallInfo *);
static int ode_deploy_small_ball(int, char**, struct MulticallInfo *);
static int ode_deploy_large_ball(int, char**, struct MulticallInfo *);
static int ode_deploy_door(int, char**, struct MulticallInfo *);
static int mw_status(int, char**, struct MulticallInfo *);
static int small_ball_status(int, char**, struct MulticallInfo *);
static int large_ball_status(int, char**, struct MulticallInfo *);
static int large_ball_status(int, char**, struct MulticallInfo *);
static int set_led_blink_delay(int, char**, struct MulticallInfo *);
static int set_led_blink_duration(int, char**, struct MulticallInfo *);
static int set_led_blink_period(int, char**, struct MulticallInfo *);

// struct holding all possible function calls
// running the executable with the - flags will call that function
// running without flags will print out this struct
struct MulticallInfo {
   int (*func)(int argc, char **argv, struct MulticallInfo *);
   const char *name;
   const char *opt;
   const char *help;
} multicall[] = {
   { &ode_status, "ode-status", "-S", "Display the current status of the ode-payload process" }, 
   { &ode_cree, "ode-cree", "-L1", "Blink Cree LED" }, 
   { &ode_led_505L, "ode-led_505L", "-L2", "Blink 505L LED" }, 
   { &ode_led_645L, "ode-led_645L", "-L3", "Blink 645L LED" }, 
   { &ode_led_851L, "ode-led_851L", "-L4", "Blink 851L LED" }, 
   { &ode_led_IR, "ode-led_IR", "-L5", "Blink IR LED" }, 
   { &ode_test, "ode-test", "-L6", "Test function building" }, 
   { &ode_deploy_small_ball, "ode-deploy_small_ball", "-B1", "Deploy small ball" }, 
   { &ode_deploy_large_ball, "ode-deploy_large_ball", "-B2", "Deploy large ball" }, 
   { &ode_deploy_door, "ode-deploy_door", "-B3", "Open door" }, 
   { &small_ball_status, "ode-small_ball_status", "-FB1", "Check if the small ball is deployed." }, 
   { &large_ball_status, "ode-large_ball_status", "-FB2", "Check if the large ball is deployed." }, 
   { &mw_status, "ode-mw_status", "-FB3", "Check if the door is open." }, 
   { &set_blink_delay, "ode-set_LED_blink_delay", "-SV1", "Set time, in seconds, to wait until led blinking starts." }, 
   { &set_blink_duration, "ode-set_LED_blink_duration", "-SV2", "Set the blink duration in seconds." }, 
   { &set_blink_period, "ode-set_LED_blink_period", "-SV3", "Set the time, in seconds, for the led to be on and off." }, 
  { NULL, NULL, NULL, NULL }
};

// Status call
//_____________________________________________________________________________________
static int ode_status(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      struct ODEStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }   
   
   printf("Small Ball deployment enabled: %d\n", resp.status.small_ball_sw);
   printf("Large Ball deployment enabled: %d\n", resp.status.large_ball_sw);
   printf("Meltwire deployment enabled: %d\n", resp.status.MW_sw);
   printf("Ball 1 fb sw: %d\n", resp.status.small_ball_fb);
   printf("Ball 2 fb sw: %d\n", resp.status.large_ball_fb);
   printf("Meltwire fb sw: %d\n", resp.status.MW_fb);
   printf("Cree LED status: %d\n", resp.status.cree_led);
   printf("LED 505L status: %d\n", resp.status.led_505L);
   printf("LED 645L status: %d\n", resp.status.led_645L);
   printf("LED 851L status: %d\n", resp.status.led_851L);   
   printf("LED IR status: %d\n", resp.status.led_IR);   
   printf("5V regulator status: %d\n", resp.status.enable_5V);   

   return 0;
}

//Set Values
//_____________________________________________________________________________________
static int set_blink_delay(int argc, char **argv, struct MulticallInfo * self) 
{
// struct to hold response from payload process
   struct {
      uint8_t cmd;
      struct ODEStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }   

   DFL_BLINK_DELAY_MS  = **argv*1000;
   printf("LED blink delay set to: %d\n", DFL_BLINK_DELAY_MS);	
	
   return 0;
}

static int set_blink_duration(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      struct ODEStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }   

   DFL_BLINK_PERIOD_MS  = **argv*1000;
   printf("LED blink period set to: %d\n", DFL_BLINK_PERIOD_MS);	
	
   return 0;
}

static int et_blink_period(int argc, char **argv, struct MulticallInfo * self) 
{
// struct to hold response from payload process
   struct {
      uint8_t cmd;
      struct ODEStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }   

   DFL_BLINK_DUR_MS  = **argv*1000;
   printf("LED blink duration set to: %d\n", DFL_BLINK_DUR_MS);	
	
   return 0;
}

//Led Commands
//_____________________________________________________________________________________
static int ode_cree(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEBlinkData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_BLINK_CREE_CMD;
   send.param.period = htonl(DFL_BLINK_PERIOD_MS);
   send.param.duration = htonl(DFL_BLINK_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:p:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
         case 'p':
            send.param.period = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_BLINK_CREE_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_CREE_RESP);
      return 5;
   }

   return 0;
}

static int ode_led_505L(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEBlinkData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_BLINK_LED_505L_CMD;
   send.param.period = htonl(DFL_BLINK_PERIOD_MS);
   send.param.duration = htonl(DFL_BLINK_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:p:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
         case 'p':
            send.param.period = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_BLINK_LED_505L_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_LED_505L_RESP);
      return 5;
   }

   return 0;
}

static int ode_led_645L(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEBlinkData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_BLINK_LED_645L_CMD;
   send.param.period = htonl(DFL_BLINK_PERIOD_MS);
   send.param.duration = htonl(DFL_BLINK_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:p:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
         case 'p':
            send.param.period = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_BLINK_LED_645L_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_LED_645L_RESP);
      return 5;
   }

   return 0;
}

static int ode_led_851L(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEBlinkData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_BLINK_LED_851L_CMD;
   send.param.period = htonl(DFL_BLINK_PERIOD_MS);
   send.param.duration = htonl(DFL_BLINK_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:p:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
         case 'p':
            send.param.period = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_BLINK_LED_851L_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_LED_851L_RESP);
      return 5;
   }

   return 0;
}

static int ode_led_IR(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEBlinkData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_BLINK_IR_LED_CMD;
   send.param.period = htonl(DFL_IR_PERIOD_MS);
   send.param.duration = htonl(DFL_BLINK_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:p:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
         case 'p':
            send.param.period = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_BLINK_IR_LED_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_IR_LED_RESP);
      return 5;
   }

   return 0;
}

//Deployments
//_____________________________________________________________________________________
static int ode_deploy_small_ball(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEDeployData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_DEPLOY_SMALL_BALL_CMD;
   send.param.duration = htonl(DFL_BALL_TIME_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_DEPLOY_SMALL_BALL_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_DEPLOY_SMALL_BALL_RESP);
      return 5;
   }

   return 0;
}

static int ode_deploy_large_ball(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEDeployData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_DEPLOY_LARGE_BALL_CMD;
   send.param.duration = htonl(DFL_BALL_TIME_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_DEPLOY_LARGE_BALL_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_DEPLOY_LARGE_BALL_RESP);
      return 5;
   }

   return 0;
}

static int ode_deploy_door(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEDeployData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_DEPLOY_DOOR_CMD;
   send.param.duration = htonl(DFL_DOOR_TIME_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_DEPLOY_DOOR_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_DEPLOY_DOOR_RESP);
      return 5;
   }

   return 0;
}

//Read functions
//_____________________________________________________________________________________
static int small_ball_status(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEFeedBackData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_SMALL_BALL_STATUS_CMD;
   send.param.duration = htonl(DFL_FB_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_SMALL_BALL_STATUS_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_SMALL_BALL_STATUS_RESP);
      return 5;
   }

   return 0;
}

static int large_ball_status(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEFeedBackData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_LARGE_BALL_STATUS_CMD;
   send.param.duration = htonl(DFL_FB_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_LARGE_BALL_STATUS_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_LARGE_BALL_STATUS_RESP);
      return 5;
   }

   return 0;
}

static int mw_status(int argc, char **argv, struct MulticallInfo * self) 
{
   // struct to hold response from payload process
   struct {
      uint8_t cmd;
      uint8_t resp;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct ODEFeedBackData param;
   } __attribute__((packed)) send;

   send.cmd = ODE_MW_STATUS_CMD;
   send.param.duration = htonl(DFL_FB_DUR_MS);
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:d:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.param.duration = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "payload", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ODE_MW_STATUS_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_MW_STATUS_RESP);
      return 5;
   }

   return 0;
}

static int ode_test(int argc, char **argv, struct MulticallInfo * self) {return 0;}

// prints out available commands for this util
static int print_usage(const char *name)
{
   struct MulticallInfo *curr;

   printf("lsb-util multicall binary, use the following names instead:\n");

   for (curr = multicall; curr->func; curr++) {
      printf("   %-16s %s\n", curr->name, curr->help);
   }

   return 0;
}

int main(int argc, char **argv) 
{   
   struct MulticallInfo *curr;
   char *exec_name;

   exec_name = rindex(argv[0], '/');
   if (!exec_name) {
      exec_name = argv[0];
   }
   else {
      exec_name++;
   }

   for (curr = multicall; curr->func; curr++) {
      if (!strcmp(curr->name, exec_name)) {
         return curr->func(argc, argv, curr);
      }
   }

   if (argc > 1) {
      for (curr = multicall; curr->func; curr++) {
         if (!strcmp(curr->opt, argv[1])) {
            return curr->func(argc - 1, argv + 1, curr);
         }
      }
   }
   else {
      return print_usage(argv[0]);
   }

   return 0;
}
