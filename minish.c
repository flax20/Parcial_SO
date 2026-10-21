//Mártin Hernández-Thomas Chanique

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <stdbool.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIMITERS " \t\r\n"

//wls
//gcc -Wall -Wextra -std=c99 minish.c -o minish
//./minish
//pwd
//ls
//ls -l


int total_procesos_ejecutados = 0;
long total_max_rss_kb = 0;

void mostrar_memoria_sistema() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        long m_libre = (info.freeram * info.mem_unit) / (1024 * 1024);
        long m_total = (info.totalram * info.mem_unit) / (1024 * 1024);
        printf("[Memoria Sistema] Libre: %ld MB / Total: %ld MB\n", m_libre, m_total);
    } else {
        perror("Error al obtener sysinfo");
    }
}

void verificar_procesos_segundo_plano() {
    int status;
    pid_t pid;
    struct rusage usage;

    while ((pid = wait4(-1, &status, WNOHANG, &usage)) > 0) {
        printf("\n[Proceso en Segundo Plano Finalizado] PID: %d\n", pid);
        printf(" -> Memoria máxima retenida (RSS): %ld KB\n", usage.ru_maxrss);
        mostrar_memoria_sistema();
        
        total_procesos_ejecutados++;
        total_max_rss_kb += usage.ru_maxrss;
    }
}


void comando_help() {
    printf("\n--- minish - Lista de Uso ---\n");
    printf("cd [dir]  : Cambia el directorio de trabajo actual.\n");
    printf("help      : Muestra esta pantalla de ayuda.\n");
    printf("exit      : Finaliza la mini-shell mostrando estadísticas.\n");
    printf("Uso de &: Agregue '&' al final para ejecutar en segundo plano.\n\n");
}


void comando_cd(char **args) {
    if (args[1] == NULL) {
        // Si no se especifica ruta intenta ir a HOME
        char *home = getenv("HOME");
        if (home == NULL || chdir(home) != 0) {
            perror("minish: cd");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("minish: cd");
        }
    }
}

bool ejecutar_comando(char **args, bool es_background) {

    // Comandos Internos
    if (strcmp(args[0], "exit") == 0) {
        return false;
    }
    if (strcmp(args[0], "help") == 0) {
        comando_help();
        return true;
    }
    if (strcmp(args[0], "cd") == 0) {
        comando_cd(args);
        return true;
    }

    // Comandos Externo
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso (fork)");
    } 
    else if (pid == 0) {
        // Código del Proceso Hijo
        if (execvp(args[0], args) == -1) {
            perror("Error de ejecución (execvp)");
        }
        exit(EXIT_FAILURE); // Termina el hijo si exec falla
    } 
    else {
        // Código del Proceso Padre (Shell)
        if (!es_background) {
            int status;
            struct rusage usage;

            wait4(pid, &status, 0, &usage);

            printf("[Proceso Finalizado] PID: %d\n", pid);
            printf(" -> Memoria máxima retenida (RSS): %ld KB\n", usage.ru_maxrss);
            mostrar_memoria_sistema();

            total_procesos_ejecutados++;
            total_max_rss_kb += usage.ru_maxrss;
        } else {
            printf("[Proceso en Segundo Plano Iniciado] PID: %d\n", pid);
        }
    }

    return true;
}


int main(void) {
    char linea[MAX_LINE];
    char *args[MAX_ARGS];
    bool ejecutar = true;

    printf("Bienvenido a minish. Digite 'help' para ver los comandos disponibles.\n");

    while (ejecutar) {
       
        verificar_procesos_segundo_plano();


        printf("minish> ");
        fflush(stdout);

        if (fgets(linea, sizeof(linea), stdin) == NULL) {
            printf("\n");
            break;
        }

        int posicion = 0;
        char *token = strtok(linea, DELIMITERS);
        while (token != NULL && posicion < MAX_ARGS - 1) {
            args[posicion] = token;
            posicion++;
            token = strtok(NULL, DELIMITERS);
        }
        args[posicion] = NULL;

        if (posicion == 0) {
            continue; 
        }


        bool es_background = false;
        if (posicion > 0 && strcmp(args[posicion - 1], "&") == 0) {
            es_background = true;
            args[posicion - 1] = NULL; // Remover el token '&' para la ejecución
        }

        ejecutar = ejecutar_comando(args, es_background);
    }

    printf("\n--- Resumen Final de Ejecución ---\n");
    printf("Total de procesos externos completados: %d\n", total_procesos_ejecutados);
    printf("Consumo acumulado de memoria máxima (RSS): %ld KB\n", total_max_rss_kb);
    mostrar_memoria_sistema();
    printf("Saliendo de minish. ¡Adiós!\n");

    return EXIT_SUCCESS;
}
