/**
 * @file paylaod.c Example libproc process
 *
 */

#include <polysat/polysat.h>
#include <stdio.h>

static ProcessData *gProc = NULL;

// Function called when a status command is sent
void payload_status(int socket, unsigned char cmd, void * data, size_t dataLen,
                     struct sockaddr_in * src)
{
   char status = 0;

   PROC_cmd_sockaddr(gProc, CMD_STATUS_RESPONSE, &status,
        sizeof(status), src);
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
   // Initialize the process
   gProc = PROC_init("payload");
   DBG_setLevel(DBG_LEVEL_ALL);

   // Add a signal handler call back for SIGINT signal
   PROC_signal(gProc, SIGINT, &sigint_handler, PROC_evt(gProc));

   // Enter the main event loop
   EVT_start_loop(PROC_evt(gProc));

   // Clean up, whenever we exit event loop
   DBG_print(DBG_LEVEL_INFO, "Cleaning up\n");

   PROC_cleanup(gProc);

   return 0;
}

