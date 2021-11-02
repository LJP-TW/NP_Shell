#ifndef SYS_VARIABLE_H
#define SYS_VARIABLE_H

#define MAX_CMDLINE_LEN 15001
#define MAX_CMD_LEN 257

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#include <signal.h>
extern sigset_t sigset_SIGCHLD;

#endif