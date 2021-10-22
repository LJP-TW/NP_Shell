#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "sys_variable.h"
#include "cmd.h"

#define ASCII_SPACE 0x20

#define ARR_LEN(x) (sizeof(x)/sizeof(x[0]))

#define IS_ALPHABET(x) ((65 <= x && x <= 90) || (97 <= x && x <= 122))
#define IS_DIGIT(x) (48 <= x && x <= 57)
#define IS_ALPHADIGIT(x) (IS_ALPHABET(x) || IS_DIGIT(x))

// declared in unistd.h
extern char** environ;

const char *bulitin_cmds[] = {"setenv", 
                              "printenv", 
                              "exit"};

const char *special_symbols[] = {">", 
                                 "|",
                                 "!"};

cmd_node_list *global_cmd_list;

int cmd_read(char *cmd_line)
{
    int len;

    if (!fgets(cmd_line, MAX_CMDLINE_LEN, stdin)) {
        exit(1);
    }

    len = strlen(cmd_line);

    if (cmd_line[len-1] == '\n') {
        cmd_line[len-1] = NULL;
        len -= 1;
    }

    return len;
}

static int valid_filepath(char *filepath)
{
    int ok = 1;

    for (char c = *filepath++; c; c = *filepath++) {
        if (IS_ALPHADIGIT(c)) {
            continue;
        }

        if (c == '.' || c == '/' || c == '_') {
            continue;
        }

        ok = 0;
        return ok;
    }

    return ok;
}

static int cmd_parse_special_symbols(cmd_node *cmd, char **token_ptr, int ssidx)
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
        if (token[1] == NULL) {
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

static void cmd_parse_bulitin_cmd(cmd_node *cmd, char *token, int bulitin_cmd_id)
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

        // Check command is a valid path
        if (!valid_filepath(token)) {
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
    int idx;
    pid_t pid;
    int old_stdout_pipe = -1;
    int old_stderr_pipe = -1;
    cmd_node *next_cmd;
    char **argv;

    while (cmd) {
        int cur_stdout_pipe[2] = {-1, -1};
        int cur_stderr_pipe[2] = {-1, -1};

        next_cmd = cmd->next;

        // TODO: Handle pipe
        switch(cmd->pipetype) {
        case PIPE_ORDINARY:
            pipe(cur_stdout_pipe);
            break;
        case PIPE_NUM_STDOUT:
            break;
        case PIPE_NUM_OUTERR:
            break;
        default:
            // No pipe
            break;
        }

        // TODO: Update global_cmd_list for numbered pipe

        // Execute command
        if ((pid = fork()) > 0) {
            // Parent process
            argv_node *cur_an, *next_an;

            // Handle pipe
            // TODO: Do we need to close old pipe?
            if (cur_stdout_pipe[1] != -1) {
                close(cur_stdout_pipe[1]);
                old_stdout_pipe = cur_stdout_pipe[0];
            } else {
                old_stdout_pipe = -1;
            }

            if (cur_stderr_pipe[1] != -1) {
                close(cur_stderr_pipe[1]);
                old_stderr_pipe = cur_stderr_pipe[0];
            } else {
                old_stderr_pipe = -1;
            }

            // Free memory
            if (cmd->cmd)
                free(cmd->cmd);
            for (cur_an = cmd->argv; cur_an; cur_an = next_an) {
                next_an = cur_an->next;
                free(cur_an->argv);
                free(cur_an);
            }
            if (cmd->rd_output)
                free(cmd->rd_output);
            free(cmd);

            // Go to next command
            cmd = next_cmd;
        } else if (!pid) {
            // Child process
            // Handle pipe
            if (old_stdout_pipe != -1) {
                dup2(old_stdout_pipe, STDIN_FILENO); // Not sure about that
            }

            if (old_stderr_pipe != -1) {
                dup2(old_stderr_pipe, STDIN_FILENO); // Not sure about that
            }
            
            if (cur_stdout_pipe[1] != -1) {
                close(cur_stdout_pipe[0]);
                dup2(cur_stdout_pipe[1], STDOUT_FILENO);
            }

            if (cur_stderr_pipe[1] != -1) {
                close(cur_stderr_pipe[0]);
                dup2(cur_stderr_pipe[1], STDERR_FILENO);
            }

            // Make argv
            argv = malloc(sizeof(char *) * (cmd->argv_len + 2));
            argv[0] = cmd->cmd;
            idx = 1;
            for (argv_node *an = cmd->argv; an; an = an->next) {
                argv[idx++] = an->argv;
            }
            argv[idx] = NULL;

            // Execute command
            execvp(cmd->cmd, argv);

            // TODO: Handle error
            printf("GG %d\n", errno);
            exit(errno);
        } else {
            // TODO: Report error
        }

        wait(NULL);
    }

    return 0;
}
