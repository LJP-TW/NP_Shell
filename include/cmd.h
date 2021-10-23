#ifndef CMD_H
#define CMD_H

#define PIPE_ORDINARY   1
#define PIPE_NUM_STDOUT 2
#define PIPE_NUM_OUTERR 3
#define PIPE_FIL_STDOUT 4

typedef struct argv_node_tag argv_node;
struct argv_node_tag {
    argv_node *next;
    char *argv;
};

typedef struct cmd_node_tag cmd_node;
struct cmd_node_tag {
    // Next pipe command
    cmd_node *next;

    // Command
    char *cmd;

    // Arguments
    argv_node *argv;

    // Redirected output path
    char *rd_output;

    int cmd_len;
    int argv_len;

    // Pipe type
    // 0: None
    // 1: |   PIPE_ORDINARY   Pipe stdout
    // 2: |x  PIPE_NUM_STDOUT Pipe stdout
    // 3: !x  PIPE_NUM_OUTERR Pipe stdout and stderr
    // 4: > f PIPE_FIL_STDOUT Pipe stdout to file
    int pipetype;

    // Numbered Pipe
    int numbered; 
};

typedef struct numbered_pipe_node_tag np_node;
struct numbered_pipe_node_tag {
    np_node *next;
    int numbered;
    int fd[2];
};

extern void cmd_init();

// Read line to cmd_line buffer
// return the length of bytes received
extern int cmd_read(char *cmd_line);

extern cmd_node* cmd_parse(char *cmd_line);

extern int cmd_run(cmd_node *cmd);

#endif