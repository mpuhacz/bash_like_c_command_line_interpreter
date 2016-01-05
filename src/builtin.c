//
// Created by Marcin Ph on 03.01.2016.
//

#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "builtin.h"

char *commands_name[] = {
    "cd",
    "exit"
};

int (*commands_func[]) (char **) = {
    &cd,
    &exit
};

int cd(char ** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "podaj lokalizacje!\n");
        return 0;
    } else {
        if (chdir(args[1]) != 0) {
            perror("SOPshell");
        }
    }
    getcwd(CURRENT_DIR, MAX_INPUT_LEN);
    return 1;
}

int availible_cmds() {
    return sizeof(commands_name) / sizeof(commands_name[0]);
}