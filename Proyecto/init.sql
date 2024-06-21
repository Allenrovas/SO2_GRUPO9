-- Crear la base de datos dbso2
CREATE DATABASE dbso2;

-- Usar la base de datos dbso2
USE dbso2;

-- Crear la tabla llamada 'process_logs' con los atributos especificados
CREATE TABLE procesos (
    pid INT NOT NULL,
    nombre VARCHAR(255) NOT NULL,
    llamada VARCHAR(255) NOT NULL,
    tamanio_memoria INT NOT NULL,
    fechahora DATETIME NOT NULL
    );