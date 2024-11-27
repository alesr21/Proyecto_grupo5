#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Definición de los colores ANSI para mejorar la visualización en la terminal
#define RED_COLOR   "\x1b[31m"   // Rojo
#define GREEN_COLOR "\x1b[32m"   // Verde
#define YELLOW_COLOR "\x1b[33m"  // Amarillo
#define BLUE_COLOR  "\x1b[34m"   // Azul
#define WHITE_COLOR "\x1b[37m" // Blanco
#define CYAN_COLOR  "\x1b[36m"   // Cian
#define RESET_COLOR "\x1b[0m"    

// Prototipos de funciones, se indican parámetros necesarios y tipo de dato para estos
void mostrar_productos_disponibles(); 
void realizar_pedido(sqlite3 *db);    
void editar_inventario();             
void generar_factura(sqlite3 *db, const char *identificador);
void guardar_en_historial(const char *identificador, const char *producto, int cantidad, int codigo, double precio);
void mostrar_historial_usuario(const char *identificador);
int comparar_facturas(const void *a, const void *b);


// Estructura para manejar artículos en el pedido
typedef struct {
    char producto[50];
    int cantidad;
} CarritoItem;

// Lista única para manejar los artículos del pedido
#define MAX_CARRITO 100
CarritoItem carrito[MAX_CARRITO];
int carrito_size = 0;



// Función 1: para mostrar los productos disponibles (Luis)
void mostrar_productos_disponibles() {
    sqlite3 *db;
    int rc;

// Apertura de la base de datos y revisión de la misma con la biblioteca SQLite
    rc = sqlite3_open("Inventario.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%sNo se pudo abrir la base de datos: %s\n%s", RED_COLOR, sqlite3_errmsg(db), RESET_COLOR);
        return;
    }

    const char *sql = "SELECT Producto, Cantidad, Código, Precio FROM productos;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "%sNo se pudo preparar la consulta: %s\n%s", RED_COLOR, sqlite3_errmsg(db), RESET_COLOR);
        sqlite3_close(db);
        return;
    }

    // Títulos de las columnas en amarillo
    printf("%s%-20s %-10s %-10s %-10s\n%s", YELLOW_COLOR, "Producto", "Cantidad", "Código", "Precio", RESET_COLOR);
    printf("--------------------------------------------------------\n");

    // Mostrar los productos con un color alternado para una mejor visualización
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *producto = (const char *)sqlite3_column_text(stmt, 0);
        int cantidad = sqlite3_column_int(stmt, 1);
        int codigo = sqlite3_column_int(stmt, 2);
        double precio = sqlite3_column_double(stmt, 3);

        // Productos con cantidad mayor a 0 en verde, los agotados en rojo
        if (cantidad > 0) {
            printf("%s%-20s %-10d %-10d %-10.2f\n%s", GREEN_COLOR, producto, cantidad, codigo, precio, RESET_COLOR);
        } else {
            printf("%s%-20s %-10d %-10d %-10.2f\n%s", RED_COLOR, producto, cantidad, codigo, precio, RESET_COLOR);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


// Fin función 1

// Función 2: realizar pedido

//Se definiran las funciones y se desarrolla el funcionamiento para que luego sean llamadas en el menú de está opción.

// Agregar un artículo al carrito, aca se agregan los productos al carrito (lista) para iniciar a efectuar la compra
void agregar_a_carrito(const char *producto, int cantidad) {
    if (carrito_size < MAX_CARRITO) {
        strncpy(carrito[carrito_size].producto, producto, sizeof(carrito[carrito_size].producto) - 1);
        carrito[carrito_size].cantidad = cantidad;
        carrito_size++;

        // Mensaje de éxito en verde
        printf(GREEN_COLOR "Producto '%s' agregado al carrito. Cantidad: %d\n" RESET_COLOR, producto, cantidad);
    } else {
        // Mensaje de error en rojo si el carrito está lleno
        printf(RED_COLOR "Error: No se pueden agregar más productos. Carrito lleno.\n" RESET_COLOR);
    }
}

// Mostrar los artículos en el carrito, presenta la lista de lo que se ha seleccionado hasta ese momento 
void mostrar_carrito() {
    if (carrito_size == 0) {
        // Mensaje en rojo si el carrito está vacío
        printf(RED_COLOR "El carrito está vacío.\n" RESET_COLOR);
        return;
    }

    // Título en azul
    printf(BLUE_COLOR "\nArtículos en el carrito:\n" RESET_COLOR);
    printf("--------------------------------------\n");

    // Encabezados de la tabla en amarillo
    printf(YELLOW_COLOR "%-20s %-10s\n" RESET_COLOR, "Producto", "Cantidad");
    printf("--------------------------------------\n");

    // Mostrar los productos en verde
    for (int i = 0; i < carrito_size; i++) {
        printf(GREEN_COLOR "%-20s %-10d\n" RESET_COLOR, carrito[i].producto, carrito[i].cantidad);
    }
}


// Actualizar inventario al finalizar el pedido
void actualizar_inventario(sqlite3 *db) {
    for (int i = 0; i < carrito_size; i++) {
        // Reducir la cantidad del producto en la base de datos
        char sql[256];
        snprintf(sql, sizeof(sql), "UPDATE productos SET Cantidad = Cantidad - %d WHERE Producto = '%s';",
                 carrito[i].cantidad, carrito[i].producto);

        int rc = sqlite3_exec(db, sql, 0, 0, 0);
        if (rc != SQLITE_OK) {
            // Mensaje de error en rojo si no se pudo actualizar el inventario
            fprintf(stderr, RED_COLOR "Error al actualizar el inventario para el producto '%s': %s\n" RESET_COLOR,
                    carrito[i].producto, sqlite3_errmsg(db));
        } else {
            // Mensaje de éxito en verde cuando la cantidad se actualiza correctamente
            printf(GREEN_COLOR "Producto '%s' actualizado correctamente. Nueva cantidad: %d\n" RESET_COLOR,
                   carrito[i].producto, carrito[i].cantidad);
        }

        // Verificar la cantidad restante después de la actualización
        snprintf(sql, sizeof(sql), "SELECT Cantidad FROM productos WHERE Producto = '%s';", carrito[i].producto);
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            // Mensaje de error en rojo si no se pudo ejecutar la consulta
            fprintf(stderr, RED_COLOR "Error al verificar la cantidad restante del producto '%s': %s\n" RESET_COLOR,
                    carrito[i].producto, sqlite3_errmsg(db));
        } else {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int cantidad_restante = sqlite3_column_int(stmt, 0);

                // Si la cantidad es 0, eliminamos el producto del inventario
                if (cantidad_restante == 0) {
                    snprintf(sql, sizeof(sql), "DELETE FROM productos WHERE Producto = '%s' AND Cantidad = 0;", carrito[i].producto);
                    rc = sqlite3_exec(db, sql, 0, 0, 0);
                    if (rc != SQLITE_OK) {
                        // Mensaje de error en rojo si no se pudo eliminar el producto
                        fprintf(stderr, RED_COLOR "Error al eliminar producto '%s' con 0 unidades: %s\n" RESET_COLOR,
                                carrito[i].producto, sqlite3_errmsg(db));
                    } else {
                        // Mensaje en amarillo para indicar que el producto ha sido eliminado
                        printf(YELLOW_COLOR "Producto '%s' eliminado del inventario por agotamiento.\n" RESET_COLOR, carrito[i].producto);
                    }
                } else {
                    // Si no es 0, mostramos un mensaje de actualización exitosa sin eliminar el producto
                    printf(GREEN_COLOR "Producto '%s' tiene %d unidades restantes en el inventario.\n" RESET_COLOR,
                           carrito[i].producto, cantidad_restante);
                }
            }
            sqlite3_finalize(stmt);
        }
    }
}




// Liberar la memoria del carrito
void liberar_carrito() {
    carrito_size = 0; // Reiniciar el tamaño del carrito
    printf(GREEN_COLOR "El carrito ha sido liberado y está listo para un nuevo pedido.\n" RESET_COLOR);
}




// Función para generar la factura
void generar_factura(sqlite3 *db, const char *identificador) {
    if (carrito_size == 0) {
        // Mensaje en rojo si no hay artículos en el carrito
        printf(RED_COLOR "No hay artículos en el carrito para generar una factura.\n" RESET_COLOR);
        return;
    }

    double total_factura = 0.0;

    // Título de la factura en verde
    printf(GREEN_COLOR "\n=== Factura para el Pedido: %s ===\n" RESET_COLOR, identificador);
    printf("+----------------------+----------+----------+----------+\n");
    printf("| Producto             | Cantidad | Código   | Precio   |\n");
    printf("+----------------------+----------+----------+----------+\n");

    for (int i = 0; i < carrito_size; i++) {
        char sql[256];
        snprintf(sql, sizeof(sql), "SELECT Código, Precio FROM productos WHERE Producto = '%s';", carrito[i].producto);
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            // Error de preparación en rojo
            fprintf(stderr, RED_COLOR "Error al preparar la consulta para el código y precio: %s\n" RESET_COLOR, sqlite3_errmsg(db));
            continue;
        }

        int codigo = 0;
        double precio = 0.0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            codigo = sqlite3_column_int(stmt, 0);
            precio = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);

        double subtotal = carrito[i].cantidad * precio;
        total_factura += subtotal;

        // Producto en verde, con la cantidad, código y precio en colores separados
        printf("| " GREEN_COLOR "%-20s " RESET_COLOR "| %-8d | %-8d | %-8.2f |\n", carrito[i].producto, carrito[i].cantidad, codigo, subtotal);

        // Llamada corregida a guardar_en_historial
        guardar_en_historial(identificador, carrito[i].producto, carrito[i].cantidad, codigo, precio);
    }

    printf("+----------------------+----------+----------+----------+\n");

    // Total factura en azul
    printf(BLUE_COLOR "Total Factura: %.2f\n" RESET_COLOR, total_factura);

    // Mensaje de éxito en verde
    printf(GREEN_COLOR "Factura generada exitosamente.\n" RESET_COLOR);
}


typedef struct {
    char fecha[11];
    char producto[50];
    int cantidad;
    int codigo;
    double precio;
} Factura;



 // Guardar en el historial de facturas
 void guardar_en_historial(const char *identificador, const char *producto, int cantidad, int codigo, double precio) {
    FILE *fp = fopen("historial_facturas.csv", "a");
    if (!fp) {
        // Mensaje de error en rojo si no se pudo abrir el archivo
        printf(RED_COLOR "Error: No se pudo abrir el archivo de historial de facturas.\n" RESET_COLOR);
        return;
    }

    // Obtener la fecha actual
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char fecha[11];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d", &tm);

    // Escribir en el archivo incluyendo el precio
    fprintf(fp, "%s,%s,%s,%d,%d,%.2f\n", identificador, fecha, producto, cantidad, codigo, precio);
    fclose(fp);

 
}


int comparar_facturas(const void *a, const void *b) {
    const Factura *fa = (const Factura *)a;
    const Factura *fb = (const Factura *)b;
    return strcmp(fa->fecha, fb->fecha);
}

//FUNCION principal 2
void realizar_pedido(sqlite3 *db) {
    char identificador[50];
    char opcion;

    // Solicitar el identificador al usuario
    printf(BLUE_COLOR "Ingrese el nombre del pedido (identificador): " RESET_COLOR);
    scanf(" %[^\n]s", identificador);

    // Mostrar el menú de pedido
    while (1) {
        printf("\n" YELLOW_COLOR "Menú de Pedido:\n" RESET_COLOR);
        printf(GREEN_COLOR "a) Solicitar artículo\n" RESET_COLOR);
        printf(GREEN_COLOR "b) Carrito de artículos\n" RESET_COLOR);
        printf(GREEN_COLOR "c) Editar pedido\n" RESET_COLOR);
        printf(GREEN_COLOR "d) Finalizar pedido\n" RESET_COLOR);
        printf(YELLOW_COLOR "Seleccione una opción: " RESET_COLOR);
        scanf(" %c", &opcion);

        switch (opcion) {
            case 'a': {
                char producto[50];
                int cantidad;
                char respuesta;

                do {
                    printf("\n" CYAN_COLOR "Lista de productos disponibles:\n" RESET_COLOR);
                    mostrar_productos_disponibles(); // Mostrar los productos disponibles

                    printf(BLUE_COLOR "\nIngrese el nombre del producto que desea agregar al carrito: " RESET_COLOR);
                    scanf(" %[^\n]s", producto);

                    printf(BLUE_COLOR "Ingrese la cantidad del producto: " RESET_COLOR);
                    scanf("%d", &cantidad);

                    // Verificar si el producto existe y agregar al carrito
                    char sql[256];
                    snprintf(sql, sizeof(sql), "SELECT Producto, Cantidad FROM productos WHERE Producto = '%s';", producto);
                    sqlite3_stmt *stmt;
                    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                    if (rc != SQLITE_OK) {
                        fprintf(stderr, RED_COLOR "Error al preparar la consulta: %s\n" RESET_COLOR, sqlite3_errmsg(db));
                        break;
                    }

                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        int cantidad_disponible = sqlite3_column_int(stmt, 1);

                        if (cantidad_disponible >= cantidad) {
                            printf(GREEN_COLOR "Producto agregado con éxito.\n" RESET_COLOR);
                            agregar_a_carrito(producto, cantidad); // Agregar al carrito
                        } else {
                            printf(RED_COLOR "Cantidad insuficiente en el inventario. Disponible: %d.\n" RESET_COLOR, cantidad_disponible);
                        }
                    } else {
                        printf(RED_COLOR "El producto no existe en el inventario.\n" RESET_COLOR);
                    }
                    sqlite3_finalize(stmt);

                    // Preguntar si desea seguir agregando productos o volver al menú de la función
                    printf(YELLOW_COLOR "\n¿Desea seguir agregando productos o regresar al menú de la función?\n" RESET_COLOR);
                    printf(CYAN_COLOR "Ingrese 's' para seguir agregando productos o 'm' para regresar al menú: " RESET_COLOR);
                    scanf(" %c", &respuesta);

                    if (respuesta == 'm') {
                        break;
                    }
                } while (respuesta == 's');

                break;
            }
            case 'b':
                mostrar_carrito(); // Mostrar el contenido del carrito
                break;
            case 'c': {
                char producto[50];
                int nueva_cantidad;
                char respuesta;

                do {
                    mostrar_carrito(); // Mostrar el contenido actual del carrito

                    // Solicitar el artículo que se desea editar
                    printf(BLUE_COLOR "\nEscriba el artículo que desea editar: " RESET_COLOR);
                    scanf(" %[^\n]s", producto);

                    // Buscar el artículo en el carrito
                    int encontrado = 0;
                    for (int i = 0; i < carrito_size; i++) {
                        if (strcmp(carrito[i].producto, producto) == 0) {
                            encontrado = 1;

                            // Solicitar la nueva cantidad
                            printf(BLUE_COLOR "Seleccione la cantidad nueva: " RESET_COLOR);
                            scanf("%d", &nueva_cantidad);

                            if (nueva_cantidad == 0) {
                                // Eliminar el artículo
                                for (int j = i; j < carrito_size - 1; j++) {
                                    carrito[j] = carrito[j + 1];
                                }
                                carrito_size--; // Actualizar el tamaño del carrito
                                i--; // Asegurar que el índice no salte un elemento
                                printf(GREEN_COLOR "Artículo eliminado con éxito.\n" RESET_COLOR);
                            } else {
                                // Actualizar la cantidad
                                carrito[i].cantidad = nueva_cantidad;
                                printf(GREEN_COLOR "Cantidad nueva actualizada correctamente.\n" RESET_COLOR);
                            }
                            break;
                        }
                    }

                    if (!encontrado) {
                        printf(RED_COLOR "El artículo no se encuentra en el carrito.\n" RESET_COLOR);
                    }

                    // Preguntar si desea seguir editando o volver al menú
                    printf(YELLOW_COLOR "\n¿Desea editar otro artículo o volver al menú?\n" RESET_COLOR);
                    printf(CYAN_COLOR "Ingrese 'c' para continuar o 'e' para salir: " RESET_COLOR);
                    scanf(" %c", &respuesta);

                } while (respuesta == 'c');

                break;
            }

            case 'd':
                printf(YELLOW_COLOR "Finalizando pedido y generando factura...\n" RESET_COLOR);
                generar_factura(db, identificador);
                actualizar_inventario(db);
                liberar_carrito();
                return;
            default:
                printf(RED_COLOR "Opción no válida. Intente de nuevo.\n" RESET_COLOR);
                break;
        }
    }
}

//FIn funcion 2

//Opción 3
//mostrar hisyorial al usuario

void mostrar_historial_usuario(const char *identificador) {
    FILE *fp = fopen("historial_facturas.csv", "r");
    if (!fp) {
        // Mensaje de error en rojo
        printf(RED_COLOR "Error: No se pudo abrir el archivo de historial de facturas.\n" RESET_COLOR);
        return;
    }

    Factura facturas[100];
    int count = 0;
    double total_factura = 0.0;

    char file_identificador[50], fecha[11], producto[50];
    int cantidad, codigo;
    double precio;

    // Leer las facturas desde el archivo
    while (fscanf(fp, "%49[^,],%10[^,],%49[^,],%d,%d,%lf\n",
                  file_identificador, fecha, producto, &cantidad, &codigo, &precio) == 6) {
        if (strcmp(file_identificador, identificador) == 0) {
            strncpy(facturas[count].fecha, fecha, sizeof(facturas[count].fecha) - 1);
            strncpy(facturas[count].producto, producto, sizeof(facturas[count].producto) - 1);
            facturas[count].cantidad = cantidad;
            facturas[count].codigo = codigo;
            facturas[count].precio = precio;
            total_factura += cantidad * precio;
            count++;
        }
    }
    fclose(fp);

    if (count == 0) {
        // Mensaje en rojo si no hay facturas asociadas
        printf(RED_COLOR "No hay facturas asociadas con el identificador '%s'.\n" RESET_COLOR, identificador);
        return;
    }

    // Ordenar las facturas por fecha
    qsort(facturas, count, sizeof(Factura), comparar_facturas);

    // Mostrar el historial de facturas
    printf(GREEN_COLOR "\n=== Historial de Facturas para: %s ===\n" RESET_COLOR, identificador);
    printf(YELLOW_COLOR "+------------+----------------------+----------+----------+----------+----------+\n" RESET_COLOR);
    printf(YELLOW_COLOR "| Fecha      | Producto             | Cantidad | Código   | Precio   | Subtotal |\n" RESET_COLOR);
    printf(YELLOW_COLOR "+------------+----------------------+----------+----------+----------+----------+\n" RESET_COLOR);

    for (int i = 0; i < count; i++) {
        double subtotal = facturas[i].cantidad * facturas[i].precio;
        // Alternar entre cian y blanco para los datos
        if (i % 2 == 0) {
            printf(CYAN_COLOR "| %-10s | %-20s | %-8d | %-8d | %-8.2f | %-8.2f |\n" RESET_COLOR,
                   facturas[i].fecha, facturas[i].producto,
                   facturas[i].cantidad, facturas[i].codigo,
                   facturas[i].precio, subtotal);
        } else {
            printf(WHITE_COLOR "| %-10s | %-20s | %-8d | %-8d | %-8.2f | %-8.2f |\n" RESET_COLOR,
                   facturas[i].fecha, facturas[i].producto,
                   facturas[i].cantidad, facturas[i].codigo,
                   facturas[i].precio, subtotal);
        }
    }

    printf(YELLOW_COLOR "+------------+----------------------+----------+----------+----------+----------+\n" RESET_COLOR);
    printf(GREEN_COLOR "Total Factura: %.2f\n" RESET_COLOR, total_factura);
}

//Final función 3


//FUNCION 4
// \\Función A para agregar un producto al inventario (Ale)
void agregar_producto(sqlite3 *db) {
    char producto[50];
    int cantidad, codigo;
    double precio;
    char sql[256];
    sqlite3_stmt *stmt;

    // Solicitar el nombre del producto
    printf(BLUE_COLOR "Ingrese el nombre del producto: " RESET_COLOR);
    scanf(" %[^\n]s", producto);

    // Verificar si el nombre del producto no está vacío
    if (strlen(producto) == 0) {
        fprintf(stderr, RED_COLOR "Error: El nombre del producto no puede estar vacío.\n" RESET_COLOR);
        return;
    }

    // Verificar si el producto ya existe
    snprintf(sql, sizeof(sql), "SELECT Producto, Cantidad, Código, Precio FROM productos WHERE Producto = '%s';", producto);
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, RED_COLOR "Error al preparar la consulta: %s\n" RESET_COLOR, sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *producto_existente = (const char *)sqlite3_column_text(stmt, 0);
        int cantidad_existente = sqlite3_column_int(stmt, 1);
        int codigo_existente = sqlite3_column_int(stmt, 2);
        double precio_existente = sqlite3_column_double(stmt, 3);

        printf(YELLOW_COLOR "El producto ya existe en el inventario:\n" RESET_COLOR);
        printf(CYAN_COLOR "Producto: %s, Cantidad: %d, Código: %d, Precio: %.2f\n" RESET_COLOR,
               producto_existente, cantidad_existente, codigo_existente, precio_existente);

        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // Si el producto no existe, solicitar datos
    printf(YELLOW_COLOR "El producto no existe en el inventario. Procederemos a agregarlo.\n" RESET_COLOR);
    printf(BLUE_COLOR "Ingrese la cantidad: " RESET_COLOR);
    scanf("%d", &cantidad);
    printf(BLUE_COLOR "Ingrese el código: " RESET_COLOR);
    scanf("%d", &codigo);
    printf(BLUE_COLOR "Ingrese el precio: " RESET_COLOR);
    scanf("%lf", &precio);

    // Validar que los valores ingresados sean correctos
    if (cantidad <= 0 || codigo <= 0 || precio <= 0) {
        fprintf(stderr, RED_COLOR "Error: Los datos del producto son inválidos. Verifique los valores.\n" RESET_COLOR);
        return;
    }

    // Insertar producto en la base de datos
    snprintf(sql, sizeof(sql),
             "INSERT INTO productos (Producto, Cantidad, Código, Precio) VALUES ('%s', %d, %d, %.2f);",
             producto, cantidad, codigo, precio);
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, RED_COLOR "Error al agregar el producto: %s\n" RESET_COLOR, sqlite3_errmsg(db));
        return;
    }

    printf(GREEN_COLOR "Producto agregado correctamente al inventario.\n" RESET_COLOR);
}




void modificar_producto(sqlite3 *db) {
    int codigo;
    char sql[256];
    sqlite3_stmt *stmt;
    int rc;

    // Solicitar el código del producto
    printf(BLUE_COLOR "Ingrese el código del producto que desea modificar: " RESET_COLOR);
    if (scanf("%d", &codigo) != 1) {
        fprintf(stderr, RED_COLOR "Entrada no válida. Debe ingresar un número.\n" RESET_COLOR);
        return;
    }

    // Verificar si el producto existe
    snprintf(sql, sizeof(sql), "SELECT Producto, Cantidad, Precio FROM productos WHERE Código = ?;");
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, RED_COLOR "Error al preparar la consulta: %s\n" RESET_COLOR, sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, codigo);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *producto = (const char *)sqlite3_column_text(stmt, 0);
        int cantidad_actual = sqlite3_column_int(stmt, 1);
        double precio_actual = sqlite3_column_double(stmt, 2);

        printf(GREEN_COLOR "Producto encontrado: %s\n" RESET_COLOR, producto);
        printf(CYAN_COLOR "Cantidad actual: %d\n" RESET_COLOR, cantidad_actual);
        printf(CYAN_COLOR "Precio actual: %.2f\n" RESET_COLOR, precio_actual);

        // Solicitar nuevos valores
        int nueva_cantidad;
        double nuevo_precio;
        printf(BLUE_COLOR "Ingrese la nueva cantidad (o -1 para no modificar): " RESET_COLOR);
        scanf("%d", &nueva_cantidad);
        printf(BLUE_COLOR "Ingrese el nuevo precio (o -1 para no modificar): " RESET_COLOR);
        scanf("%lf", &nuevo_precio);

        sqlite3_finalize(stmt);

        if (nueva_cantidad == 0) {
            // Eliminar el producto del inventario
            snprintf(sql, sizeof(sql), "DELETE FROM productos WHERE Código = ?;");
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, codigo);
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    printf(GREEN_COLOR "Producto eliminado exitosamente.\n" RESET_COLOR);
                } else {
                    fprintf(stderr, RED_COLOR "Error al eliminar el producto: %s\n" RESET_COLOR, sqlite3_errmsg(db));
                }
            } else {
                fprintf(stderr, RED_COLOR "Error al preparar la consulta de eliminación: %s\n" RESET_COLOR, sqlite3_errmsg(db));
            }
        } else {
            // Construir la consulta de actualización
            snprintf(sql, sizeof(sql),
                     "UPDATE productos SET Cantidad = CASE WHEN ? >= 0 THEN ? ELSE Cantidad END, "
                     "Precio = CASE WHEN ? >= 0 THEN ? ELSE Precio END WHERE Código = ?;");

            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
            if (rc == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, nueva_cantidad);
                sqlite3_bind_int(stmt, 2, nueva_cantidad);
                sqlite3_bind_double(stmt, 3, nuevo_precio);
                sqlite3_bind_double(stmt, 4, nuevo_precio);
                sqlite3_bind_int(stmt, 5, codigo);

                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    printf(GREEN_COLOR "Producto actualizado exitosamente.\n" RESET_COLOR);
                } else {
                    fprintf(stderr, RED_COLOR "Error al actualizar el producto: %s\n" RESET_COLOR, sqlite3_errmsg(db));
                }
            } else {
                fprintf(stderr, RED_COLOR "Error al preparar la consulta de actualización: %s\n" RESET_COLOR, sqlite3_errmsg(db));
            }
        }
    } else {
        printf(RED_COLOR "No se encontró ningún producto con el código proporcionado.\n" RESET_COLOR);
    }

    sqlite3_finalize(stmt);
}





// \\Menú de edición del inventario
void editar_inventario() {
    sqlite3 *db;
    int rc;
    char opcion;

    rc = sqlite3_open("Inventario.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, RED_COLOR "No se pudo abrir la base de datos: %s\n" RESET_COLOR, sqlite3_errmsg(db));
        return;
    }

    while (1) {
        // Título del menú en azul
        printf(BLUE_COLOR "\nMenú de Edición del Inventario:\n" RESET_COLOR);
        // Opciones del menú en verde
        printf(GREEN_COLOR "a) Agregar producto\n" RESET_COLOR);
        printf(GREEN_COLOR "b) Modificar producto (cantidad y precio)\n" RESET_COLOR);
        printf(GREEN_COLOR "c) Salir y volver al menú principal\n" RESET_COLOR);
        // Solicitar opción en amarillo
        printf(YELLOW_COLOR "Seleccione una opción: " RESET_COLOR);
        scanf(" %c", &opcion);

        switch (opcion) {
            case 'a':
                printf(CYAN_COLOR "Opción seleccionada: Agregar producto.\n" RESET_COLOR);
                agregar_producto(db);
                break;
            case 'b':
                printf(CYAN_COLOR "Opción seleccionada: Modificar producto.\n" RESET_COLOR);
                modificar_producto(db);
                break;
            case 'c':
                // Mensaje de salida en amarillo
                printf(YELLOW_COLOR "Saliendo de la edición del inventario.\n" RESET_COLOR);
                sqlite3_close(db);
                return;
            default:
                // Mensaje de error en rojo
                printf(RED_COLOR "Opción no válida. Intente de nuevo.\n" RESET_COLOR);
        }
    }
}






// \\Menú principal
int main() {
    sqlite3 *db;
    int rc;

    rc = sqlite3_open("Inventario.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, RED_COLOR "No se pudo abrir la base de datos: %s\n" RESET_COLOR, sqlite3_errmsg(db));
        return 1;
    }

    char opcion;
    while (1) {
        // Título del menú en azul
        printf(BLUE_COLOR "\nMenú principal:\n" RESET_COLOR);
        // Opciones del menú en verde
        printf(GREEN_COLOR "1) Productos disponibles\n" RESET_COLOR);
        printf(GREEN_COLOR "2) Realizar pedido\n" RESET_COLOR);
        printf(GREEN_COLOR "3) Historial de facturas por usuario\n" RESET_COLOR);
        printf(GREEN_COLOR "4) Editar inventario\n" RESET_COLOR);
        printf(GREEN_COLOR "5) Salir\n" RESET_COLOR);
        // Solicitar opción en amarillo
        printf(YELLOW_COLOR "Seleccione una opción: " RESET_COLOR);
        scanf(" %c", &opcion);

        switch (opcion) {
            case '1':
                // Notificación de selección en cian
                printf(CYAN_COLOR "Opción seleccionada: Mostrar productos disponibles.\n" RESET_COLOR);
                mostrar_productos_disponibles();
                break;
            case '2':
                printf(CYAN_COLOR "Opción seleccionada: Realizar pedido.\n" RESET_COLOR);
                realizar_pedido(db);
                break;
            case '3': {
                char identificador[50];
                printf(YELLOW_COLOR "Ingrese el identificador para ver su historial de facturación: " RESET_COLOR);
                scanf(" %[^\n]s", identificador);
                printf(CYAN_COLOR "Mostrando historial de facturas para '%s'.\n" RESET_COLOR, identificador);
                mostrar_historial_usuario(identificador);
                break;
            }
            case '4':
                printf(CYAN_COLOR "Opción seleccionada: Editar inventario.\n" RESET_COLOR);
                editar_inventario();
                break;
            case '5':
                // Mensaje de salida en amarillo
                printf(YELLOW_COLOR "Saliendo del programa...\n" RESET_COLOR);
                sqlite3_close(db);
                return 0;
            default:
                // Mensaje de error en rojo
                printf(RED_COLOR "Opción no válida. Intente de nuevo.\n" RESET_COLOR);
                break;
        }
    }
}

