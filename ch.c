/*
 *  Ce TP est présenté par Wael ABOU ALI.
 *  Numéro de matricule : 20034365.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

// =======
// Macros|
// =======

#define BUFFER_SIZE 64
#define INPUT_BUFFER 128    /* si on a besoin de plus on va utiliser realloc */
#define SHELL_TOKEN_DELIMITER   " \t\v\f\r\n\a" /* les delimiteurs des espaces */

// =======================================================
// La structure initiale du programme (l'unité du Buffer)|
// =======================================================

typedef struct instruction {

    char **args, *command;
    int args_count, cmnd_rd, cmnd_wr, bin_oper, if_statement, do_statement,
            done_statement;

} Instruction;

// ================================================
// Les fonctions initiales qui gèrent le programme|
// ================================================

void shell_loop();
void lire_ligne();
void parser_ligne();
void executer_instructions();

// =====================================
// Les fonctions responsables à l'écran|
// =====================================

void my_logo();
void clear_shell();
void get_shell();
void get_data();
void user_prompt();
void about();

// ===========================================
// Gestion d'esoace et syntaxe dans le parser|
// ===========================================

void alloc_mem(int);
void treat_start(int, const char *);
void realloc_mem(int, int);
void check_if(const char *);
int check_opers(int);
void check_redirection();
int treat_end(int);
void rdrw_action();
void check_background(char *, int, char *);

// =======================================
// Gestions des instructions et commandes|
// =======================================

void command_options();
void shell_cd(char *, Instruction);
void shell_pwd(char *, int, Instruction) ;
void shell_action_dir(char *, int, Instruction);
void shell_commands(Instruction);
void if_stat();
void read_write(Instruction);

// ================================================
// Gestion de sortie prématuré et reinitialisation|
// ================================================

void reset_data();
void error_check(Instruction, char *);
int prem_exit();

// ===================================================
// Les variables globaux à utiliser dans le programme|
// ===================================================

char *username, cwd[PATH_MAX], *line, in_file[INPUT_BUFFER],
        out_file[INPUT_BUFFER], *token;
//char hostname[1024];
int exit_flag = 0, first = 1, count = 0, instructions_pos = 0, and_result = 0,
        error_signal = 0, file_action = 0, is_background = 0, wakeup = 0;
long timer_int = 0;

Instruction *instruction_list;

// ==========================
// La fonction initiale main|
// ==========================

int main() {

    get_shell();
    fprintf(stdout, "%% ");
    get_data();
    while (exit_flag == 0) shell_loop();
    fprintf(stdout, "bye!");
    return EXIT_SUCCESS;

}

// ================================
// La boucle initiale du programme|
// ================================

void shell_loop() {

    user_prompt();
    lire_ligne();
    parser_ligne();
    executer_instructions();

}

// ****************************************************************************

/* Les fonctions principales sont les suivantes : lire, parser et
 * executer_ligne. Ces fonctions sont responsables à lire les
 * instructions, les traduire selon la syntaxe et exécuter les
 * commandes comme il faut.
*/

void lire_ligne() {

    fflush(stdout); // clear all previous buffers if any
    line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    if(strcmp(line, "\n") == 0) {
        raise(wakeup);
    }

}

void parser_ligne() {

    int bufsize = BUFFER_SIZE, j = 0;
    char *prev_tok, *bef_tok;
    const char *semi = ";";

    alloc_mem(bufsize);
    token = strtok(line, SHELL_TOKEN_DELIMITER);
    prev_tok = token;

    while (token != NULL) {

        if(j == 0) treat_start(bufsize, semi);

        if ((instructions_pos >= bufsize) || (j >= bufsize))
            realloc_mem(bufsize, j);

        else {

            // si ce n'est pas un if ou do et c'est la fin de l'instruction

            if((strcmp(token, "|") == 0) || (strcmp(token, "||") == 0) ||
               (strcmp(token, "&&") == 0) ||
               (strcmp(&token[strlen(token) - 1], ";") == 0)) {

                j = treat_end(j);

            } else {

                if ((strcmp(token, "<") == 0) || (strcmp(token, ">") == 0))
                    check_redirection();

                else instruction_list[instructions_pos].args[j] = token;

                j++;
            }

        }

        bef_tok = prev_tok;
        prev_tok = token;
        token = strtok(NULL, SHELL_TOKEN_DELIMITER);

        if(file_action != 0) rdrw_action();

    }

    check_background(prev_tok, j, bef_tok);
    instructions_pos++;
}


void executer_instructions() {

    while (count < instructions_pos) {

        /* si la commande n'est pas en background, n'est pas un if ou n'est pas
         * le résultat d'un erreur lié à une commande suivi par un and */

        if (is_background == 0 && and_result != -1 &&
            (instruction_list[count].if_statement == 0)) {
            char *current_command = instruction_list[count].command;
            if (current_command != NULL) command_options();

        } else {
            if (and_result == -1) {
                reset_data();
                return;
            } else {
                if(instruction_list[count].if_statement) if_stat();
                else command_options();
            }
        }

        count++;
    }

    reset_data();
}

// ****************************************************************************

// ============
// Screen data|
// ============

void get_shell() {

    clear_shell();
    my_logo();

}

void get_data() {
    username = getlogin();
//    gethostname(hostname, 1024);
    getcwd(cwd, sizeof(cwd));
}

/* va imprimer les datas qui se trouvent à gauche de la ligne de commandes */
void user_prompt() {

    if (first) {
        printf("%s" ":%s >> ", username, cwd);
        first = 0;
    } else { printf("%% %s" ":%s >> ", username, cwd); }

}

void clear_shell() {
    for (int x = 0; x < 10; x++ ) printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
}

void my_logo() {

    printf("%s", "\n\t\t##############      #############\n"
                 "\t\t##############      #############\n"
                 "\t\t######\t\t    ######\n"
                 "\t\t######\t\t    ######\n"
                 "\t\t############## \t    #############\n"
                 "\t\t############## \t    #############\n"
                 "\t\t\t######      ######\n"
                 "\t\t\t######      ######\n"
                 "\t\t##############  ##  ############# ##\n"
                 "\t\t##############  ##  ############# ##\n"
                 "\n\t\t====================================\n\n"
                 "\t\t##      ## ####### ####### ##\n"
                 "\t\t##  ##  ## ####### ####### ##\n"
                 "\t\t##  ##  ## ##	## ##\t   ##\n"
                 "\t\t##  ##  ## ##	## ##\t   ##\n"
                 "\t\t##  ##  ## ####### ####### ##\n"
                 "\t\t##  ##  ## ####### ####### ##   par Wael ABOU ALI\n"
                 "\t\t##  ##  ## ##   ## ##\t   ##\n"
                 "\t\t##  ##  ## ##   ## ##\t   ##\n"
                 "\t\t########## ##   ## ####### #######\n"
                 "\t\t ########  ##   ## ####### #######\n"
                 "\n\t\t====================================\n\n");
}

/* la shell est créée par Wael ABOU ALI (20034365) et .... */
void about() {
    // Source - http://ascii.co.uk/art/beaver
    printf("%s", "\t      n__n_\n"
                 "             /  = =\\\n"
                 "            /   ._Y_)\n"
                 "___________/      \"\\________________________________\n"
                 "          (_/  (_,  \\                        o!O   \n"
                 "            \\      ( \\_,--\"\"\"\"--.\n"
                 "      __..-,-`.___,-` )-.______.' Created by : \n"
                 "    <'     `-,'   `-, )-'    >\n"
                 "     `----._/      ( /\"`>.--\t Wael ABOU ALI\n"
                 "            \"--..___,--\n"
                 "\n\t@ ascii.co.uk/art/beaver\n\n");
}

//strace -e wait4,execve,clone -ff ./trial1^C
// execvp error to do //

// ****************************************************************************

// =====================================================
// Fonctions reliées à la gestion de parser les données|
// =====================================================

// gestion d'espace.
// -----------------

void alloc_mem(int bufsize) {

    instruction_list = malloc(bufsize * sizeof(char *));
    instruction_list[instructions_pos].args =
            malloc(bufsize * sizeof(char *));
    instruction_list[instructions_pos].args[0] =
            malloc(bufsize * sizeof(char *));

    if((!instruction_list[instructions_pos].args[0]) ||
       (!instruction_list[instructions_pos].args)) {
        fprintf(stderr, "shell_error: allocation error\n");
        exit(EXIT_FAILURE);
    }

}

void realloc_mem(int bufsize, int j) {

    bufsize += BUFFER_SIZE;
    if (instructions_pos >= bufsize)
        instruction_list[instructions_pos].args =
                realloc(instruction_list[instructions_pos].args,
                        bufsize * sizeof(char *));
    else
        instruction_list[instructions_pos].args[j] =
                realloc(instruction_list[instructions_pos].args[j],
                        bufsize * sizeof(char *));

    if ((!instruction_list[instructions_pos].args[j]) ||
        (!instruction_list[instructions_pos].args)) {
        fprintf(stderr, "myshell: allocation error\n");
        exit(EXIT_FAILURE);
    }

}

// Fonctions responsable de traiter les différentes parties de l'instruction.
// --------------------------------------------------------------------------

void treat_start(int bufsize, const char *semi) {

    if (instructions_pos != 0)
        instruction_list[instructions_pos].args =
                malloc(bufsize * sizeof(char *));

    if(strcmp(&token[strlen(token) - 1], ";") == 0)
        token[strlen(token) - 1] = '\0';

    if((strcmp(token, "if") != 0) && (strcmp(token, "do") != 0) &&
       (strcmp(token, "done") != 0))
        instruction_list[instructions_pos].command = token;
    else check_if(semi);

}

int treat_end(int j) {

    j = check_opers(j);
    is_background = 0;
    instruction_list[instructions_pos].args_count = j;
    instruction_list[instructions_pos].args[j] = NULL;
    instructions_pos++;
    return  0;

}

// vérifier la syntaxe des instructions
// ------------------------------------

int check_opers(int j) {

    if((strcmp(token, "&&") == 0) || (strcmp(token, "|") == 0)) {

        if(strcmp(token, "|") == 0)
            instruction_list[instructions_pos].bin_oper = 1;
        else instruction_list[instructions_pos].bin_oper = 2;

    } else if(strcmp(&token[strlen(token) - 1], ";") == 0){

        if((strlen(token) - 1) != 0) {
            token[strlen(token) - 1] = '\0';
            instruction_list[instructions_pos].args[j] = token;
        }
        j++;

    } else instruction_list[instructions_pos].bin_oper = 0;

    return j;

}

void check_redirection() {

    if ((strcmp(token, "<") == 0) &&
        (strcmp(instruction_list[instructions_pos].command,
                "cat") == 0)) {

        instruction_list[instructions_pos].cmnd_rd = 1;
        file_action = 1;

    } else if (strcmp(token, ">") == 0) {

        instruction_list[instructions_pos].cmnd_wr = 1;
        file_action = 2;

    }
}

void check_background(char *prev_tok, int j, char *bef_tok) {

    if((token == NULL) && (strcmp(prev_tok, "&") == 0)) {

        char *p;
        timer_int = strtol(bef_tok, &p, 10);
        is_background = 1;
        instruction_list[instructions_pos].args[j - 2] = NULL;
    }

}

// ****************************************************************************

// =====================================
// La gestion d'exécution des commandes|
// =====================================

void command_options() {

    char *current_command = instruction_list[count].args[0];

    if (current_command != NULL) {
        if ((strcmp(current_command, "exit") == 0) ||
            (strcmp(current_command, "z") == 0) ||
            (and_result == 1))
            prem_exit();

        else if (strcmp(current_command, "about") == 0)
            about();

        else if (strcmp(current_command, "screenfetch") == 0)
            my_logo();

        else if (strcmp(current_command, "clear") == 0)
            clear_shell();

        else if ((strcmp(current_command, "mkdir") == 0) ||
                 (strcmp(current_command, "rmdir") == 0)) {

            char *foldername = instruction_list[count].args[1];
            if (strcmp(current_command, "mkdir") == 0)
                shell_action_dir(foldername, 1, instruction_list[count]);
            else shell_action_dir(foldername, 0, instruction_list[count]);
        } else if (strcmp(current_command, "cd") == 0) {

            char *path = instruction_list[count].args[1];
            shell_cd(path, instruction_list[count]);
        } else if (strcmp(current_command, "pwd") == 0) shell_pwd(cwd, 1, instruction_list[count]);


        else shell_commands(instruction_list[count]);
    }

}

void shell_commands(Instruction cmd_list) {

    pid_t child;
    int status;
    char *msg = "**execvp() error.**";
    child = fork();

    if(child == 0) {

        if(cmd_list.cmnd_rd || cmd_list.cmnd_wr)
            read_write(cmd_list);

        if(is_background) {
            sleep((unsigned int)timer_int);
            is_background = 0;
        }

        if(is_background == 0) {
            if (timer_int != 0)
                printf("\n[après %ld seconde ... fini sleep %ld]\n", timer_int, timer_int);
            execvp(cmd_list.args[0], cmd_list.args);
            error_check(cmd_list, msg);
        }

    } else {

        if(child < 0) {
            fprintf(stderr, "Fork failed.\n");
            exit(1);
        } else {
            if(is_background == 0) waitpid(child, &status, 0);
            else printf("[commence sleep %ld, rien imprimé à l’écran]\n", timer_int);
        }

    }
}

// Shell non-builtin commands
//---------------------------

void shell_action_dir(char *name, int action_switch, Instruction cmd_list) {

    int stat;
    char *msg = "+-- Error in mkdir --+ ";
    if(action_switch) stat = mkdir(name, 0777);  // Pour avoir les permissions.
    else stat = rmdir(name);
    if(stat == -1) error_check(cmd_list, msg);

}


void shell_cd(char* path, Instruction cmd_list) {

    int curr_wd = chdir(path);
    char *msg = "+-- Error in cd --+ ";
    if(curr_wd == 0) shell_pwd(cwd,0, cmd_list);
    else error_check(cmd_list, msg);

}

void shell_pwd(char* cwd,int command, Instruction cmd_list) {

    char current_wd[BUFFER_SIZE];
    char *path = getcwd(current_wd, sizeof(current_wd));
    char *msg = "+--- Error in getcwd() : ";

    if(path != NULL) {
        strcpy(cwd, current_wd);
        if(command == 1) printf("%s\n",cwd);
    }
    else error_check(cmd_list, msg);

}

// ****************************************************************************

// ====================================================
// Fonctions reliées à la gestion de sortie prématurée|
// ====================================================

void error_check(Instruction cmd_list, char *msg) {

    if(cmd_list.bin_oper == 2 || cmd_list.if_statement == 1 ||
       cmd_list.do_statement == 1) and_result = -1;
    perror(msg);
    reset_data();
    shell_loop();

}

void reset_data() {

    free(instruction_list);
    free(line);
    error_signal = 0;
    and_result = 0;
    file_action = 0;
    is_background = 0;
    timer_int = 0;
    for(int i = 0; i < instructions_pos; i++) {
        for(int j = 0; j < instruction_list[i].args_count; j++) {
            free(instruction_list[i].args);
            instruction_list[i].args = NULL;
        }
    }
}

/* Pour quitter le programme normalement */
int prem_exit() {
    exit_flag = 1;
    return 0;
}

// ****************************************************************************

// ===========================================================================
// La gestion des instructions spécifiques soit à partir de la redirection ou|
// l'instruction if.                                                         |
// ===========================================================================

// read/write action
// -----------------

void rdrw_action() {

    if(file_action == 1) {
        strcpy(in_file,token);
        file_action = 0;
    } else {
        strcpy(out_file,token);
        file_action = 0;
    }

}

void read_write(Instruction cmd_list) {

    int success_action = 0, out_file_desc;

    if(cmd_list.cmnd_rd) {
        success_action = open(in_file, O_RDONLY);
        if(success_action < 0) {
            perror("+-- Fichier non valide --+ ");
        } else {

            dup2(success_action, STDIN_FILENO);
            close(success_action);
        }
    } else {
        out_file_desc = open(out_file, O_CREAT | O_WRONLY,
                             S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
        dup2(out_file_desc, 1);
        close(out_file_desc);
        if(out_file_desc < 0) {
            perror("+-- Fichier non valide --+ ");
        }
    }
}

// ****************************************************************************

// Le traitment de (if)
// --------------------

void check_if(const char *semi) {

    if(strcmp(token, "if") == 0) {

        instruction_list[instructions_pos].if_statement = 1;
        instruction_list[instructions_pos].command = token;
        token = strtok(NULL, SHELL_TOKEN_DELIMITER);

    } else {

        if((strcmp(token, "do") == 0) &&
           (instruction_list[instructions_pos - 1].if_statement == 1)) {

            instruction_list[instructions_pos].do_statement = 1;
            instruction_list[instructions_pos].command = token;
            token = strtok(NULL, SHELL_TOKEN_DELIMITER);

        } else {

            if ((strcmp(token, "done") == 0) &&
                (instruction_list[instructions_pos - 1].do_statement == 1)) {

                instruction_list[instructions_pos].done_statement = 1;
                instruction_list[instructions_pos].command = token;
                strcpy(token, semi);

            } else printf("+-- bad syntax for an if --+\n");

        }

    }

}

void if_stat() {

    if((error_signal == 0))

        if(((instruction_list[count + 1].do_statement) &&
            (instruction_list[count + 2].done_statement)) ||
           (instruction_list[count + 1].if_statement))
            command_options();
        else printf("+-- bad syntax for an if --+\n");

    else printf("welcome to multiple commands!\n");


}

// ****************************************************************************
