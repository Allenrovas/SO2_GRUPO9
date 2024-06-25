# PRACTICA 2 - Sistemas Operativos 2 - Grupo 9

**Luis Manuel Chay Marrroquin - 202000343**

**Allen Giankarlo Roman Vasquz - 202004745**

## Variables globales

Estas variables nos ayudan al funcionamiento del manejo de informacion y de hilos
    
```c
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
```


## Carga masiva de usuarios

Para que el sistema del banco funcione se realiza una carga masiva de usuarios a través de un archivo JSON con la siguiente sintaxis:

```json
[
    {
        "no_cuenta": 691232,
        "nombre": "Helaina Dumberell",
        "saldo": 649807.72
    },
    {
        "no_cuenta": 547707,
        "nombre": "Joelynn Astles",
        "saldo": 191539.84
    }
]
```

Para realizar este proceso se realizo haciendo uso de 3 hilos en forma paralela y se capturo registros con error para un reporte posterior

```c
void* cargarUsuariosEnHilo(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    read_json_fileUsers(data->json_part, data);
    return NULL;
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
```

## Operaciones individuales

La aplicacion posee un menu donde se permite al usuario realizar una operacion, estas son:

- Deposito: Se realiza un deposito a una cuenta especifica
- Retiro: Se realiza un retiro a una cuenta especifica
- Consulta: Se consulta el saldo de una cuenta especifica
- Transaccion: Se realiza una transferencia de una cuenta a otra

```c
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
```

## Carga masiva de operaciones

Es posible realzar más de una operacion a la vez gracias a las cargas masivas de operaciones, que se hacena  traves de un archivo JSON con la siguiente sintaxis:

```json
[
    {
        "operacion": 1,
        "cuenta1": 691232,
        "cuenta2": 0,
        "monto": 613588.14
    },
    {
        "operacion": 1,
        "cuenta1": 547707,
        "cuenta2": 0,
        "monto": 750040.81
    }
]
```
Esta operacion se hace por medio de 4 hilos de manera concurrente

```c
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
```

## Reportes

Se generan reportes de las operaciones realizadas y de los errores que se presentaron en la carga masiva de usuarios y operaciones

### Reporte estado de cuentas

Muestra la información de los usuarios, este
será escrito en un JSON y deberá mostrar la siguiente información:

```json
[
    {
        "no_cuenta": 691232,
        "nombre": "Helaina Dumberell",
        "saldo": 649807.72
    },
    {
        "no_cuenta": 547707,
        "nombre": "Joelynn Astles",
        "saldo": 191539.84
    }
]
```

```c
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
```

### Reporte de operaciones

Estos reportes se realizan automaticamente cuando se leen el archivo, este reporte contiene informacion de la cantidad de operaciones realizadas por hilo y los errores que se presentaron en la carga masiva de operaciones, tienen el siguiente formato

```txt
--- Carga de operaciones ----
Fecha: 2024-06-16 23:09:25
Operaciones realizadas por hilo 1: 5
Operaciones realizadas por hilo 2: 5
Operaciones realizadas por hilo 3: 5
Operaciones realizadas por hilo 4: 2
Total de operaciones realizadas: 17

--- errores encontrados ---
Hilo 4: Error en línea 18: Monto no valido
Hilo 4: Error en línea 19: Cuenta1 no valida
Hilo 4: Error en línea 20: Monto no valido

```

### Reporte de usuarios

Estos reportes se realizan automaticamente cuando se leen el archivo, este reporte contiene informacion de la cantidad de operaciones realizadas por hilo y los errores que se presentaron en la carga masiva de operaciones, tienen el siguiente formato

```txt

--- Carga de usuarios ----
Fecha: 2024-06-16 23:09:18
Usuarios cargados por hilo 1: 7
Usuarios cargados por hilo 2: 7
Usuarios cargados por hilo 3: 6
Total de usuarios cargados: 19

--- errores encontrados ---
     -Linea 3: Número de cuenta duplicada
```

