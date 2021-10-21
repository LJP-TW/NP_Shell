#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sys_variable.h"
#include "cmd.h"

#define ASCII_SPACE 0x20

#define ARR_LEN(x) (sizeof(x)/sizeof(x[0]))

#define IS_ALPHABET(x) ((65 <= x && x <= 90) || (97 <= x && x <= 122))
#define IS_DIGIT(x) (48 <= x && x <= 57)
#define IS_ALPHADIGIT(x) (IS_ALPHABET(x) || IS_DIGIT(x))

const char *bulitin_cmds[] = {"setenv", 
                              "printenv", 
                              "exit"};

const char *special_symbols[] = {">", 
                                 "|",
                                 "!"};

int cmd_read(char *cmd_line)
{
    int len;

    fgets(cmd_line, MAX_CMDLINE_LEN, stdin);

    len = strlen(cmd_line);

    if (cmd_line[len-1] == '\n') {
        cmd_line[len-1] = NULL;
        len -= 1;
    }

    return len;
}

int cmd_parse_special_symbols(cmd_node *cmd, char **token_ptr, int ssidx)
{
    // Parse special symbols
    char *token = *token_ptr;
    char c = token[0];
    int number;

    switch (ssidx)
    {
    case 0:
        // >
        // Stdout redirection (cmd > file)
        if ((token = strtok(NULL, " ")) != NULL) {
            cmd->rd_output = strdup(token);
            *token_ptr = token;
        } else {
            // TODO: Report error
        }
        break;
    
    case 1:
        // |
        if (token[1] == ' ') {
            // Ordinary pipe
            // cmd1 | cmd2
            cmd->pipetype = PIPE_ORDINARY;
        } else {
            // Numbered pipe
            // cmd1 |2
            number = atoi(&token[1]);
            if (number == 0) {
                // TODO: Report error
            } else {
                cmd->pipetype = PIPE_NUM_STDOUT;
                cmd->numbered = number;
            }
        }
        break;

    case 2:
        // !
        // Numbered pipe
        // cmd !2
        number = atoi(&token[1]);
        if (number == 0) {
            // TODO: Report error
        } else {
            cmd->pipetype = PIPE_NUM_OUTERR;
            cmd->numbered = number;
        }
        break;
    
    default:
        break;
    }

    return 0;
}

void cmd_parse_bulitin_cmd(cmd_node *cmd, char *token, int bulitin_cmd_id)
{
    // TODO: Parse bulit-in command
}

cmd_node* cmd_parse(char *cmd_line)
{
    int firstcmd = 1;
    int bulitin_cmd_id = -1;
    char c;
    char *strtok_arg1 = cmd_line;
    char *token;
    cmd_node *cmd_head;
    cmd_node *cmd;
    cmd_node **curcmd = &cmd_head;
    argv_node *argv;

    // Parse command
    while ((token = strtok(strtok_arg1, " ")) != NULL) {
        argv_node **ptr;
        int ssidx; // Special symbol idx

        *curcmd = malloc(sizeof(cmd_node));
        cmd = *curcmd;
        curcmd = &(cmd->next);

        cmd->next = NULL;
        cmd->cmd  = NULL;
        cmd->argv = NULL;
        cmd->rd_output = NULL;
        cmd->argv_len = 0;
        cmd->pipetype = 0;
        cmd->numbered = 0;

        // Check command isn't special symbol
        c = token[0];
        if (!IS_ALPHADIGIT(c)) {
            // TODO: Report error
            break;
        }

        // Check whether the command is built-in command
        if (firstcmd) {
            firstcmd = 0;
            strtok_arg1 = NULL;

            for (int i = 0; i < ARR_LEN(bulitin_cmds); ++i) {
                if (!strcmp(bulitin_cmds[i], token)) {
                    bulitin_cmd_id = i;
                    break;
                }
            }

            if (bulitin_cmd_id != -1) {
                cmd_parse_bulitin_cmd(cmd, token, bulitin_cmd_id);
                break;
            }
        }
        
        // Ok, save this command
        cmd->cmd = strdup(token);

        // Parse argv
        ptr = &(cmd->argv); 
        ssidx = -1;
        while ((token = strtok(NULL, " ")) != NULL) {
            c = token[0];

            for (int i = 0; i < ARR_LEN(special_symbols); ++i) {
                if (special_symbols[i][0] == c) {
                    // End of argv
                    ssidx = i;
                    break;
                }
            }

            if (ssidx != -1) {
                // End of argv
                break;
            }

            // Ok, save this argv
            argv = malloc(sizeof(argv_node));
            argv->next = NULL;
            argv->argv = strdup(token);

            *ptr = argv;
            ptr = &(argv->next);

            cmd->argv_len += 1;
        }

        // Parse special symbol
        if (token == NULL) {
            break;
        }

        cmd_parse_special_symbols(cmd, &token, ssidx);
    }

    return cmd_head;
}

int cmd_run(cmd_node *cmd)
{

    return 0;
}
