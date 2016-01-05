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
#include <readline/readline.h>

volatile  int RUNNING = 0;

char * read_cmd() {
    /*
     * funkcja odczytuje komende
     */
//    int cmd_size = MAX_INPUT_LEN;
//    int position = 0;
//    char *cmd = malloc(sizeof(char) * cmd_size);
//    int c;
//
//    if (!cmd) {
//        alloc_error();
//    }
//
//    while (1) {
//        c = getchar();
//        if (c == EOF || c == '\n') {
//            cmd[position] = '\0';
//            return cmd;
//        } else {
//            cmd[position] = c;
//        }
//        position++;
//        if (position >= cmd_size) {
//            cmd_size += MAX_INPUT_LEN;
//            cmd = realloc(cmd, cmd_size);
//            if (!cmd) {
//                alloc_error();
//            }
//        }
//    }

        static char *line_read = NULL;
        if (line_read) {
            free (line_read);
            line_read = (char *) NULL;
        }
        char prompt[100];
        sprintf(prompt, "\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
        line_read = readline (&prompt);

        if (line_read && *line_read) {
            add_history (line_read);
        }

        return (line_read);


}

int has_pipe(char ** cmd, int args_count) {
    /*
     * funkcja zwraca index operatora potoku
     * jesli go nie znajdzie, zwraca 0
     */
    for(int i=0; i<args_count; i++) {
        if (strcmp(cmd[i], "|") == 0) {
            return i;
        }
    }
    return 0;
}


int parse_cmd(char ** cmd, int args_count, int logical, int _stdin, int _stdout) {
    /*
     * funkcja sprawdza przekazana komendę pod względem operatorów: "&", "1>", "2>", "|"
     * przekazuje ją do wykonania i zwraca status wyjscia otrzymany od funkcji
     * wywołującej komendę
     */
    if(cmd[0]) {
        for (int i=0; i < availible_cmds(); i++) {
            if (strcmp(commands_name[i], cmd[0]) == 0) {
                return (*commands_func[i])(cmd);
            }
        }
        int run_background = 0;

        // sprawdzamy czy mamy do czynienia z potokiem
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
    /*
     * Funkcja tworzy potomka który wykonuje przekazana komende.
     *
     * Parametry:
     *      is_child - informuje funkcję czy została ona wywołana przez potomka procesu głównego
     */
    pid_t pid;
    int status;

    if (run_background && logical) {
        fprintf(stderr, "Polecenia warunkowe nie mogą występować dla poleceń uruchamianych w tle!\n");
        return 0;
    }


    pid = fork();

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
                        if(args_count - pipe_idx - 1 > 0) {
                            close(_pipe[1]);
                            return exec_cmd(&args[pipe_idx + 1], args_count - pipe_idx - 1, run_background, logical, _pipe[0], _stdout, 1);
                        } else {
                            exit(EXIT_SUCCESS);
                        }
                    }
                    default: {

                        char ** new_args = malloc((pipe_idx) * sizeof(args[0]));
                        memcpy(new_args, args, pipe_idx * sizeof(args[0]));
                        new_args[pipe_idx] = NULL;
                        args = new_args;
                        close(_pipe[0]);
                        _stdout = _pipe[1];
                    }
                }
            };

            dup2(_stdin, 0);
            dup2(_stdout, 1);
            int result = execvp(args[0], args);
            if (run_background) setpgid(0, 0);
            if (result == -1) {
                fprintf(stderr, "SOPshell: komenda nie znaleziona: %s\n", args[0]);
                result = EXIT_FAILURE;
            }
            // ładnie zamykamy co otwieraliśmy
            close(_stdin);
            close(_stdout);

            exit(result);
        }

        // owner
        default: {
            if(run_background) {
                fprintf(stdout, "%s pid: %d\n", args[0], pid);
            } else {
                do {
                    RUNNING = 1;
                    waitpid(pid, &status, WUNTRACED);
                    if(!RUNNING) {
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
    //fprintf(stdout, "\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
}


int prepare_cmds(char **cmd, char *ops, int ops_count) {
    /*
     * Funkcja ma za przekazać pierwszą komendę z **cmd do wykonania i zapisać jej wynik
     * do zmiennej result.
     * Jeśli wywoływana funkcja zawiera operatory logiczne, to wykonujemy kolejne komendy
     * w zależności od wartości result i kolejnego operatora logicznego.
     */
    int args_count;
    int iter = 0;
    char ** args;
    int logical = ops_count > 0;
    args = split_cmd(cmd[iter++], &args_count);
    int result = parse_cmd(args, args_count, logical, stdin, stdout);
    free(args);
    if (logical) {
        do {
            args = split_cmd(cmd[iter], &args_count);
            result = (*logic_tokens[token_idx(ops[iter - 1])].func)(result, args, args_count);
            iter++;
            free(args);
        } while(iter <= ops_count);
    }
}


void kill_task() {
    /*
     * przechwytuje ctrl+c (SIGINT) i wysyła go do aktualnie działającego procesu
     */
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
        while (1) {
            draw_prompt();
            cmd = read_cmd();
            if (strlen(cmd) > 0) {
                logical_op = malloc(100*sizeof(char));
                cmds = split_by_logical_operators(cmd, logical_op, &logical_op_count);
                prepare_cmds(cmds, logical_op, logical_op_count);
                free(cmds);
                free(logical_op);
            }
            //free(cmd);
            fflush(stdin);

        }
    }

    return 0;
}
