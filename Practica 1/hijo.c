#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define FILENAME "practica1.txt"
#define BUFFER_SIZE 8

void send_signal(int signum) {
    pid_t parent_pid = getppid();
    kill(parent_pid, signum);
}

char *generate_random_string(int length) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *random_string = NULL;

    if (length) {
        random_string = malloc(sizeof(char) * (length + 1));
        if (random_string) {
            for (int n = 0; n < length; n++) {
                int key = rand() % (int)(sizeof(charset) - 1);
                random_string[n] = charset[key];
            }
            random_string[length] = '\0';
        }
    }
    return random_string;
}

void initialize_file(const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error al crear o vaciar el archivo");
        exit(EXIT_FAILURE);
    }
    close(fd);
}

int main() {
    srand(time(NULL));

    // Escribir el PID en la tubería y continuar con la ejecución
    printf("%d\n", getpid());
    fflush(stdout);

    initialize_file(FILENAME);

    while (1) {
        int action = rand() % 3;
        int sleep_time = (rand() % 3) + 1;
        sleep(sleep_time);

        // Enviar la señal respectiva al proceso padre
        switch (action) {
            case 0:  // Open
                send_signal(SIGUSR1);
                break;

            case 1:  // Write
                send_signal(SIGUSR2);
                break;

            case 2:  // Read
                send_signal(SIGRTMIN); // Señal personalizada para lectura
                break;
        }

        // Realizar la operación respectiva
        int fd;
        switch (action) {
            case 0:  // Open
                fd = open(FILENAME, O_RDWR);
                if (fd < 0) {
                    perror("Error al abrir el archivo");
                    exit(EXIT_FAILURE);
                }
                close(fd);
                break;

            case 1:  // Write
                fd = open(FILENAME, O_RDWR | O_APPEND);
                if (fd < 0) {
                    perror("Error al abrir el archivo");
                    exit(EXIT_FAILURE);
                }
                char *random_string = generate_random_string(BUFFER_SIZE);
                if (write(fd, random_string, BUFFER_SIZE) < 0) {
                    perror("Error al escribir en el archivo");
                    free(random_string);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                free(random_string);
                close(fd);
                break;

            case 2:  // Read
                fd = open(FILENAME, O_RDONLY);
                if (fd < 0) {
                    perror("Error al abrir el archivo");
                    exit(EXIT_FAILURE);
                }
                char buffer[BUFFER_SIZE + 1];
                ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE);
                if (bytes_read < 0) {
                    perror("Error al leer el archivo");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                buffer[bytes_read] = '\0';
                close(fd);
                break;
        }
    }

    return 0;
}