#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "sys_variable.h"
#include "cmd.h"
#include "pidlist.h"

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

static np_node *global_nplist;
static int use_sh_wait;
static pid_list *plist;
sigset_t sigset_SIGCHLD;

static void signal_handler(int signum)
{
    int cpid;
    int ok = 0;

    switch (signum) {
    case SIGCHLD:
        if (!use_sh_wait)
            return;

        // When child process ends, call signal_handler and wait
        cpid = wait(NULL);

        // Save to update plist
        ok = plist_delete_by_pid(plist, cpid);

        if (!ok) {
            plist_insert(sh_closed_plist, cpid);
        }

        break;
    default:
        break;
    }
}

// Disable signal handler
static void disable_sh()
{
    sigset_t oldset;
    sigprocmask(SIG_BLOCK, &sigset_SIGCHLD, &oldset);

    use_sh_wait = 0;

    sigprocmask(SIG_SETMASK, &oldset, NULL);
}

// Enable signal handler
static void enable_sh()
{
    sigset_t oldset;
    sigprocmask(SIG_BLOCK, &sigset_SIGCHLD, &oldset);

    use_sh_wait = 1;

    sigprocmask(SIG_SETMASK, &oldset, NULL);
}

void cmd_init()
{
    // Register signal handler
    signal(SIGCHLD, signal_handler);

    // Init closed pid_list
    closed_plist    = plist_init();
    sh_closed_plist = plist_init();

    // Init sigset
    sigemptyset(&sigset_SIGCHLD);
    sigaddset(&sigset_SIGCHLD, SIGCHLD);
}

static cmd_node* cmd_node_init()
{
    cmd_node *node = malloc(sizeof(cmd_node));

    node->next = NULL;
    node->cmd  = NULL;
    node->argv = NULL;
    node->rd_output = NULL;
    node->cmd_len = 0;
    node->argv_len = 0;
    node->pipetype = 0;
    node->numbered = 0;

    return node;
}

static void cmd_node_release(cmd_node *cmd)
{
    argv_node *cur_an, *next_an;

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
}

int cmd_read(char *cmd_line)
{
    int len;

    if (!fgets(cmd_line, MAX_CMDLINE_LEN, stdin)) {
        exit(1);
    }

    len = strlen(cmd_line);

    if (cmd_line[len-1] == '\n') {
        cmd_line[len-1] = 0;
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
    int number;

    switch (ssidx)
    {
    case 0:
        // >
        // Stdout redirection (cmd > file)
        if ((token = strtok(NULL, " ")) != NULL) {
            cmd->rd_output = strdup(token);
            cmd->pipetype = PIPE_FIL_STDOUT;
            *token_ptr = token;
        } else {
            // TODO: Report error
        }
        break;
    
    case 1:
        // |
        if (token[1] == 0) {
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
    // Parse bulit-in command
    char *var;
    char *value;
    char *envvalue;

    switch (bulitin_cmd_id) {
    case 0:
        // setenv
        var = strtok(NULL, " ");
        value = strtok(NULL, " ");
        setenv(var, value, 1);
        break;
    case 1:
        // printenv
        var = strtok(NULL, " ");

        envvalue = getenv(var);
        if (envvalue)
            printf("%s\n", envvalue);
        break;
    case 2:
        // exit
        exit(0);
        break;
    }
}

cmd_node* cmd_parse(char *cmd_line)
{
    int firstcmd = 1;
    int bulitin_cmd_id = -1;
    char c;
    int cmd_len = 0;
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

        *curcmd = cmd_node_init();
        cmd = *curcmd;
        curcmd = &(cmd->next);

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
                return NULL;
            }
        }
        
        // Ok, save this command
        cmd->cmd = strdup(token);
        cmd_len += 1;

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

    cmd_head->cmd_len = cmd_len;

    return cmd_head;
}

static np_node* fdlist_find_by_numbered(int numbered)
{
    np_node **fd_ptr;
    np_node *fd_cur;
    
    // Find corresponding numbered pipe
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        fd_ptr = &(fd_cur->next);

        if (fd_cur->numbered == numbered) {
            break;
        }
    }

    return fd_cur;
}

static void fdlist_remove_by_numbered(int numbered)
{
    np_node **fd_ptr;
    np_node *fd_cur;
    
    // Find corresponding numbered pipe
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        if (fd_cur->numbered == numbered) {
            // Keep chain
            *fd_ptr = fd_cur->next;
            
            // Free it
            if (fd_cur->fd[0] != -1)
                close(fd_cur->fd[0]);
            if (fd_cur->fd[1] != -1)
                close(fd_cur->fd[1]);
            plist_release(fd_cur->plist);
            free(fd_cur);

            // End
            break;
        } 
        else {
            fd_ptr = &(fd_cur->next);
        }
    }
}

static void fdlist_close_all_writeend_except_numbered(int numbered)
{
    np_node **fd_ptr;
    np_node *fd_cur;
    
    // Find corresponding numbered pipe
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        fd_ptr = &(fd_cur->next);

        if (fd_cur->numbered != numbered) {
            close(fd_cur->fd[1]);
            fd_cur->fd[1] = -1;
        }
    }
}

static void fdlist_close_all_writeend()
{
    np_node **fd_ptr;
    np_node *fd_cur;
    
    // Find corresponding numbered pipe
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        fd_ptr = &(fd_cur->next);

        close(fd_cur->fd[1]);
        fd_cur->fd[1] = -1;
    }
}

static np_node* fdlist_insert(int numbered)
{
    np_node **fd_ptr;
    np_node *fd_cur;
    np_node *new_np = malloc(sizeof(np_node));
    new_np->next = NULL;
    new_np->plist = NULL;
    new_np->numbered = numbered;
    pipe(new_np->fd);

    // Insert
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        fd_ptr = &(fd_cur->next);
    }
    *fd_ptr = new_np;

    return new_np;
}

static void fdlist_update()
{
    np_node **fd_ptr;
    np_node *fd_cur;
    
    // Find corresponding numbered pipe
    fd_ptr = &global_nplist;
    while ((fd_cur = *fd_ptr)) {
        if (--fd_cur->numbered == -1) {
            // Keep chain
            *fd_ptr = fd_cur->next;

            // Free it
            if (fd_cur->fd[0] != -1)
                close(fd_cur->fd[0]);
            if (fd_cur->fd[1] != -1)
                close(fd_cur->fd[1]);
            plist_release(fd_cur->plist);
            free(fd_cur);
        } 
        else {
            fd_ptr = &(fd_cur->next);
        }
    }
}

int cmd_run(cmd_node *cmd)
{
    int idx;
    pid_t pid;
    int read_pipe = -1;
    np_node *np_out = NULL;
    cmd_node *next_cmd;
    char **argv;
    np_node *np_in, *origin_np_in;

    plist = plist_init();

    // Enable signal handler
    enable_sh();

    // Handle numbered pipe
    fdlist_update();
    origin_np_in = np_in = fdlist_find_by_numbered(0);

    while (cmd) {
        int cur_pipe[2] = {-1, -1};
        int filefd = -1;

        next_cmd = cmd->next;

        // Handle pipe
        switch(cmd->pipetype) {
        case PIPE_ORDINARY:
            if (pipe(cur_pipe)) {
                printf("[x] pipe error: %d\n", errno);
            }
            break;
        case PIPE_NUM_STDOUT:
        case PIPE_NUM_OUTERR:
            if (cmd->numbered) {
                np_out = fdlist_find_by_numbered(cmd->numbered);
                if (!np_out) {
                    np_out = fdlist_insert(cmd->numbered);
                }
            }
            break;
        case PIPE_FIL_STDOUT:
            filefd = open(cmd->rd_output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
            break;
        default:
            // No pipe
            break;
        }

        // Execute command
        if ((pid = fork()) > 0) {
            // Parent process

            // Handle pid list
            plist_insert_block(plist, pid);

            // Handle input pipe
            if (read_pipe != -1) {
                close(read_pipe); // read_pipe will be updated later
            }

            if (cur_pipe[1] != -1) {
                close(cur_pipe[1]);
                read_pipe = cur_pipe[0];
            } else {
                read_pipe = -1;
            }

            // Handle numbered pipe
            np_in = NULL;

            // Handle file pipe
            if (filefd != -1) {
                close(filefd);
            }

            // Free memory
            cmd_node_release(cmd);

            // Go to next command
            cmd = next_cmd;
        } else if (!pid) {
            // Child process
            // Handle another pipe
            if (np_out) {
                fdlist_close_all_writeend_except_numbered(cmd->numbered);
            } else {
                fdlist_close_all_writeend();
            }

            // Handle input pipe
            if (np_in) {
                dup2(np_in->fd[0], STDIN_FILENO);
            } else if (read_pipe != -1) {
                dup2(read_pipe, STDIN_FILENO);
            }
            
            // Handle output pipe
            switch(cmd->pipetype) {
            case PIPE_ORDINARY:
                close(cur_pipe[0]);
                dup2(cur_pipe[1], STDOUT_FILENO);
                break;
            case PIPE_NUM_STDOUT:
                close(np_out->fd[0]);
                dup2(np_out->fd[1], STDOUT_FILENO);
                break;
            case PIPE_NUM_OUTERR:
                close(np_out->fd[0]);
                dup2(np_out->fd[1], STDOUT_FILENO);
                dup2(np_out->fd[1], STDERR_FILENO);
                break;
            case PIPE_FIL_STDOUT:
                dup2(filefd, STDOUT_FILENO);
            default:
                // No pipe
                break;
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

            // Handle error
            fprintf(stderr, "Unknown command: [%s].\n", cmd->cmd);
            exit(errno);
        } else {
            // Handle error
            pid_t cpid;

            // Disable signal handler
            disable_sh();

            // Wait for one process and re-run again
            cpid = wait(NULL);

            // Record closed pid
            plist_insert(closed_plist, cpid);
            plist_delete_intersect(plist, closed_plist);

            // Recycle all the resources
            switch(cmd->pipetype) {
            case PIPE_ORDINARY:
                if (cur_pipe[0] != -1) {
                    close(cur_pipe[0]);
                    close(cur_pipe[1]);
                }
                break;
            case PIPE_NUM_STDOUT:
            case PIPE_NUM_OUTERR:
                if (cmd->numbered) {
                    fdlist_remove_by_numbered(cmd->numbered);
                }
                break;
            case PIPE_FIL_STDOUT:
                if (filefd != -1)
                    close(filefd);
                break;
            default:
                // No pipe
                break;
            }

            enable_sh();

            // Re-run
        }
    }

    // Disable wait in signal handler
    disable_sh();

    // close fd
    if (origin_np_in) {
        close(origin_np_in->fd[1]);
        origin_np_in->fd[1] = -1;
    }

    // Update pid list
    plist_merge(closed_plist, sh_closed_plist);

    if (!np_out) {
        int status;

        // Wait for origin_np_in
        if (origin_np_in) {
            plist_delete_intersect(origin_np_in->plist, closed_plist);
            for (pid_node *pn = origin_np_in->plist->next; pn; pn = pn->next) {
                waitpid(pn->pid, &status, 0);
            }
        }

        // Wait for plist
        plist_delete_intersect(plist, closed_plist);
        for (pid_node *pn = plist->next; pn; pn = pn->next) {
            waitpid(pn->pid, &status, 0);
        }

        // Free plist
        plist_release(plist);
    } else {
        // If there is origin_np_in, merge origin_np_in to plist
        if (origin_np_in) {
            plist_merge(plist, origin_np_in->plist);
        }

        // Update np_out->plist    
        if (!(np_out->plist)) {
            np_out->plist = plist;
        } else {
            plist_merge(np_out->plist, plist);
            plist_release(plist);
        }
    }

    enable_sh();

    return 0;
}
