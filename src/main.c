#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "helpers.h"
#include "parser.h"

#define MAX_INPUT_LEN 1023
volatile  int RUNNING = 1;
char CURRENT_DIR[MAX_INPUT_LEN];

int cd(char ** args) {
	if (args[1] == NULL) {
		fprintf(stderr, "podaj lokalizacje!\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("SOPshell");
		}
	}
	getcwd(CURRENT_DIR, MAX_INPUT_LEN);
	return 1;
}

int (*commands_func[]) (char **) = {
	&cd,
	&exit
};

char *commands_name[] = {
	"cd",
	"exit"
};

int availible_cmds() {
	return sizeof(commands_name) / sizeof(commands_name[0]);
}


char * read_cmd() {
	int cmd_size = MAX_INPUT_LEN;
	int position = 0;
	char *cmd = malloc(sizeof(char) * cmd_size);
	int c;

	if (!cmd) {
		alloc_error();
	}

	while (1) {
		c = getchar();
		if (c == EOF || c == '\n') {
			cmd[position] = '\0';
			return cmd;
		} else {
			cmd[position] = c;
			}
		position++;
		if (position >= cmd_size) {
			cmd_size += MAX_INPUT_LEN;
			cmd = realloc(cmd, cmd_size);
			if (!cmd) {
				alloc_error();
			}
		}
	}
}

int parse_cmd(char ** cmd, int  args_count, int logical) {
	if(cmd[0]) {
		for (int i=0; i<availible_cmds(); i++) {
			if (strcmp(commands_name[i], cmd[0]) == 0) {
				return (*commands_func[i])(cmd);
			}
		}
		int run_background = 0;
		if(strcmp(cmd[args_count], "&") == 0) {
			cmd[args_count] = NULL;
			run_background = 1;
		}
		return exec_cmd(cmd, run_background, logical);
	}
	return 0;
}


int exec_cmd(char **args, int run_background, int logical) {
	pid_t pid;
	int status;

    if (run_background && logical) {
        fprintf(stderr, "Polecenia warunkowe nie mogą występować dla poleceń uruchamianych w tle!\n");
        return 0;
    }
	pid = fork();
	if (pid == 0) {
        int result = execvp(args[0], args);
		if (result == -1) {
            fprintf(stderr, "SOPshell: komenda nie znaleziona:  %s\n", args[0]);
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	} else if (pid < 0) {
		perror("SOPshell");
        status = EXIT_FAILURE;
	} else if(!run_background) {
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

//	printf("Wartosc: %d\n", status);
	return status == 0;
}

void draw_prompt() {
	printf("\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
}


int run_cmd(char ** cmd, char * ops, int ops_count) {
    int args_count;
    int iter = 0;
    char ** args;
    args = split_cmd(cmd[iter++], &args_count);
    int res = parse_cmd(args, args_count, 1);
    free(args);
    if (ops_count > 0) {
        do {
            args = split_cmd(cmd[iter], &args_count);
            res = (*logic_tokens[token_idx(ops[iter- 1])].func)(res, args, args_count);
            free(args);
            iter++;
        } while(iter <= ops_count);
    }
}


int main(int argc, char ** argv) {
	getcwd(CURRENT_DIR, MAX_INPUT_LEN);
	signal(SIGINT, SIG_IGN);
	char * cmd, **cmds;
	char * logical_op;
	int logical_op_count;
	if (argc > 1) {
		for(int i=1; i< argc; i++) {
			if (argv[i][0] == '-') {
				switch(argv[i][1]) {
					case 'v':
						printf("Wersja 0.0.1 - Marcin Puhacz\n");	
					break;
					case 'h':
						printf("Tu będzie pomoc\n");
					break;
				}
			}
		}
	} else {
		while(RUNNING) {
			draw_prompt();
			cmd = read_cmd();
			if (strlen(cmd) > 0) {
                logical_op = malloc(100*sizeof(char));
				cmds = split_by_logical_operators(cmd, logical_op, &logical_op_count);
                run_cmd(cmds, logical_op, logical_op_count);
                free(cmds);
                free(logical_op);
			}
            free(cmd);
			fflush(stdin);
		}
	}

	return 0;
}
