/**
 * @file directorios.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa para gestionar las estructuras de directorios
 *
 */
#include "directorios.h"

//////////// NIVEL 7 //////////

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    return 0;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo,
                   unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;
    memset(inicial, 0, sizeof(entrada.nombre));
    memset(final, 0, strlen(camino_parcial));
    memset(entrada.nombre, 0, sizeof(entrada.nombre));


    if (!strcmp(camino_parcial, "/")) {    // camino_parcial es “/”
        // lectura superbloque
        struct superbloque SB;
        if (bread(posSB, &SB) == -1) {
            return -1;
        }

        // nuestra raiz siempre estará asociada al inodo 0
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return 0;
    }
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == -1) {
        return ERROR_CAMINO_INCORRECTO;
    }
#if DEBUG
    printf("[buscar_entrada()->inicial: %s, final: %s, reservar: %d]\n", inicial, final, reservar);
#endif
    // buscamos la entrada cuyo nombre se encuentra en inicial
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == -1) {
        return ERROR_PERMISO_LECTURA;
    }
    // array de tipo struct entrada de las entradas que caben en un bloque, para optimizar la lectura en RAM
    struct entrada buffer_lectura[BLOCKSIZE / sizeof(struct entrada)];
    memset(buffer_lectura, 0, (BLOCKSIZE / sizeof(struct entrada)) * sizeof(struct entrada));
    // cantidad de entradas que contiene el inodo
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    // nº de entrada inicial
    num_entrada_inodo = 0;
    if (cant_entradas_inodo > 0) {
        // Comprobar permisos
        if ((inodo_dir.permisos & 4) != 4) {
            return ERROR_PERMISO_LECTURA;
        }
        // leer entrada
        if (mi_read_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            return ERROR_PERMISO_LECTURA;
        }
        while ((num_entrada_inodo < cant_entradas_inodo) && (strcmp(inicial, entrada.nombre) != 0)) {
            num_entrada_inodo++;
            // leer siguiente entrada
            if (mi_read_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
                return ERROR_PERMISO_LECTURA;
            }
        }
    }

    // inicial != entrada.nombre --> la entrada no existe
    if (num_entrada_inodo == cant_entradas_inodo &&
        (inicial != buffer_lectura[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))].nombre)) {
        // seleccionar(reserva)
        switch (reservar) {
                // modo consulta. Como no existe retornamos error
            case 0:
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
                break;
            // modo escritura
            case 1:
                // Creamos la entrada en el directorio referenciado por *p_inodo_dir
                // si es fichero no permitir escritura
                if (inodo_dir.tipo == 'f') {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
                }
                // si es directorio comprobar que tiene permiso de escritura
                if ((inodo_dir.permisos & 2) != 2) {
                    return ERROR_PERMISO_ESCRITURA;
                } else {
                    // copiar *inicial en el nombre de la entrada
                    strcpy(entrada.nombre, inicial);
                    if (tipo == 'd') {
                        if (strcmp(final, "/") == 0) {
                            // reservar un nuevo inodo como directorio y asignarlo a la entrada
                            entrada.ninodo = reservar_inodo(tipo, permisos);  // reservar_inodo('d', 6)
#if DEBUG
                            printf("[buscar_entrada()->reservado inodo: %d tipo %c con permisos %d para '%s']\n", entrada.ninodo, tipo, permisos, entrada.nombre);
#endif
                        } else {
                            // cuelgan más diretorios o ficheros
                            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                        }
                    } else {
                        // es un fichero
                        // reservar un inodo como fichero y asignarlo a la entrada
                        entrada.ninodo = reservar_inodo(tipo, permisos);  // reservar_inodo('f', 6)
#if DEBUG
                        printf("[buscar()->reservado inodo: %d tipo %c con permisos %d para '%s']\n", entrada.ninodo, tipo, permisos, entrada.nombre);
#endif
                    }
#if DEBUG
                    fprintf(stderr, "[buscar_entrada()->creada entrada: %s, %d] \n", inicial, entrada.ninodo);
#endif

                    // escribir la entrada en el directorio padre
                    if (mi_write_f(*p_inodo_dir, &entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == -1) {
                        // se había reservado un inodo para la entrada
                        if (entrada.ninodo != -1) {
                            liberar_inodo(entrada.ninodo);
#if DEBUG
                            fprintf(stderr, "[buscar_entrada()-> liberado inodo %i, reservado a %s\n", num_entrada_inodo, inicial);
#endif
                        }
                        return -1;
                    }
                }
        }
    }
    // Si hemos llegado al final del camino
    if (!strcmp(final, "/") || !strcmp(final, "")) {
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar == 1)) {
            // modo escritura y la entrada ya existe
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        // asignar a *p_inodo el numero de inodo del directorio o fichero creado o leido
        *(p_inodo) = num_entrada_inodo;
        // asignar a *p_entrada el número de su entrada dentro del último directorio que lo contiene
        *(p_entrada) = entrada.ninodo;
        // cortamos la recursividad
        return 0;
    } else {
        // asignamos a *p_inodo_dir el puntero al inodo que se indica en la entrada encontrada;
        *(p_inodo_dir) = entrada.ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}
