#include <stdio.h>

#include "sys_variable.h"
#include "prompt.h"
#include "cmd.h"

void init(void);

int main(void)
{
    char cmd_line[MAX_CMDLINE_LEN] = { 0 };
    int cmd_line_len;
    cmd_list *cmd;

    // Initialization
    init();

    // Main shell loop
    while (1) {
        // Outputing Prompt
        prompt();

        // Reading command
        cmd_line_len = cmd_read(cmd_line);

        // Parsing command
        cmd = cmd_parse(cmd_line);

        // Executing command
        cmd_run(cmd);
    }
}

void init(void)
{
    // TODO: Initializing PATH
}

