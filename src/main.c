#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "helpers.h"
#include "parser.h"
#include "builtin.h"

volatile  int RUNNING = 1;

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

int has_pipe(char ** cmd, int args_count){
    for(int i=0;i<args_count; i++) {
        if (strcmp(cmd[i], "|") == 0) {
            return i;
        }
    }
    return 0;
}


int parse_cmd(char ** cmd, int args_count, int logical, int _stdin, int _stdout) {
	if(cmd[0]) {
		for (int i=0; i < availible_cmds(); i++) {
			if (strcmp(commands_name[i], cmd[0]) == 0) {
				return (*commands_func[i])(cmd);
			}
		}
		int run_background = 0;

        // sprawdzamy czy polecenie chce przekazac stdout
        if (!has_pipe(cmd, args_count)) {
            for (int i = 0; i < args_count; i++) {

                // stdout
                if (strcmp(cmd[i], "1>") == 0) {
                    if (!cmd[i + 1]) {
                        fprintf(stderr, "Brakujący argument dla przekierowania STDOUT\n");
                        return 0;
                    }
                }
                // stderr
                if (strcmp(cmd[i], "2>") == 0) {
                    if (!cmd[i + 1]) {
                        fprintf(stderr, "Brakujący argument dla przekierowania STDERR\n");
                        return 0;
                    }
                }
                // last arg
                if (i == args_count-1 && strcmp(cmd[i], "&") == 0) {
                    cmd[args_count-1] = NULL;
                    run_background = 1;
                }
            }
        }
		int result = exec_cmd(cmd, args_count, run_background, logical, 0, 1, 0);
        return result;
	}
	return 0;
}

int exec_cmd(char **args, int args_count, int run_background, int logical, int _stdin, int _stdout, int is_child) {
	pid_t pid;
	int status;

    if (run_background && logical) {
        fprintf(stderr, "Polecenia warunkowe nie mogą występować dla poleceń uruchamianych w tle!\n");
        return 0;
    }


	pid = fork();

//    int pipe_idx = has_pipe(args, args_count);
//    if(pipe_idx){
//    char ** new_args = malloc((pipe_idx+1) * sizeof(args[0]));
//    memcpy(new_args, args, pipe_idx * sizeof(args[0]));
//    new_args[pipe_idx+1] = NULL;
//        }
    switch (pid) {

        // error
        case -1: {
            perror("SOPshell");
            status = EXIT_FAILURE;
            break;
        }

        // child
        case 0: {
            int pipe_idx = has_pipe(args, args_count);
            pid_t child_pid = -1;

            if(pipe_idx) {
                int _pipe[2];
                if (pipe(_pipe) == -1) {
                    perror("tworzenie strumienia nienazwanego");
                    exit(EXIT_FAILURE);
                }

                child_pid = fork();
                switch (child_pid) {
                    case -1: {
                        perror("SOPshell");
                        status = EXIT_FAILURE;
                        break;
                    }
                    case 0: {
                        if(args_count - pipe_idx - 1 > 0){
                            close(_pipe[1]);
                            return exec_cmd(&args[pipe_idx + 1], args_count - pipe_idx - 1, run_background, logical, _pipe[0], _stdout, 1);
                        } else {
                            exit(EXIT_SUCCESS);
                        }
                    }
                    default: {
                        close(_pipe[0]);
                        char ** new_args = malloc((pipe_idx+1) * sizeof(args[0]));
                        memcpy(new_args, args, pipe_idx * sizeof(args[0]));
                        new_args[pipe_idx+1] = NULL;
                        args = new_args;
                        _stdout = _pipe[1];
                    }
                }
            };

            dup2(_stdin, 0);
            dup2(_stdout, 1);
            int stat;
            int result = execvp(args[0], args);
            if (result == -1) {
                perror("\nSOP SHELL");
                fprintf(stderr, "SOPshell: komenda nie znaleziona: %s\n", args[0]);
                exit(EXIT_FAILURE);
                close(1);
            }
            close(1);
            exit(EXIT_SUCCESS);
        }

        // owner
        default: {
            if(run_background) {
                fprintf(stdout, "%s pid: %d\n", args[0], pid);
            } else {
                do {
                    RUNNING = 1;
                    waitpid(pid, &status, WUNTRACED);
                    if(!RUNNING){
                        kill(pid, SIGINT);
                        printf("\n");
                    }
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                if (is_child) {
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }

    return status == EXIT_SUCCESS;

}

void draw_prompt() {
	fprintf(stdout, "\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
}


int run_cmd(char ** cmd, char * ops, int ops_count) {
    int args_count;
    int iter = 0;
    char ** args;
    int logical = 0;
    if (ops_count > 0) logical = 1;
    args = split_cmd(cmd[iter++], &args_count);
    int res = parse_cmd(args, args_count, logical, stdin, stdout);
    if (logical) {
        do {
            args = split_cmd(cmd[iter], &args_count);
            res = (*logic_tokens[token_idx(ops[iter- 1])].func)(res, args, args_count);
            iter++;
        } while(iter <= ops_count);
    }
}


void kill_task(){
    RUNNING = 0;
}

int main(int argc, char ** argv) {
	getcwd(CURRENT_DIR, MAX_INPUT_LEN);
	signal(SIGINT, kill_task);
    signal(SIGPIPE, SIG_IGN);
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
		while(1) {
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
