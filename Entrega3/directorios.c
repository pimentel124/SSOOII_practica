/**
 * @file directorios.c
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Programa para gestionar las estructuras de directorios
 *
 */
#include "directorios.h"

struct UltimaEntrada UltimaEntradaEscritura[CACHE_ULT];
int max_cache = CACHE_ULT;
int cursor = 0;

#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0

//////////// NIVEL 7 /////////////

/**
 * @brief Dada una cadena de caracteres camino (que comience por /), separa su contenido en dos.
 * Guarda en *inicial la porción de camino comprendida entre los dos primeros /. (En tal cado, *inicial contendrá el nombre de un directorio)
 * Cuando no hay segundo /, copia *camino en *inicial sin el primer /. (En tal caso *inicial contendrá el nombre de un fichero)
 *
 * @param camino    Cadena de caracteres que contiene el camino a separar
 * @param inicial   Cadena de caracteres donde se guardará la porción inicial del camino
 * @param tipo      Cadena de caracteres donde se guardará el tipo de camino (directorio o fichero)
 * @return Devuelve 0 si se ha podido separar el camino, -1 en caso contrario
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // Check syntax
    if (camino[0] != '/') {
        return -1;
    }
    if (camino == NULL || inicial == NULL || final == NULL || tipo == NULL) {
        return -1;
    }

    // se obtiene el resto del camino
    const char *resto = strchr((camino + 1), '/');

    if (resto) {
        // nos quedamos con la parte inicial menos resto
        strncpy(inicial, (camino + 1), resto - (camino + 1));
        // nos quedamos con la parte final
        strcpy(final, resto);
        // nos quedamos con el tipo
        strcpy(tipo, "d");
    } else {  // si no tiene parte final, significa que estamos trabajando con un fichero
        strcpy(inicial, (camino + 1));
        strcpy(final, "");
        strcpy(tipo, "f");
    }

#if DEBUG7
    fprintf(stderr, "Camino: %s\nInicio: %s\nFinal: %s\nTipo: %s\n", camino, inicial, final, tipo);
#endif
    return 0;
}

/**
 * @brief Esta función nos buscará una determinada entrada (la parte *inicial del *camino_parcial
 * que nos devuelva extraer_camino()) entre las entradas del inodo correspondiente a su
 * directorio padre (identificado con *p_inodo_dir).
 *
 * @param camino_parcial    Cadena de caracteres que contiene el camino a buscar
 * @param p_inodo_dir       Puntero al inodo del directorio padre
 * @param p_inodo_ent       Puntero al inodo de la entrada a buscar
 * @param p_entrada         Puntero a la entrada a buscar
 * @param reservar          Indica si se debe reservar la entrada o no
 * @param permisos          Permisos de la entrada a buscar
 * @return int
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    if (strcmp(camino_parcial, "/") == 0) {
        *p_inodo = 0;  // Inodo raìz
        *p_entrada = 0;
        fprintf(stderr, "Error: no se puede crear el Inodo raiz\n");
        return 0;
    }

    if (p_inodo == NULL) {
        printf("DEBUG - buscar_entrada() | p_inodo es null! Set a 0\n");
        unsigned int placeholder = 0;
        p_inodo = &placeholder;
    }
    if (p_entrada == NULL) {
        printf("DEBUG - buscar_entrada() | p_entrada es null! Set a 0\n");
        unsigned int placeholder = 0;
        p_entrada = &placeholder;
    }

    char inicial[256];
    char final[strlen(camino_parcial)];
    memset(inicial, 0, 256);
    memset(final, 0, strlen(camino_parcial));
    char tipo;
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == -1) {
        fprintf(stderr, "Error en directorios.c buscar_entrada() --> Error en extraer camino\n");
        return -1;
    }
    struct inodo in;

    if (leer_inodo(*p_inodo_dir, &in) == -1) {
        fprintf(stderr, "Error en directorios.c buscar_entrada() --> Ha ocurrido un error leyendo el inodo\n");
        return -1;
    }
    // printf("DEBUG - buscar_entrada() | Desp. de lectura inodo | *p_inodo: %d | *p_entrada: %d\n",*p_inodo,*p_entrada);

    char buffer[in.tamEnBytesLog];
    struct entrada *entrada;
    entrada = malloc(sizeof(struct entrada));
    entrada->nombre[0] = '\0';
    int numentrades = in.tamEnBytesLog / sizeof(struct entrada);
    int nentrada = 0;

    // printf("DEBUG - buscar_entrada() | numentrades: %d | nentrada: %d\n",numentrades,nentrada);

    if (numentrades > 0) {
        // printf("DEBUG - buscar_entrada() | Permisos del inodo %d: %c\n",*p_inodo_dir, in.permisos);
        if ((in.permisos & 4) != 4) {
            fprintf(stderr, "Error en directorios.c buscar_entrada() --> No tiene permisos de lectura\n");
            return -1;
        }
        int offset = 0;
        int encontrado = 1;
        while (nentrada < numentrades && encontrado != 0) {
            mi_read_f(*p_inodo_dir, buffer, nentrada * sizeof(struct entrada), sizeof(buffer));  // leer siguiente entrada
            memcpy(entrada, buffer, sizeof(struct entrada));

            // printf("DEBUG - buscar_entrada() | inicial: %s | entrada->nombre: %s\n",inicial,entrada->nombre);

            encontrado = strcmp(inicial, entrada->nombre);
            // printf("DEBUG - buscar_entrada() | strcmp en while 1 OK\n");
            while (offset < numentrades && nentrada < numentrades && encontrado != 0) {
                // printf("DEBUG - buscar_entrada() | EN WHILE 2\n");
                nentrada++;
                offset++;
                memcpy(entrada, offset * sizeof(struct entrada) + buffer, sizeof(struct entrada));
                // printf("DEBUG - buscar_entrada() | EN WHILE 2 | inicial: %s | entrada->nombre: %s\n",inicial,entrada->nombre);
                encontrado = strcmp(inicial, entrada->nombre);
                // printf("DEBUG - buscar_entrada() | strcmp en while 2 OK\n");
            }
            offset = 0;
        }
    }
    if (nentrada == numentrades) {
        // printf("DEBUG - buscar_entrada() | En if 2\n");
        switch (reservar) {
            case 0:
                fprintf(stderr, "Error en directorios.c buscar_entrada() --> No existe entrada consulta\n");
                return -1;
            case 1:
                if (in.tipo == 'f') {
                    fprintf(stderr, "Error en directorios.c buscar_entrada() --> Reservar = 1 y tipo de inodo = 'f'\n");
                    return -1;
                }
                strcpy(entrada->nombre, inicial);
                if (tipo == 'd') {
                    if (strcmp(final, "/") == 0) {
                        entrada->ninodo = reservar_inodo('d', permisos);
                    } else {
                        fprintf(stderr, "Error en directorios.c buscar_entrada() --> No existe directorio intermedio\n");
                        return -1;
                    }
                } else {
                    entrada->ninodo = reservar_inodo('f', permisos);
                }
                if (mi_write_f(*p_inodo_dir, entrada, nentrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
                    if (entrada->ninodo != -1) {
                        liberar_inodo(entrada->ninodo);
                    }
                    fprintf(stderr, "Error en directorios.c buscar_entrada() --> No tiene permisos de escritura\n");
                    return -1;
                }
        }
    }
    // printf("DEBUG - buscar_entrada() | Fuera de if 2\n");
    if (!strcmp(final, "") || !strcmp(final, "/")) {
        // printf("DEBUG - buscar_entrada() | strcmp final OK | final: %s\n",final);
        if ((nentrada < numentrades) && (reservar == 1)) {
            fprintf(stderr, "nentrada: %d numentradaes: %d\n", nentrada, numentrades);
            fprintf(stderr, "Error en directorios.c buscar_entrada() --> Entrada ya existente\n");
            return -1;
        }
        *p_inodo = entrada->ninodo;
        *p_entrada = nentrada;
        // printf("DEBUG - buscar_entrada() | *p_inodo: %d | *p_entrada: %d\n",*p_inodo,*p_entrada);
        // printf("DEBUG - buscar_entrada() | Antes de return 0\n");
        return 0;
    } else {
        *p_inodo_dir = entrada->ninodo;
        // printf("DEBUG - buscar_entrada() | A final de todo, llamada recursiva\n");
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}

/**
 * @brief Se muestra por consola el correspondiente error entrado por parámetro
 *
 * @param error
 */
void mostrar_error_buscar_entrada(int error) {
    switch (error) {
        case ERROR_CAMINO_INCORRECTO:
            fprintf(stderr, COLOR_ERROR "Error: Camino incorrecto\n" COLOR_RESET);
            break;
        case ERROR_PERMISO_LECTURA:
            fprintf(stderr, COLOR_ERROR "Error: Permiso denegado de lectura\n" COLOR_RESET);
            break;
        case ERROR_NO_EXISTE_ENTRADA_CONSULTA:
            fprintf(stderr, COLOR_ERROR "Error: No existe el archivo o directorio\n" COLOR_RESET);
            break;
        case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:
            fprintf(stderr, COLOR_ERROR "Error: No existe algún directorio intermedip\n" COLOR_RESET);
            break;
        case ERROR_PERMISO_ESCRITURA:
            fprintf(stderr, COLOR_ERROR "Error: Permiso denegado de escritura\n" COLOR_RESET);
            break;
        case ERROR_ENTRADA_YA_EXISTENTE:
            fprintf(stderr, COLOR_ERROR "Error: El archivo ya existe\n" COLOR_RESET);
            break;
        case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO:
            fprintf(stderr, COLOR_ERROR "Error: No es un directorio\n" COLOR_RESET);
            break;
    }
}

//////////////////NIVEL 8////////////////////////////

/**
 * @brief Función de la capa de directorios que crea un fichero/directorio y su entrada de directorio
 * Se pasa en la función buscar_entrada() con reservar = 1
 *
 * @param camino   Camino del fichero/directorio a crear
 * @param permisos Permisos del fichero/directorio a crear
 * @return int
 */
int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = 0;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    if (error != EXIT_SUCCESS) {
        // En caso de que haya ocurrido un error se muestra por pantalla
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return -1;
    }

    mi_signalSem();
    return 0;
}

/**
 * @brief Función de la capa de directorios que pone el condenido del directorio en un buffer de memoria y devuelve el número de entradas
 *
 * @param camino    Camino del directorio a consultar
 * @param buffer    Puntero a la dirección de memoria donde se guardará el contenido del directorio
 * @param tipo      Tipo de directorio a consultar (fichero o directorio)
 * @return int
 */
int mi_dir(const char *camino, char *buffer, char tipo) {
    unsigned int p_inodo_dir, p_inodo, p_entrada;

    char longitud[TAMFILA];
    struct entrada entrada;
    struct inodo inodo;
    struct tm *tm;

    p_inodo_dir = 0;

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return -1;
    }
    if (leer_inodo(p_inodo, &inodo) == -1) {
        fprintf(stderr, "Error: (mi_dir) al leer el inodo\n");
        return -1;
    }

    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, "Error: (mi_dir) no hay permisos de lectura\n");
        return -1;
    }

    if (inodo.tipo != tipo) {
        fprintf(stderr, "Error: (mi_dir) la sintaxis no concuerda con el tipo\n");
        return -1;
    }

    int num_entrada_inodo;
    int cant_entradas_inodo;

    if (inodo.tipo == 'd') {
        num_entrada_inodo = 0;

        cant_entradas_inodo = inodo.tamEnBytesLog / sizeof(entrada);

        while (num_entrada_inodo < cant_entradas_inodo) {
            if (mi_read_f(p_inodo, &entrada, num_entrada_inodo * sizeof(entrada), sizeof(entrada)) < 0) {
                fprintf(stderr, "Error: (mi_dir) ejecutando mi_read_f");
                return -1;
            }

            if (entrada.ninodo >= 0) {
                struct inodo inodoAux;
                if (leer_inodo(entrada.ninodo, &inodoAux) == -1) {
                    fprintf(stderr, "Error: (mi_dir) al leer el inodoAux");
                    return -1;
                }

                if (inodoAux.tipo == 'd') {
                    strcat(buffer, "d");
                } else if (inodoAux.tipo == 'f') {
                    strcat(buffer, "f");
                }
                strcat(buffer, "\t");

                if (inodoAux.permisos & 4)
                    strcat(buffer, "r");
                else
                    strcat(buffer, "-");

                if (inodoAux.permisos & 2)
                    strcat(buffer, "w");
                else
                    strcat(buffer, "-");

                if (inodoAux.permisos & 1)
                    strcat(buffer, "x");
                else
                    strcat(buffer, "-");

                strcat(buffer, "\t");

                char tmp[100];
                tm = localtime(&inodoAux.mtime);
                sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                strcat(buffer, tmp);
                strcat(buffer, "\t");

                memset(longitud, '\0', sizeof(longitud));
                sprintf(longitud, " %d bytes", inodoAux.tamEnBytesLog);
                strcat(buffer, longitud);
                strcat(buffer, "\t");

                strcat(buffer, entrada.nombre);
                strcat(buffer, "\t");
                strcat(buffer, "\n");

                num_entrada_inodo++;
            } else {
                return 0;
            }
        }
    }

    else if (inodo.tipo == 'f') {
        num_entrada_inodo = 1;
        if (mi_read_f(p_inodo, &entrada, num_entrada_inodo * sizeof(entrada), sizeof(entrada)) < 0) {
            fprintf(stderr, "Error: (mi_dir) ejecutando mi_read_f");
            return -1;
        }

        struct inodo inodoAux;
        if (leer_inodo(p_inodo, &inodoAux) == -1) {
            perror("Error al leer_inodo en mi_dir");
            return -1;
        }

        strcat(buffer, "f");
        strcat(buffer, "\t");

        if (inodoAux.permisos & 4)
            strcat(buffer, "r");
        else
            strcat(buffer, "-");

        if (inodoAux.permisos & 2)
            strcat(buffer, "w");
        else
            strcat(buffer, "-");

        if (inodoAux.permisos & 1)
            strcat(buffer, "x");
        else
            strcat(buffer, "-");

        strcat(buffer, "\t");

        struct tm *tm;
        char tmp[100];
        tm = localtime(&inodoAux.mtime);
        sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        strcat(buffer, tmp);
        strcat(buffer, "\t");

        memset(longitud, '\0', sizeof(longitud));
        sprintf(longitud, " %d bytes", inodoAux.tamEnBytesLog);
        strcat(buffer, longitud);
        strcat(buffer, "\t");

        strcat(buffer, entrada.nombre);
        strcat(buffer, "\t");
        strcat(buffer, "\n");
    }

    return num_entrada_inodo;
}

/**
 * @brief Busca la entrada *camino con buscar_entrada() para obtener el nº inodo.
 * Si la entrada existe llamamos a la función correspondiente de ficheros.c
 *
 * @param camino    camino de donde extraer nºinodo
 * @param permisos  Permisos
 * @return int
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    struct inodo inodo;

    p_inodo_dir = 0;

    int err_entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    if (err_entrada != EXIT_SUCCESS) {
        mostrar_error_buscar_entrada(err_entrada);
        return -1;
    }

    if (leer_inodo(p_inodo, &inodo) == -1)
        return -1;

    return mi_chmod_f(p_inodo, permisos);
}

/**
 * @brief Buscar la entrada *camino con buscar_entrada() para obtener el nº inodo.
 * Si la entrada existe llamamos a la función correspondiente de ficheros.c pasandole el p_inodo.
 *
 * @param camino    camino de donde extraer nºinodo
 * @param p_stat    Puntero a estructura stat
 * @return int
 */
int mi_stat(const char *camino, struct STAT *p_stat) {
    struct inodo inodo;
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = 0;

    int err_entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (err_entrada < 0) {
        mostrar_error_buscar_entrada(err_entrada);
        return -1;
    }

    if (mi_stat_f(p_inodo, p_stat) < 0) {
        perror("Error en mi_stat_f, en mi_stat");
        return -1;
    }

    if (leer_inodo(p_inodo, &inodo) == -1)
        return -1;

    return p_inodo;
}

//////////////////NIVEL 9////////////////////////////

/**
 * @brief Función para leer los nbytes del fichero indicado por camino, a partir del offset indicado y escribir los datos en el buffer.
 *
 * @param camino    Camino del fichero
 * @param buf       Buffer donde se escribirán los datos
 * @param offset    Offset del fichero
 * @param nbytes    Número de bytes a leer
 * @return int
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo = 0, p_inodo_dir = 0, p_entrada = 0, encontrado = 0;

    for (int i = 0; i < CACHE_ULT - 1; i++) {
        if (strcmp(camino, UltimaEntradaEscritura[i].camino) == -1) {
            p_inodo = UltimaEntradaEscritura[i].p_inodo;
            encontrado = 1;
        }
    }

    if (!encontrado) {
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (error < 0) {
            mostrar_error_buscar_entrada(error);
            return -1;
        }

        if (max_cache > 0) {
            strcpy(UltimaEntradaEscritura[CACHE_ULT - max_cache].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT - max_cache].p_inodo = p_inodo;
            max_cache--;
#if DEBUG9
            fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
#endif
        } else {
            for (int i = 0; i < CACHE_ULT - 1; i++) {
                strcpy(UltimaEntradaEscritura[i].camino, UltimaEntradaEscritura[i + 1].camino);
                UltimaEntradaEscritura[i].p_inodo = UltimaEntradaEscritura[i + 1].p_inodo;
            }
            strcpy(UltimaEntradaEscritura[CACHE_ULT - 1].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT - 1].p_inodo = p_inodo;
#if DEBUG9
            fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
#endif
        }
    }

    int bytesLeidos = mi_read_f(p_inodo, buf, offset, nbytes);
    if (bytesLeidos == -1) {
        mostrar_error_buscar_entrada(ERROR_PERMISO_LECTURA);
        return -1;
    }
    return bytesLeidos;
}

/**
 * @brief Función para escribir contenido en el fichero indicado por camino, a partir del offset indicado y leer los datos del buffer.
 *
 * @param camino    Camino del fichero
 * @param buf       Buffer donde se leerán los datos
 * @param offset    Offset del fichero
 * @param nbytes    Número de bytes a escribir
 * @return Cantidad de bytes escritos
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int encontrado = 0, error, resultado;
    // comprobar caché
    for (int i = 0; i < max_cache - 1; i++) {
        if (strcmp(camino, UltimaEntradaEscritura[i].camino) == 0) {
            encontrado = 1;
            p_inodo = UltimaEntradaEscritura[i].p_inodo;
            break;
        }
    }
    if (!encontrado) {
        if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) != EXIT_SUCCESS) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
        }

        if (max_cache > 0) {
            // actualizar caché
            strcpy(UltimaEntradaEscritura[CACHE_ULT - max_cache].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT - max_cache].p_inodo = p_inodo;

            max_cache--;
#if DEBUG9
            fprintf(stderr, "[mi_write()-> Actualizamos la caché de escritura\n");
#endif
        } else {
            for (int i = 0; i < CACHE_ULT - 1; i++) {
                strcpy(UltimaEntradaEscritura[i].camino, UltimaEntradaEscritura[i + 1].camino);
                UltimaEntradaEscritura[i].p_inodo = UltimaEntradaEscritura[i + 1].p_inodo;
            }
            strcpy(UltimaEntradaEscritura[CACHE_ULT - 1].camino, camino);
            UltimaEntradaEscritura[CACHE_ULT - 1].p_inodo = p_inodo;
        }
    }
    resultado = mi_write_f(p_inodo, buf, offset, nbytes);
    if (resultado == -1) {
        resultado = 0;
    }
    mi_signalSem();
    return resultado;
}

//////////////////NIVEL 10////////////////////////////

/**
 * @brief Crea el enlace de una entrada de directorio camino2 al inodo especificado por otra entrada de directorio camino1.
 * camino1 y camino2 han de referirse siembre a un fichero.
 *
 * @param camino1   Camino del directorio padre
 * @param camino2   Camino del directorio hijo
 * @return 0 si se ha creado correctamente, -1 en caso contrario
 */
int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem();
    unsigned int p_inodo_dir1 = 0, p_inodo1 = 0, p_entrada1 = 0;
    unsigned int p_inodo_dir2 = 0, p_inodo2 = 0, p_entrada2 = 0;

    // Comprobamos primera entrada
    int error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 6);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return -1;
    }

    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();

        return -1;
    }

    int ninodo1 = p_inodo1;
    struct inodo inodo;
    if (leer_inodo(ninodo1, &inodo) == -1) {
        printf("Error (mi_link) . No se pudo leer el inodo\n");

        mi_signalSem();
        return -1;
    }

    if (inodo.tipo != 'f') {
        printf("Error (mi_link) no es un fichero\n");
        mi_signalSem();
        return -1;
    }

    struct entrada entrada;

    if (mi_read_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(entrada), sizeof(entrada)) == -1) {
        printf("Error (mi_link) ejecutando mi_read_f()\n");
        mi_signalSem();
        return -1;
    }

    liberar_inodo(p_inodo2);

    entrada.ninodo = p_inodo1;

    if (mi_write_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(entrada), sizeof(entrada)) == -1) {
        printf("Error (mi_link) ejecutando mi_write_f()\n");
        mi_signalSem();
        return -1;
    }

    if (leer_inodo(ninodo1, &inodo) == -1) {
        printf("Error (mi_link) . No se pudo leer el inodo\n");
        mi_signalSem();
        return -1;
    }

    inodo.nlinks++;

    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo1, inodo) == -1) {
        printf("Error (mi_link) en escribir el inodo\n");
        mi_signalSem();
        return -1;
    }

    mi_signalSem();
    return 0;
}

/**
 * @brief Función que borra la entrada de directorio espesificada por camino y,
 * en caso de que fuera el último enlace existente, borrar el propio fichero/directorio.
 * Es decir: sirve tanto para borrar un enlace a un fichero como para eliminar un fichero o directorio que no contenga enlaces.
 *
 * @param camino    Camino de la entrada de directorio a borrar
 * @return 0 si se ha borrado correctamente, -1 en caso contrario
 */
int mi_unlink(const char *camino) {
    mi_waitSem();
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int error, numentradas;
    struct inodo inodo;
    struct entrada entrada;
    int tamEntrada = sizeof(struct entrada);
    p_inodo_dir = 0;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) != EXIT_SUCCESS) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return -1;
    }
    if (leer_inodo(p_inodo, &inodo) == -1) {
        mi_signalSem();
        return -1;
    }
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        fprintf(stderr, "Error: El directorio %s no está vacío\n", camino);
        mi_signalSem();
        return -1;
    }
    if (leer_inodo(p_inodo_dir, &inodo) == -1) {
        mi_signalSem();
        return -1;
    }
    // Eliminar la entrada en el directorio
    numentradas = inodo.tamEnBytesLog / tamEntrada;
    if (p_entrada != (numentradas - 1)) {
        if (mi_read_f(p_inodo_dir, &entrada, (numentradas - 1) * tamEntrada, tamEntrada) == -1) {
            mi_signalSem();
            return -1;
        }
        if (mi_write_f(p_inodo_dir, &entrada, p_entrada * tamEntrada, tamEntrada) == -1) {
            mi_signalSem();
            return -1;
        }
    }
    if (mi_truncar_f(p_inodo_dir, inodo.tamEnBytesLog - tamEntrada) == -1) {
        mi_signalSem();
        return -1;
    }
    if (leer_inodo(p_inodo, &inodo) == -1) {
        mi_signalSem();
        return -1;
    }
    inodo.nlinks--;
    if (inodo.nlinks == 0) {
        // Eliminar el inodo
        if (liberar_inodo(p_inodo) == -1) {
            mi_signalSem();
            return -1;
        }
    } else {
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, inodo) == -1) {
            mi_signalSem();
            return -1;
        }
    }
    mi_signalSem();
    return 0;
}