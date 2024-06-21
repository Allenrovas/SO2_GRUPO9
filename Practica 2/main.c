#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "cJSON.h"

struct ErrorDetail {
    int linea;
    char descripcion[100];
};

// Declaración de la estructura thread_data
struct thread_data {
    cJSON* json_part;
    char* filename;
    int hilo_id;
    int operaciones_realizadas;
    int* errores;
    int errores_size;
    struct ErrorDetail* detalle_errores;
    int num_errores;
};

// Declaración de la estructura Operacion
struct Operacion {
    int operacion;
    int cuenta1;
    int cuenta2;
    double monto;
    int linea;
};

// Prototipos de funciones
void* cargarOperacionesEnHilo(void* arg);
void generarReporteUsuarios(struct thread_data* datos, int num_hilos);
void read_json_fileUsers(cJSON* json, struct thread_data* data);
void* cargarUsuariosEnHilo(void* arg);
void cargarUsuarios(char ruta[100]);
void deposito(struct Operacion* op);
void retiro(struct Operacion* op);
void transaccion(struct Operacion* op);
void consultarCuenta();
void cargarOperaciones(char ruta[100]);
void estadoDeCuenta();
void ReporteEstadoDeCuenta();
void reporteCargarUsuarios();
void reporteCargarOperaciones();
void generarReporteOperaciones(struct thread_data* datos, int num_hilos);

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
                //cargarUsuarios();
                //Leer la ruta del archivo y llamar a la función cargarUsuarios
                char ruta[100];
                printf("Ingrese la ruta del archivo: ");
                scanf("%s", ruta);
                cargarUsuarios(ruta);
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
                //cargarOperaciones();
                //Leer la ruta del archivo y llamar a la función cargarOperaciones
                char rutaO[100];
                printf("Ingrese la ruta del archivo: ");
                scanf("%s", rutaO);
                cargarOperaciones(rutaO);
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

void* cargarUsuariosEnHilo(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    read_json_fileUsers(data->json_part, data);
    return NULL;
}

void read_json_fileUsers(cJSON* json, struct thread_data* data) {
    int linea_original = 0; // Mantener el número de línea original del archivo usuarios.json
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        linea_original++; // Incrementar la línea original antes de cualquier procesamiento
        int linea_actual = linea_original; // Guardar la línea original para el error

        if (cJSON_IsObject(item)) {
            cJSON *no_cuenta = cJSON_GetObjectItem(item, "no_cuenta");
            cJSON *nombre = cJSON_GetObjectItem(item, "nombre");
            cJSON *saldo = cJSON_GetObjectItem(item, "saldo");

            // Validar datos
            if (!cJSON_IsNumber(no_cuenta) || no_cuenta->valueint <= 0 ||
                !cJSON_IsString(nombre) || !cJSON_IsNumber(saldo) || saldo->valuedouble < 0) {
                pthread_mutex_lock(&mutexErrores);
                if (data->num_errores < 100) {
                    data->detalle_errores[data->num_errores].linea = linea_actual;
                    if (!cJSON_IsNumber(no_cuenta) || no_cuenta->valueint <= 0) {
                        snprintf(data->detalle_errores[data->num_errores].descripcion, 100, "Número de cuenta inválido");
                    } else if (!cJSON_IsString(nombre)) {
                        snprintf(data->detalle_errores[data->num_errores].descripcion, 100, "Nombre inválido");
                    } else if (!cJSON_IsNumber(saldo) || saldo->valuedouble < 0) {
                        snprintf(data->detalle_errores[data->num_errores].descripcion, 100, "Saldo no puede ser menor que 0");
                    }
                    data->num_errores++;
                }
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
                pthread_mutex_lock(&mutexErrores);
                if (data->num_errores < 100) {
                    data->detalle_errores[data->num_errores].linea = linea_actual;
                    snprintf(data->detalle_errores[data->num_errores].descripcion, 100, "Número de cuenta duplicada");
                    data->num_errores++;
                }
                pthread_mutex_unlock(&mutexErrores);
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
        }
    }


    data->operaciones_realizadas = cJSON_GetArraySize(json); // Actualizar el número de operaciones realizadas
}


void generarReporteUsuarios(struct thread_data* datos, int num_hilos) {
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
    
    int total_errores = 0;
    for(int i = 0; i < num_hilos; i++) {
        for (int j = 0; j < datos[i].num_errores; j++) {
            // Buscar el objeto JSON correspondiente al error
            cJSON* json_array = datos[i].json_part;
            cJSON* json_item = cJSON_GetArrayItem(json_array, j);
            if (json_item != NULL) {
                cJSON* linea_original = cJSON_GetObjectItem(json_item, "linea_original");
                if (linea_original != NULL && cJSON_IsNumber(linea_original)) {
                    total_errores++;
                }
            }
        }
    }
    // Escribir total de usuarios cargados - la cantidad de errores
    fprintf(reporte, "Total de usuarios cargados: %d\n", total_cargados - total_errores);

    fprintf(reporte, "\n--- errores encontrados ---\n");
    for(int i = 0; i < num_hilos; i++) {
        for (int j = 0; j < datos[i].num_errores; j++) {
            // Buscar el objeto JSON correspondiente al error
            cJSON* json_array = datos[i].json_part;
            cJSON* json_item = cJSON_GetArrayItem(json_array, j);
            if (json_item != NULL) {
                cJSON* linea_original = cJSON_GetObjectItem(json_item, "linea_original");
                if (linea_original != NULL && cJSON_IsNumber(linea_original)) {
                    fprintf(reporte, "     -Linea %d: %s\n", linea_original->valueint, datos[i].detalle_errores[j].descripcion);
                }
            }
        }
    }

    // Cerrar archivo de reporte
    fclose(reporte);
    printf("\n");
    printf("Reporte de carga de usuarios generado.\n");
}


void cargarUsuarios(char ruta[100]) {
    // Abrir y leer el archivo JSON
    FILE *file = fopen(ruta, "r");
    if (!file) {
        perror("Open File");
        return;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';

    fclose(file);

    cJSON *json = cJSON_Parse(buffer);
    free(buffer); // Liberar buffer después de parsear JSON
    if (json == NULL) {
        perror("Parse JSON");
        return;
    }

    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        cJSON_Delete(json);
        return;
    }

    // Agregar atributo 'linea_original' a cada objeto del JSON
    int linea_original = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        cJSON_AddNumberToObject(item, "linea_original", ++linea_original);
    }

    // Dividir el JSON en 3 partes
    int total_elements = cJSON_GetArraySize(json);
    int elements_per_thread = total_elements / 3;

    pthread_t hilos[3];
    struct thread_data datos[3];
    struct ErrorDetail errores[3][100]; // Suponiendo un máximo de 100 errores por hilo

    cJSON* json_parts[3] = { cJSON_CreateArray(), cJSON_CreateArray(), cJSON_CreateArray() };

    for (int i = 0; i < total_elements; i++) {
        cJSON* item = cJSON_DetachItemFromArray(json, 0);
        cJSON_AddItemToArray(json_parts[i % 3], item);
    }

    // Inicializar datos de hilos
    for (int i = 0; i < 3; i++) {
        datos[i].json_part = json_parts[i];
        datos[i].hilo_id = i + 1;
        datos[i].operaciones_realizadas = 0;
        datos[i].detalle_errores = errores[i];
        datos[i].num_errores = 0;
        pthread_create(&hilos[i], NULL, cargarUsuariosEnHilo, (void*)&datos[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < 3; i++) {
        pthread_join(hilos[i], NULL);
    }

    // Generar reporte con datos y errores
    generarReporteUsuarios(datos, 3);

    cJSON_Delete(json); // Eliminar el JSON original
    cJSON_Delete(json_parts[0]);
    cJSON_Delete(json_parts[1]);
    cJSON_Delete(json_parts[2]);

    printf("\n");
    printf("Usuarios cargados.\n");
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

void* cargarOperacionesEnHilo(void* threadarg) {
    struct thread_data* data = (struct thread_data*)threadarg;
    int n = 0;

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, data->json_part) {
        struct Operacion op;
        cJSON* operacion = cJSON_GetObjectItem(item, "operacion");
        cJSON* cuenta1 = cJSON_GetObjectItem(item, "cuenta1");
        cJSON* cuenta2 = cJSON_GetObjectItem(item, "cuenta2");
        cJSON* monto = cJSON_GetObjectItem(item, "monto");
        cJSON* linea = cJSON_GetObjectItem(item, "linea_original");

        // Validar datos
        if (!cJSON_IsNumber(operacion)) {
            pthread_mutex_lock(&mutexErrores);
            data->detalle_errores[data->num_errores].linea = linea->valueint;
            strcpy(data->detalle_errores[data->num_errores].descripcion, "Operacion no valida");
            data->num_errores++;
            pthread_mutex_unlock(&mutexErrores);
            continue;
        }
        if (!cJSON_IsNumber(cuenta1)) {
            pthread_mutex_lock(&mutexErrores);
            data->detalle_errores[data->num_errores].linea = linea->valueint;
            strcpy(data->detalle_errores[data->num_errores].descripcion, "Cuenta1 no valida");
            data->num_errores++;
            pthread_mutex_unlock(&mutexErrores);
            continue;
        }
        if (operacion->valueint == 3 && !cJSON_IsNumber(cuenta2)) {
            pthread_mutex_lock(&mutexErrores);
            data->detalle_errores[data->num_errores].linea = linea->valueint;
            strcpy(data->detalle_errores[data->num_errores].descripcion, "Cuenta2 no valida");
            data->num_errores++;
            pthread_mutex_unlock(&mutexErrores);
            continue;
        }
        if (!cJSON_IsNumber(monto) || monto->valuedouble < 0) {
            pthread_mutex_lock(&mutexErrores);
            data->detalle_errores[data->num_errores].linea = linea->valueint;
            strcpy(data->detalle_errores[data->num_errores].descripcion, "Monto no valido");
            data->num_errores++;
            pthread_mutex_unlock(&mutexErrores);
            continue;
        }

        op.operacion = operacion->valueint;
        op.cuenta1 = cuenta1->valueint;
        op.cuenta2 = (cuenta2 != NULL) ? cuenta2->valueint : -1;
        op.monto = monto->valuedouble;
        op.linea = linea->valueint;

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
                data->detalle_errores[data->num_errores].linea = linea->valueint;
                strcpy(data->detalle_errores[data->num_errores].descripcion, "Operacion no valida");
                data->num_errores++;
                pthread_mutex_unlock(&mutexErrores);
                continue;
        }

        n++;
    }

    data->operaciones_realizadas = n;
    pthread_exit(NULL);
}

void cargarOperaciones(char ruta[100]) {
    // Abrir y leer el archivo JSON
    FILE *file = fopen(ruta, "r");
    if (!file) {
        perror("Open File");
        return;
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';

    fclose(file);

    cJSON *json = cJSON_Parse(buffer);
    free(buffer); // Liberar buffer después de parsear JSON
    if (json == NULL) {
        perror("Parse JSON");
        return;
    }

    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        cJSON_Delete(json);
        return;
    }

    // Agregar atributo 'linea_original' a cada objeto del JSON
    int linea_original = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json) {
        cJSON_AddNumberToObject(item, "linea_original", ++linea_original);
    }

    // Dividir el JSON en 4 partes
    int total_elements = cJSON_GetArraySize(json);
    int elements_per_thread = total_elements / 4;

    pthread_t hilos[4];
    struct thread_data datos[4];
    cJSON* json_parts[4] = { cJSON_CreateArray(), cJSON_CreateArray(), cJSON_CreateArray(), cJSON_CreateArray() };

    for (int i = 0; i < total_elements; i++) {
        int index = i / elements_per_thread;
        if (index >= 4) {
            index = 3;
        }
        cJSON* item = cJSON_DetachItemFromArray(json, 0);
        cJSON_AddItemToArray(json_parts[index], item);
    }

    // Inicializar datos de hilos y crear hilos
    for (int i = 0; i < 4; i++) {
        datos[i].json_part = json_parts[i];
        datos[i].hilo_id = i + 1;
        datos[i].operaciones_realizadas = 0;
        datos[i].errores = (int*)malloc(100 * sizeof(int)); // Suponiendo un máximo de 100 errores
        datos[i].detalle_errores = (struct ErrorDetail*)malloc(100 * sizeof(struct ErrorDetail));
        datos[i].num_errores = 0;
        pthread_create(&hilos[i], NULL, cargarOperacionesEnHilo, (void*)&datos[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < 4; i++) {
        pthread_join(hilos[i], NULL);
    }

    generarReporteOperaciones(datos, 4);

    cJSON_Delete(json); // Eliminar el JSON original
    for (int i = 0; i < 4; i++) {
        cJSON_Delete(json_parts[i]);
        free(datos[i].errores);
        free(datos[i].detalle_errores);
    }

    printf("\n");
    printf("Operaciones cargadas.\n");
}

void generarReporteOperaciones(struct thread_data* datos, int num_hilos) {
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
    if (!reporte) {
        perror("Open Report File");
        return;
    }

    // Escribir encabezado del reporte
    fprintf(reporte, "--- Carga de operaciones ----\n");
    fprintf(reporte, "Fecha: %s\n", fecha);

    int total_operaciones = 0;
    for (int i = 0; i < num_hilos; i++) {
        fprintf(reporte, "Operaciones realizadas por hilo %d: %d\n", datos[i].hilo_id, datos[i].operaciones_realizadas);
        total_operaciones += datos[i].operaciones_realizadas;
    }

    fprintf(reporte, "Total de operaciones realizadas: %d\n", total_operaciones);

    // Escribir errores
    fprintf(reporte, "\n--- errores encontrados ---\n");
    for (int i = 0; i < num_hilos; i++) {
        for (int j = 0; j < datos[i].num_errores; j++) {
            fprintf(reporte, "Hilo %d: Error en línea %d: %s\n", datos[i].hilo_id, datos[i].detalle_errores[j].linea, datos[i].detalle_errores[j].descripcion);
        }
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