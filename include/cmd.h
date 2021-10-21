#ifndef CMD_H
#define CMD_H

#define PIPE_ORDINARY   1
#define PIPE_NUM_STDOUT 2
#define PIPE_NUM_OUTERR 3

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

    int argv_len;

    // Pipe type
    // 0: None
    // 1: |   PIPE_ORDINARY   Pipe stdout
    // 2: |x  PIPE_NUM_STDOUT Pipe stdout
    // 3: !x  PIPE_NUM_OUTERR Pipe stdout and stderr
    int pipetype;

    // Numbered Pipe
    int numbered; 
};

// Read line to cmd_line buffer
// return the length of bytes received
extern int cmd_read(char *cmd_line);

int cmd_parse_special_symbols(cmd_node *cmd, char **token, int ssidx);
void cmd_parse_bulitin_cmd(cmd_node *cmd, char *token, int bulitin_cmd_id);
extern cmd_node* cmd_parse(char *cmd_line);

extern int cmd_run(cmd_node *cmd);

#endif