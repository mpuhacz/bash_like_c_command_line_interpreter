//
// Created by Marcin Ph on 03.01.2016.
//

#ifndef PARSER_H
#define PARSER_H

#define TOKEN_SIZE 64
#define TOKENS " \t\r\n\a"


typedef int (*function)(int val, char ** cmd, int args_count);

typedef struct {
    char token;
    function func;
} token_to_function;

token_to_function logic_tokens[];

int _any(int val, char ** cmd, int args_count);
int _and(int val, char ** cmd, int args_count);
int _or(int val, char ** cmd, int args_count);


int token_idx(char t);

char ** split_by_logical_operators(char * line, char * operators,  int * arg_count);

char ** split_cmd(char *line, int * arg_count);

#endif