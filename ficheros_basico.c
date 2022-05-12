#include "ficheros_basico.h"

struct superbloque SB;

/**
 * @brief Calcula el tamaño en bloques necesario para el mapa de bits
 *
 * @param nbloques  Numero de bloques totales
 * @return int      Tamaño en bloques del mapa de bits
 */
int tamMB(unsigned int nbloques) {
    int size = (nbloques / 8) / BLOCKSIZE;
    int resta = (nbloques / 8) % BLOCKSIZE;
    if (resta != 0) {
        size++;  // hemos incrementado en 1 el resultado de la división entera con 1024 porque el módulo no es 0
    }
    return size;
}

/**
 * @brief Calcula el tamaño en bloques del array de inodos
 *
 * @param ninodos   Número de inodos
 * @return int      Tamaño en bloques del array de inodos
 */
int tamAI(unsigned int ninodos) {
    // int ninodos = nbloques/4; --> El programa mi_mkfs.c le pasará este dato a esta función como parámetro al llamarla
    int size = (ninodos * INODOSIZE) / BLOCKSIZE;
    int resta = (ninodos * INODOSIZE) % BLOCKSIZE;
    if (resta != 0) {
        size++;
    }
    return size;
}

/**
 * @brief Inicializa los datos del superbloque y lo escribe en el SF
 *
 * @param nbloques  Número de bloques totales del disco
 * @param ninodos   Número de inodos totales del disco
 * @return int      0 si todo va bien, -1 si hay algún error
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;       // tocar nivel3
    SB.cantBloquesLibres = nbloques;  // tocar nivel3
    SB.cantInodosLibres = ninodos;    // tocar nivel3
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    return bwrite(posSB, &SB);
}

/**
 * @brief Inicializa el mapa de bits (todos a 0)
 *
 * @return int  0 si todo va bien, -1 si hay algún error
 */
int initMB() {
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, '\0', BLOCKSIZE);
    // Leemos superbloque para obtener las posiciones de los datos
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // El contenido del buffer se escribe en los bloques correspondientes al mapa de bits
    for (size_t i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++) {
        if (bwrite(i, buffer) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
    }

    // Actualizamos mapa de bits
    int bloquesLibres = SB.cantBloquesLibres;
    escribir_bit(posSB, 1);
    bloquesLibres--;

    for (int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++) {
        escribir_bit(i, 1);
        bloquesLibres--;
    }
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        escribir_bit(i, 1);
        bloquesLibres--;
    }

    SB.cantBloquesLibres = bloquesLibres;

    // Actualizamos el superbloque
    if (bwrite(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * @brief Inicializa la lista enlazada de inodos libres
 *
 * @return int  0 si todo va bien, -1 si hay error
 */
int initAI() {
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    // Leemos superbloque para obtener las posiciones de los datos
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    unsigned int contInodos = SB.posPrimerInodoLibre + 1;
    for (size_t i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        for (size_t j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l';  // libre
            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }
        // Escribimos el bloque de inodos en el dispositivo virtual
        if (bwrite(i, inodos) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
    }
    /* se puede quitar
    if (bwrite(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    */
    return 0;
}

//////////////////NIVEL 3//////////////////

/**
 * @brief Esta función escribe el valor indicado por el parámetro bit: 0 (libre) ó 1 (ocupado)
 * en un determinado bit del MB que representa el bloque nbloque. La utilizaremos cada vez que
 * necesitemos reservar o liberar un bloque
 *
 * @param nbloque   Número de bloque del MB
 * @param bit       Valor a escribir en el bit
 * @return int      Devuelve 0 si todo ha ido bien, -1 si hay error
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;

    // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;

    // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
    // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    posbyte = posbyte % BLOCKSIZE;
    unsigned char bufferMB[BLOCKSIZE];

    memset(bufferMB, '\0', BLOCKSIZE);
    bread(nbloqueabs, &bufferMB);

    // utilizaremos una máscara y realizaremos un desplazamiento de bits (tantos como indique el valor posbit)
    unsigned char mascara = 128;  // 10000000

    mascara >>= posbit;  // desplazamiento de bits a la derecha
    if (bit == 1) {
        bufferMB[posbyte] |= mascara;  // operador OR para bits
    } else {
        bufferMB[posbyte] &= ~mascara;  // operadores AND y NOT para bits
    }
    return bwrite(nbloqueabs, bufferMB);
}

/**
 * @brief Lee un determinado bit del MB y devuelve el valor del bit leído en el mapa de bits
 *
 * @param nbloque           Número del bloque del MB que se quiere leer
 * @return unsigned char    Valor del bit leído
 */
unsigned char leer_bit(unsigned int nbloque) {
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;

    // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;

    // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
    // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
    unsigned int nbloqueabs = nbloqueMB + SB.posPrimerBloqueMB;

    posbyte = posbyte % BLOCKSIZE;
    unsigned char bufferMB[BLOCKSIZE];
    memset(bufferMB, '\0', BLOCKSIZE);
    if (bread(nbloqueabs, &bufferMB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\nImposible leer el bloque %d", errno, strerror(errno), nbloqueabs);
        return -1;
    }

    unsigned char mascara = 128;  // 10000000

    mascara >>= posbit;            // Desplazamiento de bits a la derecha
    mascara &= bufferMB[posbyte];  // AND para bits
    mascara >>= (7 - posbit);      // Desplazamiento de bits a la derecha, ahora el bit leido està
    // al final de la mascara
    return mascara;  // Devolvemos el bit leido
}

/**
 * @brief Encuentra el primer bloque libre, consultando el MB (primer bit a 0),
 * lo ocupa (poniendo el correspondiente bit a 1 con la ayuda de la función
 * escribir_bit()) y devuelve su posición
 *
 * @return int  Devuelve el número de bloque que se ha reservado, -1 si hay error
 */
int reservar_bloque() {
    // Leemos el Super Bloque
    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Si no hay bloques libres, acaba con -1
    if (SB.cantBloquesLibres == 0) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nImposible reservar bloque, no quedan libres!\n", errno, strerror(errno));
        return -1;
    }

    unsigned int posBloqueMB = SB.posPrimerBloqueMB;
    unsigned char bufferMB[BLOCKSIZE];  // Buffer de lectura MB
    memset(bufferMB, '\0', BLOCKSIZE);
    unsigned char bufferAUX[BLOCKSIZE];  // Buffer auxiliar(todos 1)
    memset(bufferAUX, 255, BLOCKSIZE);
    int verificar = 1;
    int bit = 0;
    unsigned char mascara = 128;

    // Recorremos los bloques del MB hasta encontrar uno que esté a 0
    for (; posBloqueMB <= SB.posUltimoBloqueMB && verificar > 0 && bit == 0; posBloqueMB++) {
        verificar = bread(posBloqueMB, bufferMB);
        bit = memcmp(bufferMB, bufferAUX, BLOCKSIZE);
    }
    // Una vez encontrado, vamos atràs de 1
    posBloqueMB--;

    if (verificar > 0) {
        for (int posbyte = 0; posbyte < BLOCKSIZE; posbyte++) {
            if (bufferMB[posbyte] < 255) {
                int posbit = 0;
                while (bufferMB[posbyte] & mascara) {
                    bufferMB[posbyte] <<= 1;  // Desplazamos bits a la izquierda
                    posbit++;
                }
                // Ahora posbit contiene el bit = 0
                int nbloque = ((posBloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE + posbyte) * 8 + posbit;
                escribir_bit(nbloque, 1);                         // Marcamos el bloque como reservado
                SB.cantBloquesLibres = SB.cantBloquesLibres - 1;  // Actualizamos cantidad de bloques libres en el SB

                // Guardamos el SB
                if (bwrite(posSB, &SB) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
                    return -1;
                }

                // Limpiamos ese bloque en la zona de datos, grabando un buffer de 0s en la
                // posición del nbloque del dispositivo, por si había basura
                unsigned char bufferVacio[BLOCKSIZE];  // Buffer de ceros
                memset(bufferVacio, '\0', BLOCKSIZE);
                if (bwrite(nbloque, &bufferVacio) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nImposible escribir buffer vacio", errno, strerror(errno));
                    return -1;
                }
                return nbloque;  // Devolvemos el nº de bloque que hemos reservado
            }
        }
    }
    fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nNo es valido", errno, strerror(errno));
    return -1;
}

/**
 * @brief Libera un bloque determinado con la ayuda de la función escribir_bit()
 *
 * @param nbloque   Nº de bloque a liberar
 * @return int      Devuelve nbloque si no hay error, en caso de error devuelve -1
 */
int liberar_bloque(unsigned int nbloque) {
    // Ponemos a 0 el bit del Mapa de Bits correspondiente al bloque nbloque
    if (escribir_bit(nbloque, 0) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Después leemos el Super Bloque
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Sumamos una unidad a la cantidad de bloques libres
    SB.cantBloquesLibres = SB.cantBloquesLibres + 1;
    // Guardamos el SB
    if (bwrite(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Devolvemos el nº de bloque liberado
    return nbloque;
}

// Escribe el contenido de una variable de tipo structinodoen un determinado inodo del array de inodos, inodos
/**
 * @brief Escribe el contenido de una variable de tipo structinodoen un determinado
 * inodo del array de inodos, inodos
 *
 * @param ninodo    Nº de inodo del array de inodos, inodos
 * @param inodo     Puntero a la variable de tipo structinodo
 * @return int      Devuelve -1 si hay error, 0 si no
 */
int escribir_inodo(unsigned int ninodo, struct inodo inodo) {
    // Leemos el superbloque para obtener la localización del array de inodos
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    unsigned int posInodo = SB.posPrimerBloqueAI + ((ninodo * INODOSIZE) / BLOCKSIZE);
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // Escribe el contenido de una variable del tipo struct inodo en un determinado inodo del array de inodos
    bread(posInodo, inodos);  // Leemos el bloque del inodo
    memcpy(&inodos[ninodo % (BLOCKSIZE / INODOSIZE)], &inodo, INODOSIZE);

    return bwrite(posInodo, inodos);
}

/**
 * @brief Lee y devuelve el inodo especificado
 *
 * @param ninodo    Número de inodo a leer
 * @param inodo     Puntero a la variable donde se guardará el inodo leído
 * @return int      Devuelve -1 en caso de error, 0 en caso contrario
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    unsigned int posInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    int comprobar = bread(posInodo, inodos);
    *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
    return (comprobar < 0) ? -1 : 0;
}

/**
 * @brief Permite reservar un nuevo inodo y asignarle tipo y permisos
 * y actualiza la lista de inodos libres
 *
 * @param tipo      Tipo de inodo
 * @param permisos  Permisos de inodo
 * @return int      Devuelve el nº de inodo reservado
 */
int reservar_inodo(unsigned char tipo, char permisos) {
    // encuentra el primer idondo libre (dato alamcenado en el SB), lo reserva (con
    // la ayuda de la funcuion escribir_inodo(), devuelve su numero y actualiza la
    // lista de inodos libres)
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Si no hay inodos libres, error
    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\nNingún inodo libre!", errno, strerror(errno));
        return -1;
    }

    int posInodoReservado = SB.posPrimerInodoLibre;

    struct inodo in;
    leer_inodo(posInodoReservado, &in);

    // Actualización superbloque
    SB.posPrimerInodoLibre = in.punterosDirectos[0];
    SB.cantInodosLibres = SB.cantInodosLibres - 1;
    if (bwrite(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\nNo se ha podido actualizar el SB!", errno, strerror(errno));
        return -1;
    }

    in.tipo = tipo;
    in.permisos = permisos;
    in.nlinks = 1;
    in.tamEnBytesLog = 0;
    in.atime = time(NULL);
    in.ctime = time(NULL);
    in.mtime = time(NULL);
    in.numBloquesOcupados = 0;
    memset(in.punterosDirectos, 0, sizeof(in.punterosDirectos));
    memset(in.punterosIndirectos, 0, sizeof(in.punterosIndirectos));
    escribir_inodo(posInodoReservado, in);

    return posInodoReservado;
}

//////////////////NIVEL 4//////////////////

/**
 * @brief Devuelve el rango de bloques lògicos
 *
 * @param inodo     Puntero al inodo
 * @param nblogico  Nùmero de bloque lògico
 * @param ptr       Puntero a la direcciòn de memoria donde se almacenará el rango
 * @return int      Devuelve entre 0-3 si todo va bien, -1 si hay algún error
 */
int obtener_nrangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    } else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    } else {
        *ptr = 0;
        fprintf(stderr, "Error en ficheros_basico.c obtener_nrangoBL()\nBloque lógico nblogico=%d fuera de rango --> %d: %s\n", nblogico, errno, strerror(errno));
        return -1;
    }
}

/**
 * @brief Permite obtener el indice de un bloque en funciòn de su nivel sale como int pero como no toma valores
 * negativos para ahorrar memoria empleamos unsigned int
 *
 * @param nblogico          Nùmero de bloque lògico
 * @param nivel_punteros    Nivel del puntero
 * @return int              Devuelve el indice del bloque en funciòn de su nivel
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) {  // si se encuentra en el primer nivel
        return nblogico;
    } else if (nblogico < INDIRECTOS0) {  // si se encuentra en el segundo nivel
        return nblogico - DIRECTOS;
    } else if (nblogico < INDIRECTOS1) {  // si se encuentra en el tercer nivel
        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    } else if (nblogico < INDIRECTOS2) {  // si se encuentra en el 4 nivel
        if (nivel_punteros == 3) {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }
    fprintf(stderr, "Error en ficheros_basico.c obtener_indice() --> %d: %s\n", errno, strerror(errno));
    return -1;
}

/**
 * @brief Esta función se encarga de obtener el nº de bloque físico correspondiente
 * a un bloque lógico determinado del inodo indicado. Enmascara la gestión de los
 * diferentes rangos de punteros directos e indirectos del inodo, de manera que
 * funciones externas no tienen que preocuparse de cómo acceder a los bloques
 * físicos apuntados desde el inodo.
 *
 * @param ninodo    Nº de inodo del que se quiere obtener el bloque
 * @param nblogico  Nº de bloque lógico del que se quiere obtener el bloque físico
 * @param reservar  Indica si se debe reservar el bloque físico si no existe
 * @return int      Nº de bloque físico correspondiente al bloque lógico indicado
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, int reservar) {
    struct inodo inodo;
    unsigned int ptr, ptr_ant, salvar_inodo, nRangoBL, nivel_punteros, indice;
    int buffer[NPUNTEROS];

    if (leer_inodo(ninodo, &inodo) == -1) return -1;

    ptr = 0, ptr_ant = 0, salvar_inodo = 0;
    nRangoBL = obtener_nrangoBL(&inodo, nblogico, &ptr);  // 0:D, 1:I0, 2:I1, 3:I2
    nivel_punteros = nRangoBL;                            // nivel_punteros = nRangoBL
    while (nivel_punteros > 0) {
        if (ptr == 0) {
            if (reservar == 0) {
                // no es necesario imprimir por pantalla
                // fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> reservar = 0\n");
                return -1;
            } else {
                salvar_inodo = 1;

                if ((ptr = reservar_bloque()) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nptr ==-1", errno, strerror(errno));
                    return -1;
                }
                inodo.numBloquesOcupados++;
                inodo.ctime = time(NULL);  // se actualiza ctime a fecha actual
                if (nivel_punteros == nRangoBL) {
                    printf("[traducir_bloque_inodo()→ inodo.punterosIndirectos[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nRangoBL - 1, ptr, ptr, nivel_punteros);
                    inodo.punterosIndirectos[nRangoBL - 1] = ptr;

                } else {
                    buffer[indice] = ptr;
                    printf("[traducir_bloque_inodo()→ inodo.punteros_nivel%i[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nivel_punteros, indice, ptr, ptr, nivel_punteros);
                    if (bwrite(ptr_ant, buffer) == -1) {
                        fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bwrite", errno, strerror(errno));
                        return -1;
                    }
                }
                memset(buffer, 0, BLOCKSIZE);
            }
        } else {
            if (bread(ptr, buffer) == -1) {
                fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bread", errno, strerror(errno));
                return -1;
            }
        }
        if ((indice = obtener_indice(nblogico, nivel_punteros)) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nobtenerindice=-1", errno, strerror(errno));
            return -1;
        }
        ptr_ant = ptr;
        ptr = buffer[indice];
        nivel_punteros--;
    }  // al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) {  // punteros directos
        if (reservar == 0) {
            // fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> reservar = 0 (error de lectura bloque)\n");
            return -1;  // error lectura no existe el bloque
        } else {
            salvar_inodo = 1;
            if ((ptr = reservar_bloque()) == -1) {
                fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nptr=reservarbloque", errno, strerror(errno));
                return -1;
            }  // de datos
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            if (nRangoBL == 0) {
                printf("[traducir_bloque_inodo()→ inodo.punterosDirectos[%i] = %i (reservado BF %i para BL %i)]\n", nblogico, ptr, ptr, nblogico);
                inodo.punterosDirectos[nblogico] = ptr;  //
            } else {
                buffer[indice] = ptr;  // (imprimirlo)
                printf("[traducir_bloque_inodo()→ inodo.punteros_nivel1[%i] = %i (reservado BF %i para BL %i)]\n", indice, ptr, ptr, nblogico);
                if (bwrite(ptr_ant, buffer) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nbwrite ptr_ant", errno, strerror(errno));
                    return -1;
                }
            }
        }
    }
    if (salvar_inodo == 1) {
        if (escribir_inodo(ninodo, inodo) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nescribirinodo", errno, strerror(errno));
            return -1;
        }  // sólo si lo hemos actualizado
    }
    return ptr;  // nº de bloque físico correspondiente al bloque de datos lógico, nblogic
}

//////////////////NIVEL 6//////////////////

/**
 * @brief Permite liberar un inodo reservado
 *
 * @param ninodo    nº de inodo a liberar
 * @return int      ninodo si todo va bien, -1 si hay error
 */
int liberar_inodo(unsigned int ninodo) {
    struct inodo inodo;
    // Leer el inodo
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }
    // Llamar a la función auxiliar liberar_bloques_inodo() para liberar todos los bloques del inodo
    int bloq_Liberados = liberar_bloques_inodo(ninodo, 0);
    // Leer el inodo actualizado
    if (leer_inodo(ninodo, &inodo) == -1) {
        return -1;
    }

    // A la cantidad de bloques ocupados del inodo, inodo.numBloquesOcupados, se le restará
    // la cantidad de bloques liberados por la función anterior (y debería quedar a 0).
    if (inodo.numBloquesOcupados - bloq_Liberados == 0) {
        inodo.tipo = 'l';  // Marcar el inodo como tipo libre
        inodo.tamEnBytesLog = 0;
    } else {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo()\n El nùmero de bloques ocupados por el inodo liberado y los bloques liberados no son los mismos! Error --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // Actualizar la lista enlaxada de inodos libres

    // Leer el superbloque
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Incluir el inodo que queremos liberar en la lista de inodos libres
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre = ninodo;

    // En el superbloque, incrementar la cantidad de inodos libres
    SB.cantInodosLibres++;

    // Escribir el inodo actualizado
    escribir_inodo(ninodo, inodo);

    // Escribir el superbloque actualizado
    if (bwrite(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Devolver el nº del inodo liberado
    return ninodo;
}

/**
 * @brief La función liberar_bloques_inodo() libera todos los bloques ocupados a partir del bloque lógico indicado por el argumento primerBL
 *
 * @param ninodo        Número de inodo del que se desea liberar los bloques
 * @param nblogico      nº de bloque lógico
 * @return liberados    devuelve la cantidad de bloques liberados
 */
int liberar_bloques_inodo(unsigned int ninodo, unsigned int nblogico) {
    // Variables
    struct inodo inodo;
    unsigned int nRangoBL, nivel_punteros, indice, ptr, nblog, ultimoBL, tamInodo;
    int bloques_punteros[3][NPUNTEROS];
    int indices[3];
    int ptr_nivel[3];
    int liberados, salvar_inodo;
    unsigned char bufferAUX[BLOCKSIZE];
    memset(bufferAUX, 0, BLOCKSIZE);

    // Procedimiento
    liberados = 0;
    salvar_inodo = 0;
    leer_inodo(ninodo, &inodo);
    tamInodo = inodo.tamEnBytesLog;
    // Disabilitar este if si se hacen pruebas con leer_sf
    /*if (tamInodo == 0) {
        printf("El fichero estaba vacío, return 0 en liberar_bloques_inodo()\n");
        return 0;  // Fichero vacìo
    }*/
    // Obtenemos el ùltimo bloque lògico del inodo
    if (tamInodo % BLOCKSIZE == 0) {
        ultimoBL = tamInodo / BLOCKSIZE - 1;
    } else {
        ultimoBL = tamInodo / BLOCKSIZE;
    }
    ptr = 0;
    // Recorrido BLs
    for (nblog = nblogico; nblog <= ultimoBL; nblog++) {
        nRangoBL = obtener_nrangoBL(&inodo, nblog, &ptr);
        if (nRangoBL < 0) {
            fprintf(stderr, "Error en ficheros_basico.c liberar_bloques_inodo() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
        nivel_punteros = nRangoBL;  // El nivel_punteros +alto cuelga del inodo
        while (ptr > 0 && nivel_punteros > 0) {
            bread(ptr, bloques_punteros[nivel_punteros - 1]);
            indice = obtener_indice(nblog, nivel_punteros);
            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }
        if (ptr > 0) {
            liberar_bloque(ptr);
            liberados++;
            if (nRangoBL == 0) {
                inodo.punterosDirectos[nblog] = 0;
                salvar_inodo = 1;
            } else {
                while (nivel_punteros < nRangoBL) {
                    indice = indices[nivel_punteros];
                    bloques_punteros[nivel_punteros][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros];
                    if (memcmp(bloques_punteros[nivel_punteros], bufferAUX, BLOCKSIZE) == 0) {
                        // No cuelgan bloques ocupados, hay que liberar el bloque de punteros
                        liberar_bloque(ptr);
                        liberados++;
                        nivel_punteros++;
                        if (nivel_punteros == nRangoBL) {
                            inodo.punterosIndirectos[nRangoBL - 1] = 0;
                            salvar_inodo = 1;
                        }
                    } else {
                        // Escribimos en el dispositivo el bloque de punteros modificado
                        bwrite(ptr, bloques_punteros[nivel_punteros]);
                        nivel_punteros = nRangoBL;  // Salimos del bucle
                    }
                }
            }
        }
    }
    if (salvar_inodo == 1) {
        escribir_inodo(ninodo, inodo);
    }
    return liberados;
}
