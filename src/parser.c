//
// Created by Marcin Ph on 03.01.2016.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "helpers.h"

token_to_function logic_tokens[] = {
    { '&', _and },
    { '|', _or  },
    { ';', _any },
    { NULL, NULL}
};


int _any(int val, char ** cmd, int args_count) {
    parse_cmd(cmd, args_count, 1, 0, 1);
    return 1;
}

int _and(int val, char ** cmd, int args_count) {
    return val && parse_cmd(cmd, args_count, 1, 0, 1);
}

int _or(int val, char ** cmd, int args_count) {
    return val || parse_cmd(cmd, args_count, 1, 0, 1);
}

int check_token(char * cmd, int pos) {
    /*
     *  Funkcja sprawdza czy 2 kolejne znaki naleza do operatorów logicznych
     */
    if (cmd[pos+1]) {
        for(int i=0; logic_tokens[i].token != NULL; i++) {
            if (cmd[pos] == logic_tokens[i].token && cmd[pos + 1] == logic_tokens[i].token) {
                return 1;
            }
        }
    }
    return 0;
}

int token_idx(char t) {
    /*
     * Zwraca index operatora logicznego w tablicy
     */
    for(int i=0; logic_tokens[i].token != NULL; i++) {
        if (t == logic_tokens[i].token) {
            return i;
        }
    }
    fatal_error("UNKNOWN LOGIC OPERATOR");
    return EXIT_FAILURE;

}

char ** split_by_logical_operators(char * cmd, char * logical_operators, int * arg_count) {
    /*
     * funkcja dzieli komende wg operatorów logicznych
     * i zwraca tablice zawierajaca podkomendy
     */
    char **tokens = malloc(1000 * sizeof(char*));

    if (!tokens) {
        alloc_error();
    }

    int new_pos = 0;
    int iter = 0;
    for (int i=0; i < strlen(cmd) && cmd[new_pos]; i++) {
        if (check_token(cmd, i)) {
            char * t = malloc((i-new_pos + 1) * sizeof(char));
            if (!t) {
                alloc_error();
            } else {
                strncpy(t, &cmd[new_pos], i - new_pos);
                tokens[iter] = t;
                logical_operators[iter++] = cmd[i];
                new_pos = i+2;
            }
        }
    }
    char * t = malloc((strlen(cmd) - new_pos + 1) * sizeof(char));
    strcat(t, &cmd[new_pos]);
    //strncpy(t, &cmd[new_pos], strlen(cmd) - new_pos);
    tokens[iter] = t;
    *arg_count = iter;
    return tokens;
}

char ** split_cmd(char *cmd, int * arg_count) {
    /*
     * funkcja dzieli komende na tablice argumentow
     */
    int bufsize = TOKEN_SIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        alloc_error();
    }

    token = strtok(cmd, TOKENS);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOKEN_SIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                alloc_error();
            }
        }

        token = strtok(NULL, TOKENS);
    }
    tokens[position] = NULL;
    *arg_count = position;
    return tokens;
}