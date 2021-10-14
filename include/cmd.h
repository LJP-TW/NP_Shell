#ifndef CMD_H
#define CMD_H

typedef struct cmd_list_tag cmd_list;
struct cmd_list_tag {
    // Next pipe command
    struct cmd_list *next;

    // Command
    char *cmd;

    // Arguments
    char *argv;

    // Redirected output path
    char *rd_output;

    // Pipe type
    // 0: None
    // 1: | Pipe stdout
    // 2: ! Pipe stdout and stderr
    int pipetype;

    // Numbered Pipe
    int numbered; 
};

// Read line to cmd_line buffer
// return the length of bytes received
extern int cmd_read(char *cmd_line);

extern cmd_list* cmd_parse(char *cmd_line);

extern int cmd_run(cmd_list *cmd);

#endif