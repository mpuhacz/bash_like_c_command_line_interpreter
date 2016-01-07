#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "helpers.h"
#include "parser.h"
#include "builtin.h"




volatile int RUNNING = 1;
char prompt[100];

typedef struct proc_obj proc_obj;
struct proc_obj {
    int pid;
    char * name;
};

proc_obj * bgproc[1024];

int bg_proc_count = 0;


void add_bg_process(char * name, int pid) {
    proc_obj * p = malloc(sizeof(proc_obj));
    p->pid = pid;
    char * n = malloc(sizeof(char) * strlen(name));
    strncpy(n, name, strlen(name));
    n[strlen(name)] = (char) NULL;
    p->name = n;
    bgproc[bg_proc_count++] = p;
    if (bg_proc_count==1023) bg_proc_count = 0;
}

char * read_cmd() {
    /*
     * funkcja odczytuje komende
     */
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



int has_char(char ** cmd, int args_count, char * c){
    /*
    * funkcja zwraca index znaku c
    * jesli go nie znajdzie, zwraca 0
    */
    for(int i=0; i<args_count; i++) {
        if (strcmp(cmd[i], &c) == 0) {
            return i;
        }
    }
    return 0;
}

int has_pipe(char ** cmd, int args_count) {
    /*
     * funkcja zwraca index operatora potoku
     * jesli go nie znajdzie, zwraca 0
     */
    return has_char(cmd, args_count, '|');
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
                if (strcmp(cmd[i], ">") == 0) {
                    if (!cmd[i + 1]) {
                        fprintf(stderr, "Brakujący argument dla przekierowania\n");
                        return 0;
                    }
                }
                // stderr
                if (strcmp(cmd[i], "<") == 0) {
                    if (!cmd[i + 1]) {
                        fprintf(stderr, "Brakujący argument dla przekierowania\n");
                        return 0;
                    }
                }
                // last arg
                if (i == args_count-1 && strcmp(cmd[i], "&") == 0) {
                    cmd[args_count-1] = NULL;
                    args_count--;
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
            perror("SOPshell fork");
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
                    perror("SOPshell pipe");
                    exit(EXIT_FAILURE);
                }

                child_pid = fork();
                switch (child_pid) {
                    case -1: {
                        perror("SOPshell fork");
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

            int redirect_stdout = has_char(args, args_count, '>');
            if (redirect_stdout && args[redirect_stdout+1]) {
                int fd = open(args[redirect_stdout+1], O_RDWR | O_CREAT, S_IRWXU | S_IRWXG);
                if(fd == -1) {
                    perror("SOPshell open");
                } else {
                    args[redirect_stdout] = NULL;
                    args_count = redirect_stdout;
                    dup2(fd, _stdout);
                    close(fd);
                }
            }

            int redirect_stdin = has_char(args, args_count, '<');
            if (redirect_stdin && args[redirect_stdin+1]) {
                int fd = open(args[redirect_stdin+1], O_RDONLY);
                if(fd == -1) {
                    perror("SOPshell open");
                }
                args[redirect_stdin] = NULL;
                dup2(fd, _stdin);
                close(fd);
            }


            int result = execvp(args[0], args);
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
                add_bg_process(args[0], pid);
                fprintf(stdout, "[%d] %s pid: %d\n", bg_proc_count, args[0], pid);
            } else {
                do {
                    RUNNING = 1;
                    waitpid(pid, &status, WUNTRACED);
                    if(!RUNNING) {
                        //kill(pid, SIGINT);
                        printf("\n");
                    }
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                //printf("pid: %d, status:%d, WIFEXITED: %d, WEXITSTATUS: %d, WIFSIGNALED: %d, WTERMSIG: %d\n", pid, status, WIFEXITED(status), WEXITSTATUS(status), WIFSIGNALED(status), WTERMSIG(status));
            }
            if (is_child) {
                exit(EXIT_SUCCESS);
            }

        }
    }
    return status == EXIT_SUCCESS;

}

void print_prompt() {
    // sprintf(prompt, "\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
    fprintf(stdout, "\x1b[31m[%s]\n ❯ \x1b[0m", CURRENT_DIR);
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


void sigint_handler() {
    RUNNING = 0;
}

void sigchld_handler(int signal) {
    int status;
    pid_t pid;
    while (1)
    {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0)
            return;

        for (int i = 0; i < 300; i++) {
            if (bgproc[i] && bgproc[i]->pid == pid) {
                char *str_status;
                if (status > 1) str_status = strsignal(status);
                else if (status == 1) str_status = "exit 1";
                else str_status = "done";
                fprintf(stdout, "\npid: %d, nazwa: %s, status: %s\n", pid, bgproc[i]->name, str_status);
                free(bgproc[i]);
                return;
            }
        }
    }

}

int main(int argc, char ** argv) {


    getcwd(CURRENT_DIR, MAX_INPUT_LEN);


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

            signal(SIGINT, sigint_handler);
            signal(SIGPIPE, SIG_IGN);
            signal(SIGCHLD, sigchld_handler);

            print_prompt();
            cmd = read_cmd();
            if (strlen(cmd) > 0) {
                logical_op = malloc(100*sizeof(char));
                cmds = split_by_logical_operators(cmd, logical_op, &logical_op_count);
                prepare_cmds(cmds, logical_op, logical_op_count);
                free(cmds);
                free(logical_op);
            }
            free(cmd);
            fflush(stdin);

        }
    }

    return 0;
}
