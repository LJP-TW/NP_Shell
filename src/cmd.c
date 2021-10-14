#include <stdio.h>
#include <string.h>

#include "sys_variable.h"
#include "cmd.h"

int cmd_read(char *cmd_line)
{
    scanf("%" STR(MAX_CMDLINE_LEN) "s", cmd_line);

    return strlen(cmd_line);
}

cmd_list* cmd_parse(char *cmd_line)
{
    int firstcmd = 1;
    
    // TODO: Need a data struct to save parsed cmd

    while (1) {
        // TODO: Parse command
        
        // TODO: Check whether the first command is built-in command

        // TODO: If reach end of line, break
        break;
    }

    // TODO: Return the data struct pointer of parsed cmd
    return 0;
}

int cmd_run(cmd_list *cmd)
{
    return 0;
}
