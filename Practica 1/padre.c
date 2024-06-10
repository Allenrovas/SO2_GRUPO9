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

int total_calls = 0;
int read_calls = 0;
int write_calls = 0;
int open_calls = 0;
pid_t pid1 = 0; // Variables globales para los PIDs de los procesos hijos
pid_t pid2 = 0;

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
    pid_t pid = getpid(); // Obtener el PID del proceso actual
    if (signum == SIGUSR1) {
        total_calls++;
        open_calls++;
        printf("Señal SIGUSR1 recibida\n");
        //log_syscall("open", pid);
    } else if (signum == SIGUSR2) {
        total_calls++;
        write_calls++;
        printf("Señal SIGUSR2 recibida\n");
        //log_syscall("write", pid);
    } else if (signum == SIGRTMIN) {
        total_calls++;
        read_calls++;
        printf("Señal SIGRTMIN recibida\n");
        //log_syscall("read", pid);
    }
}

void sigint_handler(int signum) {
    printf("\nNúmero total de llamadas al sistema: %d\n", total_calls);
    printf("Número de llamadas al sistema por tipo:\n");
    printf("Read: %d\n", read_calls);
    printf("Write: %d\n", write_calls);
    printf("Open: %d\n", open_calls);
    exit(EXIT_SUCCESS);
}

int main() {
    int pipe1[2], pipe2[2];

    // Initialize logfile
    FILE *log_file = fopen(LOGFILE, "w");
    if (log_file == NULL) {
        perror("Error al crear el archivo de log");
        exit(EXIT_FAILURE);
    }
    fclose(log_file);

    // Print PID of parent process
    printf("PID del proceso padre: %d\n", getpid());

    // Set up signal handlers
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGRTMIN, sig_handler); // Señal personalizada para lectura
    signal(SIGINT, sigint_handler);

    // Create pipes
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Error al crear las tuberías");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) { // Child process 1
        close(pipe1[0]); // Close reading end
        dup2(pipe1[1], STDOUT_FILENO); // Redirect stdout to pipe
        execl("./hijo.bin", "./hijo.bin", NULL);
        perror("Error al ejecutar el proceso hijo 1");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipe1[1]); // Close writing end
        char buffer[16];
        read(pipe1[0], buffer, sizeof(buffer));
        pid1 = (pid_t) atoi(buffer);
        printf("PID del proceso hijo 1: %d\n", pid1);
        close(pipe1[0]); // Close reading end
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) { // Child process 2
        close(pipe2[0]); // Close reading end
        dup2(pipe2[1], STDOUT_FILENO); // Redirect stdout to pipe
        execl("./hijo.bin", "./hijo.bin", NULL);
        perror("Error al ejecutar el proceso hijo 2");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipe2[1]); // Close writing end
        char buffer[16];
        read(pipe2[0], buffer, sizeof(buffer));
        pid2 = (pid_t) atoi(buffer);
        printf("PID del proceso hijo 2: %d\n", pid2);
        close(pipe2[0]); // Close reading end
    }

    // Esperar a que ambos procesos hijos terminen
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // Ejecutar el script de SystemTap
        char command[100];
        sprintf(command, "sudo stap trace.stp %d %d > syscalls.log", pid1, pid2);
        system(command);


    return 0;
}
