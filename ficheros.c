#include "ficheros.h"

/**
 * @brief Escribe el contenido procedente de un buffer de memoria, buf_original, de tamaño nbytes,
 * en un fichero/directorio (correspondiente al inodo pasado como argumento, ninodo): le
 * indicamos la posición de escritura inicial en bytes lógicos, offset, con respecto al inodo, y
 * el número de bytes, nbytes, que hay que escribir.
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param buf_original  Buffer de memoria con el contenido a escribir
 * @param offset        Posición de escritura inicial en bytes lógicos
 * @param nbytes        Número de bytes a escribir
 * @return int          Devuelve el nbytes, o -1 en caso de error
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico;
    char buf_bloque[BLOCKSIZE];
    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error en mi_write_f(), no existen permisos de escritura\npermisos inodo: %d  ||  %d: %s\n",inodo.permisos, errno, strerror(errno));
        return -1;
    }
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;
    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (bread(nbfisico, buf_bloque) == -1) {
            return -1;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1) {
            return -1;
        }
    } else {  // Más de un bloque involucrado

        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (bread(nbfisico, buf_bloque) == -1) {
            return -1;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == -1) {
            return -1;
        }
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 1);
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE) == -1) {
                return -1;
            }
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (bread(nbfisico, buf_bloque) == -1) {
            return -1;
        }

        memcpy(buf_bloque, buf_original + (nbytes - desp2 - 1), desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == -1) {
            return -1;
        }
    }
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    inodo.mtime = time(NULL);
    if ((offset + nbytes) >= inodo.tamEnBytesLog) {
        inodo.ctime = time(NULL);
        inodo.tamEnBytesLog = offset + nbytes;
    }
    if (escribir_inodo(ninodo, inodo) == -1) {
        return -1;
    }
    return nbytes;
}

/**
 * @brief Lee información de un fichero/directorio (correspondiente al nº de inodo, ninodo, pasado
 * como argumento) y la almacena en un buffer de memoria, buf_original: le indicamos la
 * posición de lectura inicial offset con respecto al inodo (en bytes) y el número de bytes
 * nbytes que hay que leer.
 *
 * @param ninodo            Número de inodo del fichero/directorio
 * @param buf_original      Buffer de memoria donde se almacena la información leída
 * @param offset            Posición de lectura inicial en bytes lógicos
 * @param nbytes            Número de bytes a leer
 * @return int              Devuelve leidos, o -1 en caso de error
 *
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned int primerBL, ultimoBL, desp1, desp2, leidos;
    char buf_bloque[BLOCKSIZE];
    int nbfisico;
    struct inodo inodo;
    leidos = 0;

    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    if ((inodo.permisos & 4) != 4) {  // Esta operación sólo está permitida cuando haya permiso de lectura
    // Esta operación
        fprintf(stderr, "Error en mi_read_f()\nNo existen permisos de lectura, permisos del inodo: %d ||  %d: %s\n", inodo.permisos, errno, strerror(errno));
        return -1;
    }

        if (offset >= inodo.tamEnBytesLog) {
            return leidos;  // No podemos leer nada
        }
        if ((offset + nbytes) >= inodo.tamEnBytesLog) {  // pretende leer más allá de EOF
            nbytes = inodo.tamEnBytesLog - offset;       // leemos sólo los bytes que podemos desde el offset hasta EOF
        }
    
    
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    memset(buf_bloque, 0, sizeof(buf_bloque));
    nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
    if (primerBL == ultimoBL)  // Un solo bloque involucrado
    {
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        }
        leidos = nbytes;
    } else {  // Más de un bloque involucrado

        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                return -1;
            }
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        }
        leidos = BLOCKSIZE - desp1;
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 0);
            if (nbfisico != -1) {
                if (bread(nbfisico, buf_bloque) == -1) {
                    return -1;
                }
                memcpy(buf_original + leidos, buf_bloque, BLOCKSIZE);
            }
            leidos += BLOCKSIZE;
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == -1) {
                return -1;
            }
            memcpy(buf_original + leidos, buf_bloque, desp2 + 1);
        }
        leidos += desp2 + 1;
    }
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1) {
        return -1;
    }
    return leidos;
}

/**
 * @brief Devuelve la metainformación de un fichero/directorio (correspondiente al nº de inodo
 * pasado como argumento): tipo, permisos, cantidad de enlaces de entradas en directorio,
 * tamaño en bytes lógicos, timestamps y cantidad de bloques ocupados en la zona de
 * datos, es decir todos los campos menos los punteros
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param STAT          Puntero a estructura de tipo STAT que almacena la información
 * @return int          Devuelve 0 si todo va bien, o -1 en caso de error
 */
int mi_stat_f(unsigned int ninodo, struct STAT *STAT) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    STAT->tipo = inodo.tipo;
    STAT->permisos = inodo.permisos;
    STAT->atime = inodo.atime;
    STAT->mtime = inodo.mtime;
    STAT->ctime = inodo.ctime;
    STAT->nlinks = inodo.nlinks;
    STAT->tamEnBytesLog = inodo.tamEnBytesLog;
    STAT->numBloquesOcupados = inodo.numBloquesOcupados;

    return 0;
}

/**
 * @brief Cambia los permisos de un fichero/directorio (correspondiente al nº de inodo pasado
 * como argumento, ninodo) con el valor que indique el argumento permisos
 *
 * @param ninodo        Número de inodo del fichero/directorio
 * @param permisos      Nuevos permisos
 * @return int          Devuelve 0 si todo va bien, o -1 en caso de error
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, inodo) == -1) {
        return -1;
    }
    return 0;
}

/**
 * @brief Trunca un fichero/directorio (correspondiente al nº de inodo, ninodo, pasado
 * como argumento) a los bytes indicados como nbytes, liberando los bloques necesarios
 *
 * @param ninodo    Número de inodo del fichero/directorio
 * @param nbytes    Número de bytes a los que se quiere truncar
 * @return int      Devuelve la cantidad de bloques liberados o -1 si hay un error o la operación no está permitida.
 */
int truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;
    unsigned int nblogico;
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    // Miramos si tenemos permisos
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error en ficheros.c mi_truncar_f() --> Permisos denegados de escritura\n");
        return -1;
    } else {
        if (inodo.tamEnBytesLog <= nbytes) {
            fprintf(stderr, "Error en ficheros.c mi_truncar_f() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
        // Calculamos nblogico
        if (nbytes % BLOCKSIZE == 0) {
            nblogico = nbytes / BLOCKSIZE;
        } else {
            nblogico = nbytes / BLOCKSIZE + 1;
        }
        int bloq_Liberados = liberar_bloques_inodo(ninodo, nblogico);
        inodo.numBloquesOcupados = inodo.numBloquesOcupados - bloq_Liberados;
        // Actualizamos mtime, ctime y tamEnBytesLog
        inodo.mtime = time(NULL);
        inodo.ctime = time(NULL);
        inodo.tamEnBytesLog = nbytes;
        escribir_inodo(ninodo, inodo);
        return bloq_Liberados;
    }
    fprintf(stderr, "Error en ficheros.c mi_truncar_f() --> %d: %s\n", errno, strerror(errno));
    return -1;
}