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
        open_calls++;
        printf("Señal SIGUSR1 recibida\n");
        log_syscall("open", getpid());
    } else if (signum == SIGUSR2) {
        total_calls++;
        write_calls++;
        printf("Señal SIGUSR2 recibida\n");
        log_syscall("write", getpid());
    } else if (signum == SIGRTMIN) {
        total_calls++;
        read_calls++;
        printf("Señal SIGRTMIN recibida\n");
        log_syscall("read", getpid());
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
    int pipe1[2], pipe2[2];
    pid_t pid1, pid2;

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
    signal(SIGRTMIN, sig_handler); // Señal personalizada para lectura
    signal(SIGINT, sig_handler);

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
    }

    // Close writing ends of pipes in the parent process
    close(pipe1[1]);
    close(pipe2[1]);

    // Read PIDs from pipes
    char buffer[16];
    read(pipe1[0], buffer, sizeof(buffer));
    pid1 = (pid_t) atoi(buffer);
    read(pipe2[0], buffer, sizeof(buffer));
    pid2 = (pid_t) atoi(buffer);

    // Print PIDs (optional)
    printf("PID del hijo 1: %d\n", pid1);
    printf("PID del hijo 2: %d\n", pid2);

    // Pass the PIDs to the SystemTap script (code for this part should be written separately)

    // Wait for child processes to finish
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}