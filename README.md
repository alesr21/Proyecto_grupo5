# Proyecto_grupo5

Descripción:

El objetivo principal de este proyecto consiste en desarrollar un sistema sencillo para gestionar un inventario enfocado en pequeños negocios en lenguaje C. Este proyecto permite agregar, modificar y eliminar productos del inventario, y de manera simultánea generar facturas con cálculos automáticos correspondientes en formato PDF.

Este proyecto representa una solución eficiente y práctica para pequeños negocios o comercios que necesitan llevar un control básico de inventario sin depender de herramientas externas complejas. La elección de archivos CSV permite que la información sea accesible y portátil, lo que facilita su apertura en programas como Excel u otros editores de texto.

Para usar este proyecto necesitas tener instalado lo siguiente en tu computadora:

1. Un compilador C (como GCC):
Verifica si lo tienes instalado ejecutando: 
gcc --version
sudo apt update && sudo apt install build-essential

Si no, instálalo con este comando:
sudo apt update && sudo apt install build-essential

2. SQLITE
Verifica si lo tienes instalado ejecutando:
sqlite3 --version
Si no lo tienes, instálalo con este comando:
sudo apt update && sudo apt install sqlite3 libsqlite3-dev

3. Un editor de texto:
Puede ser  Vim, Nano o cualquier otro.

INSTALACIÓN:
1. Ingrese al repositorio en el siguiente link https://github.com/alesr21/Proyecto_grupo5.git

Dirijase a la rama proyecto_grupo05

Descargue el zip con los archivos dando en el botón "code" y luego en "dowload zip"

Con el zip descargado proceda a extraer los archivos del zip y continue con el proceso

2. Descarga la base de datos del programa o crea una:
a. Abre SQLite escribiendo en la terminal:
sqlite3 Inventario.db
b. Escribe el siguiente comando para crear la tabla de productos:
sql
Copiar código
CREATE TABLE productos (
    Producto TEXT,
    Cantidad INTEGER,
    Código INTEGER,
    Precio REAL
);
c. .separator ";"
d. .mode csv
e. .import Inventario.csv productos
c. Sal de SQLite escribiendo .exit.

EJECUTAR EL PROGRAMA:
Ejecuta en la terminal:
gcc proyecto_pp.c -o proyecto_pp -lsqlite3
Para correr el programa:
./proyecto_pp


