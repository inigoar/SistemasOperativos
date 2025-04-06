/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h>            /* NULL */
#include <stdio.h>            /* setbuf, printf */
#include <stdlib.h>

#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

extern int obtain_order();        /* See parser.y for description */

int gestionarMandatos(int mandato, int background, int argc, char ***args);
int gestionarSecuencias(int background, int argumentos, char ****argvv);
int gestionarRedirreciones(int redireccion, char *filename);
void restaurarRedirecciones(int redirEntrada, int redirSalida, int redirError, char *filev[3]);
void ejecutar(char ***argv);
void esperaProcesoHijo(pid_t pid, int bg, int *estado);
void configuracionSeñalesA(struct sigaction *action);
void configuracionSeñalesB(struct sigaction *action);

int main(void)
{
    char ***argvv = NULL;
    int argvc;
    char **argv = NULL;
    int argc;
    char *filev[3] = { NULL, NULL, NULL };
    int bg;
    int ret;
    struct sigaction signalActionA;
    int redireccionEntrada;
    int redireccionSalida;
    int redireccionError;

    setbuf(stdout, NULL);            /* Unbuffered */
    setbuf(stdin, NULL);

    configuracionSeñalesA(&signalActionA);

    while (1) {
        fprintf(stderr, "%s", "msh> ");    /* Prompt */
        ret = obtain_order(&argvv, filev, &bg);
        if (ret == 0) break;        /* EOF */
        if (ret == -1) {
            fprintf(stderr, "Se ha producido un error en la sintaxis de obtenir order\n");
            continue;    /* Syntax error */
        }
        argvc = ret - 1;        /* Line */
        if (argvc == 0) {
            fprintf(stderr, "linea vacia obtain order\n");
            continue;    /* Empty line */
        }

        argc = 0;
        argv = argvv[0];

        if((filev[0] != NULL) && ((redireccionEntrada = gestionarRedirreciones(1, filev[0])) == -1)){continue;}
        if((filev[1] != NULL) && ((redireccionSalida = gestionarRedirreciones(2, filev[1])) == -1)){continue;}
        if((filev[2] != NULL) && ((redireccionError = gestionarRedirreciones(3, filev[2])) == -1)){continue;}

        if(argvc == 1){

            for(int contadorArgumentos = 0; argv[contadorArgumentos]; contadorArgumentos++){argc++;}

            if(strcmp(argv[0], "cd") == 0){
                if(gestionarMandatos(1, bg, argc, &argv) == -1){continue;}
            }
            else if(strcmp(argv[0], "umask") == 0){
                if(gestionarMandatos(2, bg, argc, &argv) == -1){continue;}
            }
            else if(strcmp(argv[0], "limit") == 0){
                if(gestionarMandatos(3, bg, argc, &argv) == -1){continue;}
            }
            else if(strcmp(argv[0], "set") == 0){
                if(gestionarMandatos(4, bg, argc, &argv) == -1){continue;}
            }
            else {
                if(gestionarMandatos(5, bg, argc, &argv) == -1){continue;}
            }

        } else if(argvc >= 1){
            if(gestionarSecuencias(bg, argvc, &argvv) == -1){continue;}
        }
        restaurarRedirecciones(redireccionEntrada, redireccionSalida, redireccionError, filev);

    }//WHILE
    exit(0);
    return 0;
}

int gestionarRedirreciones(int redireccion, char *filename){

    if(redireccion == 1){

        int fdEntrada;
        fdEntrada = open(filename, O_RDONLY);
        if (fdEntrada == -1) {
            perror("Se ha producido error en el comando open");
            return -1;
        }

        int redirEntrada;
        redirEntrada = dup(STDIN_FILENO);
        if (redirEntrada == -1) {
            perror("Se ha producido error en el comando dup");
            close(fdEntrada);
            return -1;
        }

        if (close(STDIN_FILENO) == -1 || dup(fdEntrada) == -1) {
            perror("Se ha producido error en el comando close/dup");
            close(fdEntrada);
            close(redirEntrada);
            return -1;
        }

        close(fdEntrada);
        return redirEntrada;

    }else if(redireccion == 2){
        int fdSalida;
        fdSalida = creat(filename, 0666);
        if (fdSalida == -1) {
            perror("Se ha producido error en el comando creat");
            return -1;
        }

        int redirSalida;
        redirSalida = dup(STDOUT_FILENO);
        if (redirSalida == -1) {
            perror("Se ha producido error en el comando dup");
            close(fdSalida);
            return -1;
        }

        if (close(STDOUT_FILENO) == -1 || dup(fdSalida) == -1) {
            perror("Se ha producido error en el comando close/dup");
            close(fdSalida);
            close(redirSalida);
            return -1;
        }

        close(fdSalida);
        return redirSalida;


    }else if(redireccion == 3){
        int fdError;
        fdError = creat(filename, 0666);
        if (fdError == -1) {
            perror("Se ha producido error en el comando creat");
            return -1;
        }

        int redirError;
        redirError = dup(STDERR_FILENO);
        if (redirError == -1) {
            perror("Se ha producido error en el comando dup");
            close(fdError);
            return -1;
        }

        if (close(STDERR_FILENO) == -1 || dup(fdError) == -1) {
            perror("Se ha producido error en el comando close/dup");
            close(fdError);
            close(redirError);
            return -1;
        }

        close(fdError);
        return redirError;

    }
    return 0;
}

void restaurarRedirecciones(int redirEntrada, int redirSalida, int redirError, char *filev[3]){

    int redireccion;
    int descriptor;

    if(filev[0] != NULL){
        redireccion = redirEntrada;
        descriptor = 0;
        dup2(redireccion, descriptor);
        close(redireccion);
    }
    if(filev[1] != NULL){
        redireccion = redirSalida;
        descriptor = 1;
        dup2(redireccion, descriptor);
        close(redireccion);
    }
    if(filev[2] != NULL){
        redireccion = redirError;
        descriptor = 2;
        dup2(redireccion, descriptor);
        close(redireccion);
    }
}

int gestionarMandatos(int mandato, int background, int argc, char ***args){

    pid_t pid;
    char **argv = *args;

    if(mandato == 1){
        char cwd[PATH_MAX];
        char *dirHome = NULL;

        if(argc > 2){
            fprintf(stderr, "Se ha producido un error en el uso del mandato cd, revise el numero de argumentos\n");
            return -1;
        }

        if(argc == 1){
            if(background == 0){
                dirHome = getenv("HOME");
                if (chdir(dirHome) == -1){
                    perror("Se ha producido un error en el comando chdir cd");
                    return -1;
                }
            } else {
                pid = fork();
                if(pid == -1){
                    perror("Se ha producido un error en el comando fork cd");
                    return -1;
                }
                else if(pid == 0){
                    dirHome = getenv("HOME");
                    if (chdir(dirHome) == -1) {
                        perror("Se ha producido un error en el comando chdir cd");
                        return -1;
                    }
                    return 0;
                }
                else{
                    fprintf(stdout, "Proceso padre\n");
                }
            }
        }
        else if(argc == 2){
            if(background == 0){
                if (chdir(argv[1]) == -1){
                    perror("Se ha producido un error en el comando chdir cd");
                    return -1;
                }
            } else {
                pid = fork();

                if(pid == -1){
                    perror("Se ha producido un error en el comando fork cd");
                    return -1;
                }
                else if(pid == 0){
                    if(chdir(argv[1]) == -1){
                        perror("Se ha producido un error en el comando chdir cd");
                        return -1;
                    }
                    return 0;
                }
                else{
                    fprintf(stdout, "Proceso padre\n");
                }
            }
        }
        fprintf(stdout, "%s\n", getcwd(cwd, sizeof(cwd)));
        return 0;

    } else if(mandato == 2){

        mode_t mascara;

        if(argc > 2){
            fprintf(stderr, "Se ha producido un error en el uso del comando umask, revise los argumentos\n");
            return -1;
        }

        if(argc == 1) {
            if(background == 0){
                mascara = umask(0);
                umask(mascara);
                fprintf(stdout, "%o\n", mascara);
            } else {
                pid = fork();

                if(pid == -1){
                    perror("Se ha producido un error en el comando fork umask");
                    return -1;
                }
                else if(pid == 0){
                    mascara = umask(0);
                    umask(mascara);
                    fprintf(stdout, "%o\n", mascara);
                    return 0;
                }
                else{
                    fprintf(stdout, "Proceso padre\n");
                }
            }
        } else if (argc == 2) {
            char *controlStrtol;
            mode_t mascaraOctal = (mode_t) strtol(argv[1], &controlStrtol, 8);

            if (isdigit(*controlStrtol)){
                fprintf(stderr, "Se ha producido un error en el uso del mandato umask, comprobar formato octal\n");
                return -1;
            }

            if (background == 0) {
                mascara = umask(mascaraOctal);
                fprintf(stdout, "%o\n", mascaraOctal);
            } else {
                pid = fork();

                if(pid == -1){
                    perror("Se ha producido un error en el comando fork umask");
                    return -1;
                }
                else if(pid == 0){
                    mascara = umask(mascaraOctal);
                    fprintf(stdout, "%o\n", mascaraOctal);
                    return 0;
                }
                else{
                    fprintf(stdout, "Proceso padre\n");
                }
            }
        }

        return 0;

    } else if(mandato == 3){

        if(argc > 2){
            fprintf(stderr, "Se ha producido un error en el uso del comando limit, revise los argumentos\n");
            return -1;
        }

        if(argc == 1){
            return 0;
        }

        if(argc == 2){
            return 0;
        }

        return 0;

    } else if(mandato == 4){

        if(argc > 2){
            fprintf(stderr, "Se ha producido un error en el uso del comando set, revise los argumentos\n");
            return -1;
        }

        return 0;

    } else if(mandato == 5){

        struct sigaction signalActionB;
        int estado;

        pid = fork();

        if (pid == -1){
            perror("Se ha producido error en el comando fork mandato no interno");
            return -1;
        } else if (pid == 0){
            if (background == 0){
                configuracionSeñalesB(&signalActionB);
            }
            ejecutar(args);
        } else {
            esperaProcesoHijo(pid, background, &estado);
        }
        return estado;
    }

    return 0;
}

void esperaProcesoHijo(pid_t pid, int background, int *estado){
    int espera;
    if (background == 0) {
        do {
            espera = waitpid(pid, estado, 0);
        } while (espera == -1 && errno == EINTR);
    } else if (background == 1) {
        fprintf(stdout, "%d\n", pid);
    }
}

void ejecutar(char ***argv){
    if (execvp((*argv)[0], *argv) == -1) {
        perror("Se ha producido un error en el comando execvp");
        exit(-1);
    }
}


int gestionarSecuencias(int background, int argumentos, char ****argvv){
    struct sigaction signalActionB;
    int tuberia[argumentos-1][2];
    char ***secuencia = *argvv;
    pid_t pid;
    char **argv = NULL;
    int ultimo;
    int sec;

    for(int i = 0; i < (argumentos-1); i++){
        if(pipe(tuberia[i]) == -1){
            perror("Se ha producido un error al crear la tuberia");
            return -1;
        }
    }
    if(argumentos == 2){
        argv = secuencia[0];
        pid = fork();

        if(pid == -1){
            perror("Se ha producido un error en el comando fork secuencia");
            return -1;
        }
        else if(pid == 0){
            if(background == 0){
                configuracionSeñalesB(&signalActionB);
                fprintf(stderr, "Sse ha creado un fork\n");
            }

            close(tuberia[0][0]);
            dup2(tuberia[0][1],1);
            close(tuberia[0][1]);

            if(execvp(argv[0], argv) == -1) {
                perror("Se ha producido un error en el comando execpv secuencia");
                return -1;
            }
        }

        argv = secuencia[1];
        pid = fork();

        if(pid == -1){
            perror("Se ha producido un error en el comando fork secuencia");
            return -1;
        }
        else if(pid == 0){
            if(background == 0){
                configuracionSeñalesB(&signalActionB);
            }

            close(tuberia[0][1]);
            dup2(tuberia[0][0],0);
            close(tuberia[0][0]);

            if(execvp(argv[0], argv) == -1) {
                perror("Se ha producido un error en el comando execpv secuencia");
                return -1;
            }
        }
    }
    else if(argumentos > 2){
        sec = argumentos - 1;
        ultimo = argumentos - 2;
        argv = secuencia[0];
        pid = fork();

        if(pid == -1){
            perror("Se ha producido un error en el comando fork secuencia");
            return -1;
        }
        else if(pid == 0){
            if(background == 0){
                configuracionSeñalesB(&signalActionB);
                fprintf(stderr, "Se ha creado un fork\n");
            }

            close(tuberia[0][0]);
            dup2(tuberia[0][1],1);
            close(tuberia[0][1]);

            if(execvp(argv[0], argv) == -1) {
                perror("Se ha producido un error en el comando execpv secuencia");
                return -1;
            }
        }
        for(int j = 1; j < sec; j++){

            argv = secuencia[j];
            pid = fork();

            if(pid == -1){
                perror("Se ha producido un error en el comando fork secuencia");
                return -1;
            }
            else if(pid == 0){
                if(background == 0){
                    configuracionSeñalesB(&signalActionB);
                    fprintf(stderr, "Se ha creado un fork\n");
                }

                close(tuberia[j][0]);
                close(tuberia[j-1][1]);

                for(int k = 0; k < ultimo; k++){
                    if((k != (j-1)) && (k != j)){
                        close(tuberia[k][1]), close(tuberia[k][0]);
                    }
                }

                dup2(tuberia[j-1][0],0);
                dup2(tuberia[j][1],1);
                close(tuberia[j-1][0]);
                close(tuberia[j][1]);

                if(execvp(argv[0], argv) == -1) {
                    perror("Se ha producido un error en el comando execpv secuencia");
                    return -1;
                }

            }
        }

        argv = secuencia[sec];
        pid = fork();

        if(pid == -1){
            perror("Se ha producido un error en el comando fork secuencia");
            return -1;
        }
        else if(pid == 0){
            if(background == 0){
                configuracionSeñalesB(&signalActionB);
            }

            close(tuberia[ultimo][1]);

            for(int m = 0; m < ultimo; m++){
                close(tuberia[m][0]), close(tuberia[m][1]);
            }

            dup2(tuberia[ultimo][0],0);
            close(tuberia[ultimo][0]);

            if(execvp(argv[0], argv) == -1) {
                perror("Se ha producido un error en el comando execpv secuencia");
                return -1;
            }
        }
    }

    sec = argumentos - 1;

    for(int n = 0; n < sec; n++){
        close(tuberia[n][1]), close(tuberia[n][0]);
    }

    int estado;
    esperaProcesoHijo(pid, background, &estado);

    return estado;
}

void configuracionSeñalesA(struct sigaction *action){
    action->sa_handler = SIG_IGN;
    sigaction(SIGQUIT, action, NULL);
    sigaction(SIGINT, action, NULL);
}

void configuracionSeñalesB(struct sigaction *action){
    action->sa_handler = SIG_DFL;
    sigaction(SIGQUIT, action, NULL);
    sigaction(SIGINT, action, NULL);
}

