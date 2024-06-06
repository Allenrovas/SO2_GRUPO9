
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define LOGFILE "syscalls.log"
#define MY_SIGNAL SIGRTMIN  // Definir una señal en tiempo real personalizada
#define OPEN_SIGNAL (SIGRTMIN + 1)  // Definir una señal en tiempo real personalizada para aperturas de archivos

int total_calls = 0;
int read_calls = 0;
int write_calls = 0;
int open_calls = 0;

void log_syscall(const char *syscall_name, pid_t pid) {
    FILE *log_file = fopen(LOGFILE, "a");
    if (log_file == NULL) {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }

    time_t current_time;
    time(&current_time);
    char *time_str = ctime(&current_time);
    time_str[strlen(time_str) - 1] = '\0'; // Remove trailing newline from ctime result

    fprintf(log_file, "Proceso %d: %s (%s)\n", pid, syscall_name, time_str);
    fclose(log_file);
}

void sig_handler(int signum) {
    if (signum == SIGUSR1) {
        total_calls++;
        printf("Received SIGUSR1\n");
    } else if (signum == SIGUSR2) {
        read_calls++;
        printf("Received SIGUSR2\n");
    } else if (signum == MY_SIGNAL) {
        write_calls++;
        printf("Received MY_SIGNAL\n");
    } else if (signum == OPEN_SIGNAL) {
        open_calls++;
        printf("Received OPEN_SIGNAL\n");
    } else if (signum == SIGINT) {
        printf("\nNúmero total de llamadas al sistema: %d\n", total_calls);
        printf("Número de llamadas al sistema por tipo:\n");
        printf("Read: %d\n", read_calls);
        printf("Write: %d\n", write_calls);
        printf("Open: %d\n", open_calls);
        exit(EXIT_SUCCESS);
    }
}

int main() {
    // Initialize logfile
    FILE *log_file = fopen(LOGFILE, "w");
    if (log_file == NULL) {
        perror("Error al crear el archivo de log");
        exit(EXIT_FAILURE);
    }
    fclose(log_file);

    // Set up signal handlers
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(MY_SIGNAL, sig_handler); // Manejar la señal personalizada
    signal(OPEN_SIGNAL, sig_handler); // Manejar la señal de apertura de archivos
    signal(SIGINT, sig_handler);

    // Fork two child processes
    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) { // Child process 1
        execl("./hijo", "./hijo", NULL);
        perror("Error al ejecutar el proceso hijo 1");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) { // Child process 2
        execl("./hijo", "./hijo", NULL);
        perror("Error al ejecutar el proceso hijo 2");
        exit(EXIT_FAILURE);
    }

    // Wait for child processes to finish
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}