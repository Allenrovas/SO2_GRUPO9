# PRACTICA 1 - Sistemas Operativos 2 - Grupo 9

**Luis Manuel Chay Marrroquin - 202000343**

**Allen Giankarlo Roman Vasquz - 202004745**

## Procesos padre

### 1. Crear dos procesos hijos

Este es el proceso principal, que crea dos procesos hijos haciendo uso del syscall `fork()`.

```c
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
```

### 2. Creacion de logs de syscalls

Para monitorear las llamadas del sistema se hace uso de una función que escribe las llamadas de sistema que realiza un proceso en un archivo de texto con el siguiente formato: Proceso [pid]: [syscall] ([fecha y hora])

```c
void log_syscall(const char *syscall_name, pid_t pid) {
    FILE *log_file = fopen(LOGFILE, "a");
    if (log_file == NULL) {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }
    time_t current_time;
    time(&current_time);
    char *time_str = ctime(&current_time);
    time_str[strlen(time_str) - 1] = '\0';
    fprintf(log_file, "Proceso %d: %s (%s)\n", pid, syscall_name, time_str);
    fclose(log_file);
}
```

### 3. Señal SIGINT

El proceso padre finaliza al recibir la señal SIGINT (Ctrl + C) y es capturada para imprimir los datos antes de terminar su ejecución:
Número total de llamadas al sistema: 23

Número de llamadas al sistema por tipo:
- Read: 6
- Write: 7
- Open: 10

```c
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
```

## Procesos hijo

### 1. Cracion de procesos hijos
Cada proceso hijo tendra un ciclo donde realizaran llamadas del sistema para manejo de archivos sobre un archivo de texto llamado `practica1.txt`. UNa vez abierto el archivo los procesos proceden a realizar llamadas al sistema de manera aletoria que son:
- Open: Llamada al sistema para abrir un archivo. `open()`
- Read: Llamada al sistema para leer un archivo. `read()`
- Write: Llamada al sistema para escribir en un archivo, escribe una linea de texto con 8 caracteres alfanumericos aleatorios. `write()`

Todas las llamadas seran realziadas con un lapso de tiempo aleatorio entre 1 y 3 segundos.

```c
int main() {
    srand(time(NULL));

    initialize_file(FILENAME);

    while (1) {
        int action = rand() % 3;
        int sleep_time = (rand() % 3) + 1;
        sleep(sleep_time);

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
                write(fd, random_string, BUFFER_SIZE);
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
```

## Comunicación entre procesos

### 1. Señales (Padre)
El proceso padre recibe las señales personalizadas de los procesos hijos y las señales de interrupción del usuario. Para esto se define una función que maneja las señales y se asigna a cada señal un handler.

```c
int main() {
    // Set up signal handlers
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(MY_SIGNAL, sig_handler);
    signal(SIGINT, sig_handler);
}
``` 

### 2. Señales (Hijo)
Los procesos hijos envían señales personalizadas al proceso padre para llevar un conteo de las llamadas al sistema que realizan.

```c
void send_signal(int signum) {
    pid_t parent_pid = getppid();
    kill(parent_pid, signum);
}
```