//
// Created by Marcin Ph on 03.01.2016.
//

#ifndef BUILTIN_H
#define BUILTIN_H

int cd(char ** args);
int help(char ** args);
int (*commands_func[]) (char **);
int availible_cmds();
char *commands_name[];
#endif
