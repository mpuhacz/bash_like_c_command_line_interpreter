#ifndef MAIN_H
#define MAIN_H

#define MAX_INPUT_LEN 1023
char CURRENT_DIR[MAX_INPUT_LEN];
int check_cmd(char **cmd, int args_count, int logical, int _stdin, int _stdout);

#endif
