/**
 * @file ficheros_basico.c
 * @author Álvaro Pimentel, Andreu Marqués
 *
 */
#include "ficheros_basico.h"

#define DEBUG3 0
#define DEBUG4 0
#define DEBUG6 0 
/**
 * @brief Calcula el tamaño en bloques necesario para el mapa de bits
 *
 * @param nbloques  Numero de bloques totales
 * @return int      Tamaño en bloques del mapa de bits
 */
int tamMB(unsigned int nbloques)
{
    return (((nbloques / 8) % BLOCKSIZE) != 0) ? (((nbloques / 8) / BLOCKSIZE) + 1) : ((nbloques / 8) / BLOCKSIZE);
}

/**
 * @brief Calcula el tamaño en bloques del array de inodos
 *
 * @param ninodos   Número de inodos
 * @return int      Tamaño en bloques del array de inodos
 */
int tamAI(unsigned int ninodos)
{
    return (((ninodos * INODOSIZE) % BLOCKSIZE) != 0) ? (((ninodos * INODOSIZE) / BLOCKSIZE) + 1) : ((ninodos * INODOSIZE) / BLOCKSIZE);
}

/**
 * @brief Inicializa los datos del superbloque y lo escribe en el SF
 *
 * @param nbloques  Número de bloques totales del disco
 * @param ninodos   Número de inodos totales del disco
 * @return int      0 si todo va bien, -1 si hay algún error
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;

    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;       
    SB.cantBloquesLibres = nbloques;  
    SB.cantInodosLibres = ninodos;    
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    if (bwrite(posSB, &SB) == -1) {
        return -1;
    }
    return 0;
}

/**
 * @brief Inicializa el mapa de bits (todos a 0)
 *
 * @return int  0 si todo va bien, -1 si hay algún error
 */
int initMB() {
    struct superbloque SB;

    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);
    // Leemos superbloque para obtener las posiciones de los datos
    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    
    // El contenido del buffer se escribe en los bloques correspondientes al mapa de bits para inicializarlo
    for (int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++)
   {
      if (bwrite(i, buffer) == -1)
         return -1;
   }
    
    for (int i = posSB; i < SB.posUltimoBloqueAI; i++) {
        if (escribir_bit(i,1) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
    }

     SB.cantBloquesLibres -= SB.posUltimoBloqueAI + 1;
    // Escribimos en los bloques correspondientes
    if (bwrite(posSB,&SB) == -1)return -1; 
    return 0;
}

/**
 * @brief Inicializa la lista enlazada de inodos libres
 *
 * @return int  0 si todo va bien, -1 si hay error
 */
int initAI() {
    // buffer para recorrer el array de inodos
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    struct superbloque SB;
    unsigned int contInodos;
    // Leemos superbloque para obtener las posiciones de los datos
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    contInodos = SB.posPrimerInodoLibre + 1;
    int final = 0;
    for (int i = SB.posPrimerBloqueAI; (i <= SB.posUltimoBloqueAI) && final == 0; i++) {
        for (int j = 0; j < (BLOCKSIZE / INODOSIZE); j++) {
            inodos[j].tipo = 'l';  // libre

            // en caso de no haber llegado al inodo final
            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                // se ha llegado al final
                inodos[j].punterosDirectos[0] = UINT_MAX;
                final = 1;
                break;
            }
        }
        // Escribimos el bloque de inodos en el dispositivo virtual
        if (bwrite(i, &inodos) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
    }

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
    struct superbloque SB;

    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    unsigned char bufferMB[BLOCKSIZE];
    // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;

    // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    
    // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
    // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    if (bread(nbloqueabs, bufferMB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    posbyte = posbyte % BLOCKSIZE;


    // utilizaremos una máscara y realizaremos un desplazamiento de bits (tantos como indique el valor posbit)
    // 10000000
    unsigned char mascara = 128;

    mascara >>= posbit;  // desplazamiento de bits a la derecha
    if (bit == 0) {
        // operadores AND y NOT para bits
        bufferMB[posbyte] &= ~mascara;
    } else if (bit == 1) {
        //  operador OR para bits
        bufferMB[posbyte] |= mascara;
    } else {
        return -1;
    }

    if (bwrite(nbloqueabs, bufferMB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief Lee un determinado bit del MB y devuelve el valor del bit leído en el mapa de bits
 *
 * @param nbloque           Número del bloque del MB que se quiere leer
 * @return char             Valor del bit leído
 */
char leer_bit(unsigned int nbloque) {
    struct superbloque SB;

    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    unsigned char bufferMB[BLOCKSIZE];

    // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;

    // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;

    // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
    // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;


    if (bread(nbloqueabs, bufferMB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\nImposible leer el bloque %d", errno, strerror(errno), nbloqueabs);
        return -1;
    }

    posbyte = posbyte % BLOCKSIZE;

    unsigned char mascara = 128;  // 10000000

    mascara >>= posbit;            // Desplazamiento de bits a la derecha
    mascara &= bufferMB[posbyte];  // AND para bits
    mascara >>= (7 - posbit);      // Desplazamiento de bits a la derecha, ahora el bit leido està
    // al final de la mascara

#if DEBUG3
    printf("[leer_bit(%i) → posbyte:%i, posbit:%i, nbloqueMB:%i, nbloqueabs:%i)]\n", nbloque, posbyte, posbit, nbloqueMB, nbloqueabs);
#endif

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
    unsigned char bufferMB[BLOCKSIZE], bufferAUX[BLOCKSIZE];  // Buffer de lectura MB y AUX
    
    struct superbloque SB;

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
    memset(bufferAUX, 255, BLOCKSIZE);

    unsigned int nbloqueabs = SB.posPrimerBloqueMB;

    int libre = 0;
    while (libre == 0) {
        if (bread(nbloqueabs, bufferMB) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
        if (memcmp(bufferMB, bufferAUX, BLOCKSIZE) != 0) {
            libre = 1;
            break;
        }
        nbloqueabs++;
    }
    unsigned int posbyte = 0;
    while (bufferMB[posbyte] == 255) {
        posbyte++;
    }
    unsigned char mascara = 128;  // 10000000
    unsigned int posbit = 0;
    // encontrar el primer bit a 0 en ese byte
    while (bufferMB[posbyte] & mascara)  // operador AND para bits
    {
        bufferMB[posbyte] <<= 1;  // desplazamiento de bits a la izquierda
        posbit++;
    }
    unsigned int nbloque = ((nbloqueabs - SB.posPrimerBloqueMB) * BLOCKSIZE + posbyte) * 8 + posbit;
    if (escribir_bit(nbloque, 1) == -1) {
        return -1;
    }
    SB.cantBloquesLibres--;
    memset(bufferAUX, 0, BLOCKSIZE);

    if (bwrite(SB.posPrimerBloqueDatos + nbloque - 1, bufferAUX) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    if (bwrite(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return nbloque;
}

/**
 * @brief Libera un bloque determinado con la ayuda de la función escribir_bit()
 *
 * @param nbloque   Nº de bloque a liberar
 * @return int      Devuelve nbloque si no hay error, en caso de error devuelve -1
 */
int liberar_bloque(unsigned int nbloque) {
    struct superbloque SB;

    // Leemos el Super Bloque
    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // Ponemos a 0 el bit del Mapa de Bits correspondiente al bloque nbloque
    if (escribir_bit(nbloque, 0) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Sumamos una unidad a la cantidad de bloques libres
    SB.cantBloquesLibres++;

    // Guardamos el SB
    if (bwrite(posSB, &SB) == -1) {
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
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    struct superbloque SB;

    // Leemos el superbloque para obtener la localización del array de inodos

    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    unsigned int nbloqueabs = (ninodo * INODOSIZE) / BLOCKSIZE + SB.posPrimerBloqueAI;

    // Escribe el contenido de una variable del tipo struct inodo en un determinado inodo del array de inodos
    // Leemos el bloque del inodo
    if (bread(nbloqueabs, inodos) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // Escribimos el inodo en el array de inodos
    inodos[ninodo % (BLOCKSIZE / INODOSIZE)] = inodo;

    // Escribimos el array de inodos en el bloque correspondiente
    if (bwrite(nbloqueabs, inodos) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief Lee y devuelve el inodo especificado
 *
 * @param ninodo    Número de inodo a leer
 * @param inodo     Puntero a la variable donde se guardará el inodo leído
 * @return int      Devuelve -1 en caso de error, 0 en caso contrario
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    struct superbloque SB;

    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    unsigned int nbloqueabs = (ninodo * INODOSIZE) / BLOCKSIZE + SB.posPrimerBloqueAI;

    if (bread(nbloqueabs, inodos) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c leer_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
    return 0;
}

/**
 * @brief Permite reservar un nuevo inodo y asignarle tipo y permisos
 * y actualiza la lista de inodos libres
 *
 * @param tipo      Tipo de inodo
 * @param permisos  Permisos de inodo
 * @return int      Devuelve el nº de inodo reservado
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    struct superbloque SB;
    struct inodo in;

    // encuentra el primer inodo libre (dato alamcenado en el SB), lo reserva (con
    // la ayuda de la funcuion escribir_inodo(), devuelve su numero y actualiza la
    // lista de inodos libres)
    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    // Si no hay inodos libres, error
    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\nNingún inodo libre!", errno, strerror(errno));
        return -1;
    }

    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    // Actualizamos el SB

    SB.cantInodosLibres--;
    SB.posPrimerInodoLibre++;

    // creación del inodo
    in.tipo = tipo;
    in.permisos = permisos;
    in.nlinks = 1;
    in.tamEnBytesLog = 0;
    in.atime = time(NULL);
    in.ctime = time(NULL);
    in.mtime = time(NULL);
    in.numBloquesOcupados = 0;
    memset(in.punterosDirectos, 0, 12 * sizeof(unsigned int));
    memset(in.punterosIndirectos, 0, 3 * sizeof(unsigned int));

    // escribimos el inodo en el array de inodos
    if (escribir_inodo(posInodoReservado, in) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    // escribimos el SB
    if (bwrite(posSB, &SB) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    return posInodoReservado;
}

//////////////////NIVEL 4//////////////////

/**
 * @brief Devuelve el rango de bloques lògicos
 *
 * @param inodo     Puntero al inodo
 * @param primerBL  Nùmero de bloque lògico
 * @param ptr       Puntero a la direcciòn de memoria donde se almacenará el rango
 * @return int      Devuelve entre 0-3 si todo va bien, -1 si hay algún error
 */
int obtener_nrangoBL(struct inodo *inodo, unsigned int primerBL, unsigned int *ptr) {
    if (primerBL < DIRECTOS) {
        *ptr = inodo->punterosDirectos[primerBL];
        return 0;
    } else if (primerBL < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    } else if (primerBL < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    } else if (primerBL < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    } else {
        *ptr = 0;
        fprintf(stderr, "Error en ficheros_basico.c obtener_nrangoBL()\nBloque lógico primerBL=%d fuera de rango --> %d: %s\n", primerBL, errno, strerror(errno));
        return -1;
    }
}

/**
 * @brief Permite obtener el indice de un bloque en funciòn de su nivel sale como int pero como no toma valores
 * negativos para ahorrar memoria empleamos unsigned int
 *
 * @param primerBL          Nùmero de bloque lògico
 * @param nivel_punteros    Nivel del puntero
 * @return int              Devuelve el indice del bloque en funciòn de su nivel
 */
int obtener_indice(unsigned int primerBL, int nivel_punteros) {
    if (primerBL < DIRECTOS) {  // si se encuentra en el primer nivel
        return primerBL;
    } else if (primerBL < INDIRECTOS0) {  // si se encuentra en el segundo nivel
        return primerBL - DIRECTOS;
    } else if (primerBL < INDIRECTOS1) {  // si se encuentra en el tercer nivel
        if (nivel_punteros == 2) {
            return (primerBL - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return (primerBL - INDIRECTOS0) % NPUNTEROS;
        }
    } else if (primerBL < INDIRECTOS2) {  // si se encuentra en el 4 nivel
        if (nivel_punteros == 3) {
            return (primerBL - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return ((primerBL - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return ((primerBL - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }

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
 * @param primerBL  Nº de bloque lógico del que se quiere obtener el bloque físico
 * @param reservar  Indica si se debe reservar el bloque físico si no existe
 * @return int      Nº de bloque físico correspondiente al bloque lógico indicado
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int primerBL, unsigned char reservar) {
    struct inodo inodo;
    unsigned int ptr;
    int ptr_ant, salvar_inodo, nRangoBL, nivel_punteros, indice;
    int buffer[NPUNTEROS];

    // se lee el inodo
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    ptr = 0;
    ptr_ant = 0;
    salvar_inodo = 0;

    nRangoBL = obtener_nrangoBL(&inodo, primerBL, &ptr);  // 0:D, 1:I0, 2:I1, 3:I2

    nivel_punteros = nRangoBL;
    
    // nivel_punteros = nRangoBL
    while (nivel_punteros > 0) {
        if (ptr == 0) {
            if (reservar == 0) {
                // no es necesario imprimir por pantalla
                // fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> reservar = 0\n");
                return -1;
            } else {
                salvar_inodo = 1;

                ptr = reservar_bloque();
                inodo.numBloquesOcupados++;
                inodo.ctime = time(NULL);  // se actualiza ctime a fecha actual

                if (nivel_punteros == nRangoBL) {

                    // el bloque estará colgado directamente del inodo
                    inodo.punterosIndirectos[nRangoBL - 1] = ptr;
#if DEBUG4
                    printf("[traducir_bloque_inodo()→ inodo.punterosIndirectos[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nRangoBL - 1, ptr, ptr, nivel_punteros);
#endif

                } else {
                    // el bloque estara colgando de otro bloque de punteros

                    buffer[indice] = ptr;
#if DEBUG4
                    printf("[traducir_bloque_inodo()→ inodo.punteros_nivel%i[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nivel_punteros, indice, ptr, ptr, nivel_punteros);
#endif

                    // se guarda el buffer de punteros modificado
                    if (bwrite(ptr_ant, buffer) == -1) {
                        fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bwrite", errno, strerror(errno));
                        return -1;
                    }
                }
            }
        }

        if (bread(ptr, buffer) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bread", errno, strerror(errno));
            return -1;
        }
    
        indice = obtener_indice(primerBL, nivel_punteros);
        ptr_ant = ptr;
        ptr = buffer[indice];
        nivel_punteros--;
    }  // al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) {  // punteros directos
        if (reservar == 0) {
            return -1;  // error lectura no existe el bloque de datos
        } else {
            salvar_inodo = 1;
            ptr = reservar_bloque();
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            if (nRangoBL == 0) {
                inodo.punterosDirectos[primerBL] = ptr;
#if DEBUG4
                printf("[traducir_bloque_inodo()→ inodo.punterosDirectos[%i] = %i (reservado BF %i para BL%i)]\n", primerBL, ptr, ptr, primerBL);
#endif

            } else {
                buffer[indice] = ptr;  // (imprimirlo)
#if DEBUG4
                printf("[traducir_bloque_inodo()→ inodo.punteros_nivel1[%i] = %i (reservado BF %i para BL%i)]\n", indice, ptr, ptr, primerBL);
#endif
                if (bwrite(ptr_ant, buffer) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nbwrite ptr_ant", errno, strerror(errno));
                    return -1;
                }
            }
        }
    }
    if (salvar_inodo == 1) {
        // sólo si lo hemos actualizado
        if (escribir_inodo(ninodo, inodo) == -1) {
            fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nescribirinodo", errno, strerror(errno));
            return -1;
        }
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
    struct superbloque SB;
    struct inodo inodo;
    // Leer el inodo
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo--> %d: %s\nleer_inodo", errno, strerror(errno));
        return -1;
    }
    // liberar todos los bloques del inodo y restar cantidad bloques liberados
    inodo.numBloquesOcupados -= liberar_bloques_inodo(0, &inodo);

    // Combrobar si los inodos se han liberado
    if (inodo.numBloquesOcupados != 0) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo--> %d: %s\nno se han liberado los bloques del inodo", errno, strerror(errno));
        return -1;
    }

    // se actualizan los atribitos del inodo
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;

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
    if (escribir_inodo(ninodo, inodo) == -1) {
        fprintf(stderr, "Error en ficheros_basico.c liberar_inodo() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

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
 * @param primerBL      nº de bloque lógico
 * @return liberados    devuelve la cantidad de bloques liberados
 */
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    int nRangoBL, liberados = 0;
    unsigned int nivel_punteros, indice, ptr = 0, nblog, ultimoBL;

    unsigned int bloques_punteros[3][NPUNTEROS];
    unsigned char bufferAUX[BLOCKSIZE];
    int indices[3];
    int ptr_nivel[3];
    

    // el fichero se encientra vacío
    if ((inodo->tamEnBytesLog) == 0) {
        return 0;
    }

    // Obtenemos el último bloque lógico del inodo
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = ((inodo->tamEnBytesLog) / BLOCKSIZE) - 1;
    } else {
        ultimoBL = (inodo->tamEnBytesLog) / BLOCKSIZE;
    }
    // Si el bloque lógico es menor que el último bloque lógico, liberar los bloques
    memset(bufferAUX, 0, BLOCKSIZE);
#if DEBUG6
    printf("[liberar_bloques_inodo()→ primerBL = %d, ultimoBL = %d]\n", primerBL, ultimoBL);
#endif

    // Recorrido BLs
    for (nblog = primerBL; nblog <= ultimoBL; nblog++) {
        // indice :  0:D, 1:I0, 2:I1, 3:I2
        nRangoBL = obtener_nrangoBL(inodo, nblog, &ptr);
        if (nRangoBL < 0) {
            fprintf(stderr, "Error en ficheros_basico.c liberar_bloques_inodo() --> %d: %s\n", errno, strerror(errno));
            return -1;
        }
        nivel_punteros = nRangoBL;  // El nivel_punteros +alto cuelga del inodo
        while ((ptr > 0) && (nivel_punteros > 0)) {
            indice = obtener_indice(nblog, nivel_punteros);
            if ((indice == 0) || (nblog == primerBL)) {
                // únicamen se lee el dispositibo si no está ya cargado previamente en un buffer
                if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == -1) {
                    fprintf(stderr, "Error en ficheros_basico.c liberar_bloques_inodo() --> %d: %s\n", errno, strerror(errno));
                    return -1;
                }
            }
            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }

        if (ptr > 0) {
            liberar_bloque(ptr);
            liberados++;
#if DEBUG6
            printf("[liberar_bloques_inodo()→ liberados = %d, ptr = %d]\n", ptr, nblog);
#endif
            if (nRangoBL == 0) {
                inodo->punterosDirectos[nblog] = 0;

            } else {
                nivel_punteros = 1;
                while (nivel_punteros <= nRangoBL) {
                    indice = indices[nivel_punteros - 1];
                    bloques_punteros[nivel_punteros - 1][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros - 1];
                    if (memcmp(bloques_punteros[nivel_punteros - 1], bufferAUX, BLOCKSIZE) == 0) {
                        // No cuelgan bloques ocupados, hay que liberar el bloque de punteros
                        liberar_bloque(ptr);
                        liberados++;
#if DEBUG6
                        printf("[liberar_bloques_inodo()→ liberado BF %i de punteros_nivel%i correspondiente al BL: %i]\n", ptr, nivel_punteros, nblog);
#endif

                        if (nivel_punteros == nRangoBL) {
                            inodo->punterosIndirectos[nRangoBL - 1] = 0;
                        }
                        nivel_punteros++;
                    } else {
                        // Escribimos en el dispositivo el bloque de punteros modificado
                        if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == -1) {
                            fprintf(stderr, "Error en ficheros_basico.c liberar_bloques_inodo() --> %d: %s\n", errno, strerror(errno));
                            return -1;
                        }

                        nivel_punteros = nRangoBL + 1;  // Salimos del bucle
                    }
                }
            }
        }
    }
#if DEBUG6
    printf("[liberar_bloques_inodo()→ total bloques liberados = %d]\n", liberados);
#endif

    return liberados;
}
