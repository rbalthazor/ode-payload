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

#define DFL_BALL_TIME_MS (15*1000)
#define DFL_BLINK_PERIOD_MS 1000
#define DFL_BLINK_DUR_MS (15*60*1000)
#define WAIT_MS (4 * 1000)

struct MulticallInfo;

static int ode_status(int, char**, struct MulticallInfo *);
static int ode_led_505L(int, char**, struct MulticallInfo *);
static int ode_led_645L(int, char**, struct MulticallInfo *);
static int ode_led_851L(int, char**, struct MulticallInfo *);
static int ode_sm_ball(int, char**, struct MulticallInfo *);
static int ode_lg_ball(int, char**, struct MulticallInfo *);
static int ode_mw(int, char**, struct MulticallInfo *);
static int ode_mw_fb(int, char**, struct MulticallInfo *);
static int ode_sm_ball_fb(int, char**, struct MulticallInfo *);
static int ode_lg_ball_fb(int, char**, struct MulticallInfo *);
static int ode_led_irfb (int, char**, struct MulticallInfo *);

// struct holding all possible function calls
// running the executable with the - flags will call that function
// running without flags will print out this struct
struct MulticallInfo {
   int (*func)(int argc, char **argv, struct MulticallInfo *);
   const char *name;
   const char *opt;
   const char *help;
} multicall[] = {
   { &ode_status, "ode-status", "-S", 
       "Display the current status of the ode-payload process" }, 
   { &ode_led_505L, "ode-505L", "-L1", "Blink 505L" }, 
   { &ode_led_645L, "ode-645L", "-L2", "Blink 645L" }, 
   { &ode_led_851L, "ode-851L", "-L3", "Blink 851L" }, 
   { &ode_led_CREE, "ode-CREE", "-L4", "Blink CREE" }, 
   { &ode_sm_ball, "ode-ball1", "-B1", "Deploy small ball" }, 
   { &ode_lg_ball, "ode-ball2", "-B2", "Deploy large ball" }, 
   { &ode_mw, "ode-door", "-mw", "Open door" }, 
   { &ode_mw_fb, "ode-doorFB", "-FB1", "Query door Feedback" }, 
   { &ode_sm_ball_fb, "ode-ball1FB", "-FB2", "Query small ball Feedback" }, 
   { &ode_lg_ball_fb, "ode-ball2FB", "-FB3", "Query large ball Feedback" }, 
   { &ode_led_irfb, "ode-ball2FB", "-L5", "Enable the feedback IR source" }, 
   { NULL, NULL, NULL, NULL }
};

static int ode_ball1(int argc, char **argv, struct MulticallInfo * self) 
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

   send.cmd = ODE_BURN_BALL1_CMD;
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
 
   if (resp.cmd != ODE_BURN_BALL1_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BURN_BALL1_RESP);
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

   send.cmd = ODE_BLINK_LED1_CMD;
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
 
   if (resp.cmd != ODE_BLINK_LED1_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ODE_BLINK_LED1_RESP);
      return 5;
   }

   return 0;
}

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

   // print out returned status values   
   // printf("Total Packets Read: %d\n", ntohl(resp.status.totalSerRead));
   printf("SW 1: %d\n", resp.status.sw_1);
   printf("SW 2: %d\n", resp.status.sw_2);
   printf("SW 3: %d\n", resp.status.sw_3);
   
   return 0;
}

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
