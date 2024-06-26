#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <time.h>

#define BUFFER_SIZE 256

void insert_into_db(const char *pid, const char *name, const char *call, const char *addr, const char *length, const char *timestamp);

int main() {
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = popen("stap -c 'sleep 10' memory_monitor.stp", "r");
    if (fp == NULL) {
        perror("Failed to run SystemTap script");
        exit(1);
    }

    printf("SystemTap script started...\n");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char pid[16], name[64], call[8], addr[32], length[16], timestamp[32];

        // Intentar parsear para 'mmap'
        if (sscanf(buffer, "mmap: PID=%15s NAME=%63s ADDR=%31s LENGTH=%15s TIME=%31[^\n]", pid, name, addr, length, timestamp) == 5) {
            fprintf(stdout, "PID: %s\n", pid);
            insert_into_db(pid, name, "mmap", addr, length, timestamp);
        }
        // Intentar parsear para 'munmap'
        else if (sscanf(buffer, "munmap: PID=%15s NAME=%63s ADDR=%31s LENGTH=%15s TIME=%31[^\n]", pid, name, addr, length, timestamp) == 5) {
            fprintf(stdout, "PID: %s\n", pid);
            insert_into_db(pid, name, "munmap", addr, length, timestamp);
        } else {
            fprintf(stderr, "Failed to parse: %s\n", buffer);
        }
    }

    printf("SystemTap script finished.\n");

    pclose(fp);
    return 0;
}

void insert_into_db(const char *pid, const char *name, const char *call, const char *addr, const char *length, const char *timestamp) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char query[BUFFER_SIZE];
    char formatted_timestamp[64];

    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));

    // Utilizar sscanf para extraer los componentes de la fecha y hora
    int day, year, hour, min, sec;
    char month[4];

    if (sscanf(timestamp, "%*s %3s %d %d:%d:%d %d", month, &day, &hour, &min, &sec, &year) != 6) {
        fprintf(stderr, "Failed to parse timestamp: %s\n", timestamp);
        return;
    }

    // Convertir el mes de cadena a número
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int month_num = 0;
    while (strcmp(month, months[month_num]) != 0 && month_num < 12) {
        month_num++;
    }
    if (month_num == 12) {
        fprintf(stderr, "Invalid month: %s\n", month);
        return;
    }

    timeinfo.tm_mon = month_num;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;
    timeinfo.tm_year = year - 1900;

    // Formatear la fecha y hora en el formato YYYY-MM-DD HH:MM:SS
    strftime(formatted_timestamp, sizeof(formatted_timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return;
    }

    if (mysql_real_connect(conn, "34.31.162.29", "root", "adminso2", "dbso2", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return;
    }

    printf("Connected to MySQL server.\n");

    snprintf(query, sizeof(query), "INSERT INTO procesos (pid, nombre, llamada, tamanio_memoria, fechahora) VALUES ('%s', '%s', '%s', '%s', '%s')", pid, name, call, length, formatted_timestamp);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT error: %s\n", mysql_error(conn));
    } else {
        printf("Insert successful.\n");
    }

    mysql_close(conn);
}
