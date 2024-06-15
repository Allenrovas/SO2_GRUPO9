#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "cJSON.h"

// Declaración de la estructura thread_data
struct thread_data {
    char* filename;
    int hilo_id;
    int operaciones_realizadas;
    int* errores;
    int errores_size;
};

// Declaración de la estructura Operacion
struct Operacion {
    int operacion;
    int cuenta1;
    int cuenta2;
    double monto;
};

// Prototipos de funciones
void* cargarOperacionesEnHilo(void* arg);
void generarReporteUsuarios(struct thread_data* datos, int* errores, int num_hilos);
void read_json_fileUsers(char *filename, struct thread_data* data);
void* cargarUsuariosEnHilo(void* arg);
void cargarUsuarios();
void deposito(struct Operacion* op);
void retiro(struct Operacion* op);
void transaccion(struct Operacion* op);
void consultarCuenta();
void cargarOperaciones();
void estadoDeCuenta();
void ReporteEstadoDeCuenta();
void reporteCargarUsuarios();
void reporteCargarOperaciones();

void generarReporteOperaciones(struct thread_data* datos, int* errores, int num_hilos);

// Variables globales para los usuarios y operaciones
struct Usuario {
    int no_cuenta;
    char nombre[75];
    float saldo;
};

struct Usuario *usuarios = NULL; // Arreglo dinámico para almacenar usuarios
int usuarios_size = 0;
int usuarios_capacity = 0;

int total_depositos = 0;
int total_retiros = 0;
int total_transferencias = 0;

pthread_mutex_t mutexUsuarios = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexErrores = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexOperaciones = PTHREAD_MUTEX_INITIALIZER;

// Función principal
int main() {
    int opcionPrincipal, opcionOperaciones, opcionReportes;

    do {
        printf("\nMenu Principal:\n");
        printf("1. Carga Usuarios\n");
        printf("2. Operaciones\n");
        printf("3. Carga Operaciones\n");
        printf("4. Reportes\n");
        printf("5. Salir\n");
        printf("Seleccione una opcion: ");
        scanf("%d", &opcionPrincipal);

        switch (opcionPrincipal) {
            case 1:
                cargarUsuarios();
                break;
            case 2:
                do {
                    printf("\nMenu Operaciones:\n");
                    printf("1. Deposito\n");
                    printf("2. Retiro\n");
                    printf("3. Transaccion\n");
                    printf("4. Consultar Cuenta\n");
                    printf("5. Volver al Menu Principal\n");
                    printf("Seleccione una opcion: ");
                    scanf("%d", &opcionOperaciones);

                    switch (opcionOperaciones) {
                        case 1: {
                            struct Operacion op;
                            printf("Ingrese la cuenta para el deposito: ");
                            scanf("%d", &op.cuenta1);
                            printf("Ingrese el monto del deposito: ");
                            scanf("%lf", &op.monto);
                            deposito(&op);
                            break;
                        }
                        case 2: {
                            struct Operacion op;
                            printf("Ingrese la cuenta para el retiro: ");
                            scanf("%d", &op.cuenta1);
                            printf("Ingrese el monto del retiro: ");
                            scanf("%lf", &op.monto);
                            retiro(&op);
                            break;
                        }
                        case 3: {
                            struct Operacion op;
                            printf("Ingrese la cuenta de origen: ");
                            scanf("%d", &op.cuenta1);
                            printf("Ingrese la cuenta de destino: ");
                            scanf("%d", &op.cuenta2);
                            printf("Ingrese el monto de la transaccion: ");
                            scanf("%lf", &op.monto);
                            transaccion(&op);
                            break;
                        }
                        case 4:
                            consultarCuenta();
                            break;
                        case 5:
                            printf("Volviendo al Menu Principal...\n");
                            break;
                        default:
                            printf("Opcion invalida, intente de nuevo.\n");
                    }
                } while (opcionOperaciones != 5);
                break;
            case 3:
                cargarOperaciones();
                break;
            case 4:
                do {
                    printf("\nMenu Reportes:\n");
                    printf("1. Estado de Cuenta\n");
                    printf("2. Volver al Menu Principal\n");
                    printf("Seleccione una opcion: ");
                    scanf("%d", &opcionReportes);

                    switch (opcionReportes) {
                        case 1:
                            ReporteEstadoDeCuenta();
                            break;
                        case 2:
                            printf("Volviendo al Menu Principal...\n");
                            break;
                        default:
                            printf("Opcion invalida, intente de nuevo.\n");
                    }
                } while (opcionReportes != 2);
                break;
            case 5:
                printf("Saliendo del programa...\n");
                break;
            default:
                printf("Opcion invalida, intente de nuevo.\n");
        }
    } while (opcionPrincipal != 5);

    return 0;
}

// Implementaciones de funciones

void cargarUsuarios() {

    pthread_t hilos[3];
    struct thread_data datos[3];
    int errores[3] = {0, 0, 0};

    // Inicializar datos de hilos
    for (int i = 0; i < 3; i++) {
        datos[i].filename = "usuarios.json";
        datos[i].hilo_id = i + 1;
        datos[i].operaciones_realizadas = 0;
        datos[i].errores = &errores[i];
        datos[i].errores_size = 0;
        pthread_create(&hilos[i], NULL, cargarUsuariosEnHilo, (void*)&datos[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < 3; i++) {
        pthread_join(hilos[i], NULL);
    }

    generarReporteUsuarios(datos, errores, 3);

    printf("\n");
    printf("Usuarios cargados.\n");
}

void generarReporteUsuarios(struct thread_data* datos, int* errores, int num_hilos) {
    // Obtener la fecha y hora actuales
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char fecha[100];
    strftime(fecha, sizeof(fecha)-1, "%Y-%m-%d %H:%M:%S", t);

    // Generar nombre de archivo de reporte
    char filename[100];
    strftime(filename, sizeof(filename)-1, "carga_%Y_%m_%d-%H_%M_%S.log", t);

    // Abrir archivo de reporte
    FILE *reporte = fopen(filename, "w");
    if(!reporte) {
        perror("Open Report File");
        return;
    }

    // Escribir encabezado del reporte
    fprintf(reporte, "--- Carga de usuarios ----\n");
    fprintf(reporte, "Fecha: %s\n", fecha);

    int total_cargados = 0;
    for(int i = 0; i < num_hilos; i++) {
        fprintf(reporte, "Usuarios cargados por hilo %d: %d\n", datos[i].hilo_id, datos[i].operaciones_realizadas);
        total_cargados += datos[i].operaciones_realizadas;
    }

    fprintf(reporte, "Total de usuarios cargados: %d\n", total_cargados);

    // Escribir errores
    fprintf(reporte, "\n--- errores encontrados ---\n");
    for(int i = 0; i < num_hilos; i++) {
        fprintf(reporte, "Hilo %d: %d errores\n", datos[i].hilo_id, errores[i]);
    }

    // Cerrar archivo de reporte
    fclose(reporte);
    printf("\n");
    printf("Reporte de carga de usuarios generado.\n");
}

void* cargarUsuariosEnHilo(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    read_json_fileUsers(data->filename, data);
    return NULL;
}

void read_json_fileUsers(char *filename, struct thread_data* data) {
    // Abre el archivo JSON para lectura
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Open File");
        return;
    }

    // Obtén el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Lee todo el archivo en un buffer
    char *buffer = (char*)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';

    // Cierra el archivo
    fclose(file);

    // Parsea los datos JSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        perror("Parse JSON");
        free(buffer);
        return;
    }

    // Verifica si es un arreglo JSON válido
    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        cJSON_Delete(json);
        free(buffer);
        return;
    }

    int n = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
            if (cJSON_IsObject(item)) {
            cJSON *no_cuenta = cJSON_GetObjectItem(item, "no_cuenta");
            cJSON *nombre = cJSON_GetObjectItem(item, "nombre");
            cJSON *saldo = cJSON_GetObjectItem(item, "saldo");

            // Validar datos
            if (!cJSON_IsNumber(no_cuenta) || no_cuenta->valueint <= 0 ||
                !cJSON_IsString(nombre) || !cJSON_IsNumber(saldo) || saldo->valuedouble < 0) {
                pthread_mutex_lock(&mutexErrores);
                (*data->errores)++;
                pthread_mutex_unlock(&mutexErrores);
                continue;
            }

            // Validar duplicados
            pthread_mutex_lock(&mutexUsuarios);
            int cuenta_duplicada = 0;
            for (int i = 0; i < usuarios_size; i++) {
                if (usuarios[i].no_cuenta == no_cuenta->valueint) {
                    cuenta_duplicada = 1;
                    break;
                }
            }

            if (cuenta_duplicada) {
                (*data->errores)++;
                pthread_mutex_unlock(&mutexUsuarios);
                continue;
            }

            // Asegurarse de tener suficiente espacio en el arreglo dinámico
            if (usuarios_size >= usuarios_capacity) {
                // Aumentar la capacidad del arreglo (por ejemplo, duplicar)
                int new_capacity = (usuarios_capacity == 0) ? 1 : usuarios_capacity * 2;
                struct Usuario *new_array = realloc(usuarios, new_capacity * sizeof(struct Usuario));
                if (!new_array) {
                    perror("Memory Allocation Error");
                    pthread_mutex_unlock(&mutexUsuarios);
                    cJSON_Delete(json);
                    free(buffer);
                    return;
                }
                usuarios = new_array;
                usuarios_capacity = new_capacity;
            }

            // Agregar usuario al arreglo
            usuarios[usuarios_size].no_cuenta = no_cuenta->valueint;
            strncpy(usuarios[usuarios_size].nombre, nombre->valuestring, sizeof(usuarios[usuarios_size].nombre) - 1);
            usuarios[usuarios_size].nombre[sizeof(usuarios[usuarios_size].nombre) - 1] = '\0'; // Asegura que el string esté terminado
            usuarios[usuarios_size].saldo = saldo->valuedouble;
            usuarios_size++;

            pthread_mutex_unlock(&mutexUsuarios);

            n++;
        }
    }

    data->operaciones_realizadas = n;

    // Limpiar
    cJSON_Delete(json);
    free(buffer);
}

void deposito(struct Operacion* op) {
    pthread_mutex_lock(&mutexUsuarios);
    for (int i = 0; i < usuarios_size; i++) {
        if (usuarios[i].no_cuenta == op->cuenta1) {
            usuarios[i].saldo += op->monto;
            pthread_mutex_unlock(&mutexUsuarios);
            pthread_mutex_lock(&mutexOperaciones);
            total_depositos++;
            pthread_mutex_unlock(&mutexOperaciones);
            printf("Deposito realizado a la cuenta %d por un monto de %.2f\n", op->cuenta1, op->monto);
            return;
        }
    }
    pthread_mutex_unlock(&mutexUsuarios);
    printf("Cuenta no encontrada: %d\n", op->cuenta1);
}

void retiro(struct Operacion* op) {
    pthread_mutex_lock(&mutexUsuarios);
    for (int i = 0; i < usuarios_size; i++) {
        if (usuarios[i].no_cuenta == op->cuenta1) {
            if (usuarios[i].saldo >= op->monto) {
                usuarios[i].saldo -= op->monto;
                pthread_mutex_unlock(&mutexUsuarios);
                pthread_mutex_lock(&mutexOperaciones);
                total_retiros++;
                pthread_mutex_unlock(&mutexOperaciones);
                printf("Retiro realizado de la cuenta %d por un monto de %.2f\n", op->cuenta1, op->monto);
                return;
            } else {
                pthread_mutex_unlock(&mutexUsuarios);
                printf("Fondos insuficientes en la cuenta %d\n", op->cuenta1);
                return;
            }
        }
    }
    pthread_mutex_unlock(&mutexUsuarios);
    printf("Cuenta no encontrada: %d\n", op->cuenta1);
}

void transaccion(struct Operacion* op) {
    pthread_mutex_lock(&mutexUsuarios);
    int cuenta1_encontrada = 0, cuenta2_encontrada = 0;
    struct Usuario* cuenta1 = NULL;
    struct Usuario* cuenta2 = NULL;
    for (int i = 0; i < usuarios_size; i++) {
        if (usuarios[i].no_cuenta == op->cuenta1) {
            cuenta1 = &usuarios[i];
            cuenta1_encontrada = 1;
        }
        if (usuarios[i].no_cuenta == op->cuenta2) {
            cuenta2 = &usuarios[i];
            cuenta2_encontrada = 1;
        }
        if (cuenta1_encontrada && cuenta2_encontrada) {
            break;
        }
    }

    if (cuenta1_encontrada && cuenta2_encontrada) {
        if (cuenta1->saldo >= op->monto) {
            cuenta1->saldo -= op->monto;
            cuenta2->saldo += op->monto;
            pthread_mutex_unlock(&mutexUsuarios);
            pthread_mutex_lock(&mutexOperaciones);
            total_transferencias++;
            pthread_mutex_unlock(&mutexOperaciones);
            printf("Transaccion realizada de la cuenta %d a la cuenta %d por un monto de %.2f\n", op->cuenta1, op->cuenta2, op->monto);
            return;
        } else {
            pthread_mutex_unlock(&mutexUsuarios);
            printf("Fondos insuficientes en la cuenta %d\n", op->cuenta1);
            return;
        }
    }
    pthread_mutex_unlock(&mutexUsuarios);
    if (!cuenta1_encontrada) printf("Cuenta no encontrada: %d\n", op->cuenta1);
    if (!cuenta2_encontrada) printf("Cuenta no encontrada: %d\n", op->cuenta2);
}

void consultarCuenta() {
    int no_cuenta;
    printf("Ingrese el numero de cuenta: ");
    scanf("%d", &no_cuenta);
    
    pthread_mutex_lock(&mutexUsuarios);
    for (int i = 0; i < usuarios_size; i++) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            pthread_mutex_unlock(&mutexUsuarios);
            printf("Numero de cuenta: %d\n", usuarios[i].no_cuenta);
            printf("Nombre: %s\n", usuarios[i].nombre);
            printf("Saldo: %.2f\n", usuarios[i].saldo);
            return;
        }
    }
    pthread_mutex_unlock(&mutexUsuarios);
    printf("Cuenta no encontrada: %d\n", no_cuenta);
}

void cargarOperaciones() {
    pthread_t hilos[4];
    struct thread_data datos[4];
    int errores[4] = {0, 0, 0, 0};

    // Inicializar datos de hilos
    for (int i = 0; i < 4; i++) {
        datos[i].filename = "operaciones.json";
        datos[i].hilo_id = i + 1;
        datos[i].operaciones_realizadas = 0;
        datos[i].errores = &errores[i];
        datos[i].errores_size = 0;
        pthread_create(&hilos[i], NULL, cargarOperacionesEnHilo, (void*)&datos[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < 4; i++) {
        pthread_join(hilos[i], NULL);
    }

    generarReporteOperaciones(datos, errores, 4);

    printf("\n");
    printf("Operaciones cargadas.\n");
}

void read_json_fileOperaciones(char *filename, struct thread_data* data) {
    // Abre el archivo JSON para lectura
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Open File");
        return;
    }

    // Obtén el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Lee todo el archivo en un buffer
    char *buffer = (char*)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';

    // Cierra el archivo
    fclose(file);

    // Parsea los datos JSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        perror("Parse JSON");
        free(buffer);
        return;
    }

    // Verifica si es un arreglo JSON válido
    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        cJSON_Delete(json);
        free(buffer);
        return;
    }

    int n = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (cJSON_IsObject(item)) {
                        cJSON *operacion = cJSON_GetObjectItem(item, "operacion");
            cJSON *cuenta1 = cJSON_GetObjectItem(item, "cuenta1");
            cJSON *cuenta2 = cJSON_GetObjectItem(item, "cuenta2");
            cJSON *monto = cJSON_GetObjectItem(item, "monto");

            // Validar datos
            if (!cJSON_IsNumber(operacion) || !cJSON_IsNumber(cuenta1) ||
                (operacion->valueint == 3 && !cJSON_IsNumber(cuenta2)) ||
                !cJSON_IsNumber(monto) || monto->valuedouble < 0) {
                pthread_mutex_lock(&mutexErrores);
                (*data->errores)++;
                pthread_mutex_unlock(&mutexErrores);
                continue;
            }

            struct Operacion op;
            op.operacion = operacion->valueint;
            op.cuenta1 = cuenta1->valueint;
            op.cuenta2 = (cuenta2 != NULL) ? cuenta2->valueint : -1;
            op.monto = monto->valuedouble;

            // Ejecutar operación
            switch (op.operacion) {
                case 1:
                    deposito(&op);
                    break;
                case 2:
                    retiro(&op);
                    break;
                case 3:
                    transaccion(&op);
                    break;
                default:
                    pthread_mutex_lock(&mutexErrores);
                    (*data->errores)++;
                    pthread_mutex_unlock(&mutexErrores);
                    continue;
            }

            n++;
        }
    }

    data->operaciones_realizadas = n;

    // Limpiar
    cJSON_Delete(json);
    free(buffer);
}

void generarReporteOperaciones(struct thread_data* datos, int* errores, int num_hilos) {
    // Obtener la fecha y hora actuales
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char fecha[100];
    strftime(fecha, sizeof(fecha)-1, "%Y-%m-%d %H:%M:%S", t);

    // Generar nombre de archivo de reporte
    char filename[100];
    strftime(filename, sizeof(filename)-1, "operaciones_%Y_%m_%d-%H_%M_%S.log", t);

    // Abrir archivo de reporte
    FILE *reporte = fopen(filename, "w");
    if(!reporte) {
        perror("Open Report File");
        return;
    }

    // Escribir encabezado del reporte
    fprintf(reporte, "--- Carga de operaciones ----\n");
    fprintf(reporte, "Fecha: %s\n", fecha);

    int total_operaciones = 0;
    for(int i = 0; i < num_hilos; i++) {
        fprintf(reporte, "Operaciones realizadas por hilo %d: %d\n", datos[i].hilo_id, datos[i].operaciones_realizadas);
        total_operaciones += datos[i].operaciones_realizadas;
    }

    fprintf(reporte, "Total de operaciones realizadas: %d\n", total_operaciones);

    // Escribir errores
    fprintf(reporte, "\n--- errores encontrados ---\n");
    for(int i = 0; i < num_hilos; i++) {
        fprintf(reporte, "Hilo %d: %d errores\n", datos[i].hilo_id, errores[i]);
    }

    // Cerrar archivo de reporte
    fclose(reporte);

    printf("\n");
    printf("Reporte de operaciones generado.\n");
}

void estadoDeCuenta() {
    //NO GENERA REPORTE

    int no_cuenta;
    printf("Ingrese el numero de cuenta: ");
    scanf("%d", &no_cuenta);

    pthread_mutex_lock(&mutexUsuarios);
    for (int i = 0; i < usuarios_size; i++) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            pthread_mutex_unlock(&mutexUsuarios);
            printf("Numero de cuenta: %d\n", usuarios[i].no_cuenta);
            printf("Nombre: %s\n", usuarios[i].nombre);
            printf("Saldo: %.2f\n", usuarios[i].saldo);
            return;
        }
    }
    pthread_mutex_unlock(&mutexUsuarios);
    printf("Cuenta no encontrada: %d\n", no_cuenta);
}


void ReporteEstadoDeCuenta() {
    cJSON *root = cJSON_CreateArray(); // Crear un arreglo JSON

    // Recorrer el arreglo de usuarios y agregar cada usuario al arreglo JSON
    for (int i = 0; i < usuarios_size; ++i) {
        cJSON *usuario = cJSON_CreateObject(); // Crear un objeto JSON para cada usuario

        // Agregar campos al objeto JSON
        cJSON_AddItemToObject(usuario, "no_cuenta", cJSON_CreateNumber(usuarios[i].no_cuenta));
        cJSON_AddItemToObject(usuario, "nombre", cJSON_CreateString(usuarios[i].nombre));
        cJSON_AddItemToObject(usuario, "saldo", cJSON_CreateNumber(usuarios[i].saldo));

        // Agregar el objeto usuario al arreglo root
        cJSON_AddItemToArray(root, usuario);
    }

    // Convertir el objeto cJSON a una cadena JSON formateada
    char *json_str = cJSON_Print(root);

    // Crear y abrir el archivo para escritura
    FILE *reporte_file = fopen("reporte_estadodecuenta.json", "w");
    if (reporte_file == NULL) {
        perror("Error al abrir el archivo de reporte");
        cJSON_Delete(root);
        free(json_str);
        return;
    }

    // Escribir la cadena JSON en el archivo
    fprintf(reporte_file, "%s\n", json_str);

    // Cerrar el archivo y liberar recursos
    fclose(reporte_file);
    cJSON_Delete(root);
    free(json_str);

    printf("\n");
    printf("Reporte de estado de cuenta generado.\n");
}

void* cargarOperacionesEnHilo(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    read_json_fileOperaciones(data->filename, data);
    return NULL;
}


