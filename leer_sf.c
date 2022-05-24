#include "directorios.h"

#define DEBUG 1
#define DEBUG2 0

struct superbloque SB;

struct inodo inodos[BLOCKSIZE / INODOSIZE];

int InfoSB();
int InfoListaEnlazada();
int InfoMB();
int InfoInodo();
int InfoTraducirBloqueInodo();
void mostrar_buscar_entrada(char *camino, char reservar);

int main(int argc, char **argsv) {
    if (argc == 2) {
        if (bmount(argsv[1]) == -1) {
            fprintf(stderr, "Error al montar el dispositivo\n");
            return -1;
        }
        // Mostrar información del superbloque
        InfoSB();
#if DEBUG2
        // Mostrar información de la lista enlazada
        InfoListaEnlazada();
#endif

#if DEBUG

        printf("sizeof struct superbloque: %li\n", sizeof(struct superbloque));
        printf("sizeof struct inodo: %li\n\n", sizeof(struct inodo));
#endif

#if DEBUG
        // Mostrar información del mapa de bits
        #if DEBUG2
        InfoMB();
        #endif
        // Mostrar información de los inodos
        InfoInodo();
#endif

#if DEBUG
        // Mostrar información de la traducción de BloquesInodo
        InfoTraducirBloqueInodo();
#endif
#if DEBUG2
        // Mostrar creación directorios y errores
        mostrar_buscar_entrada("pruebas/", 1);            // ERROR_CAMINO_INCORRECTO
        mostrar_buscar_entrada("/pruebas/", 0);           // ERROR_NO_EXISTE_ENTRADA_CONSULTA
        mostrar_buscar_entrada("/pruebas/docs/", 1);      // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
        mostrar_buscar_entrada("/pruebas/", 1);           // creamos /pruebas/
        mostrar_buscar_entrada("/pruebas/docs/", 1);      // creamos /pruebas/docs/
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
        // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
        mostrar_buscar_entrada("/pruebas/", 1);           // ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/docs/doc1", 0);  // consultamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/casos/", 1);     // creamos /pruebas/casos/
        mostrar_buscar_entrada("/pruebas/docs/doc2", 1);  // creamos /pruebas/docs/doc2
#endif
        if (bumount() == -1) {
            fprintf(stderr, "Error desmontando dispositivo virtual.\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Syntax error: ./leer_sf <nombre_dispositivo>\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Muestra la creación y consulta de directorios
 * 
 * @param camino    Camino a crear o consultar
 * @param reservar  Indica si se debe reservar una entrada
 */
void mostrar_buscar_entrada(char *camino, char reservar) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}

/**
 * @brief Muestra la información del superbloque
 * 
 * @return int 
 */
int InfoSB() {
    // Se lee el superbloques
    if (bread(0, &SB) == -1) {
        return -1;
    }
    printf("DATOS DEL SUPERBLOQUE:\n");
    printf("posPrimerBloqueMB: %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB: %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI: %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI: %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos: %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos: %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz: %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre: %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres: %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres: %d\n", SB.cantInodosLibres);
    printf("totBloques: %d\n", SB.totBloques);
    printf("totInodos: %d\n\n", SB.totInodos);
    return 0;
}

/**
 * @brief  Imprime el recorrido de la lista enlazada de inodos libres
 * 
 * @return int 
 */
int InfoListaEnlazada() {
    printf("Recorrido de la lista de inodos libres:\n");
    int indiceInodos = SB.posPrimerInodoLibre + 1;  // Cantidad de Inodos
    int ultimoInodo = 0;
    for (int i = SB.posPrimerBloqueAI; (i <= SB.posUltimoBloqueAI) && (ultimoInodo == 0); i++) {  // Recorrido de todos los inodos
        if (bread(i, inodos) == -1) {
            fprintf(stderr, "Error while reading");
            return -1;
        }
        for (int j = 0; j < (BLOCKSIZE / INODOSIZE); j++) {  // Recorrido de cada uno de los inodos
            if (indiceInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = indiceInodos;  // Declaramos la conexion con el siguiente inodo
                indiceInodos++;
                printf("%d ", inodos[j].punterosDirectos[0]);
            } else {  // Estamos al final
                inodos[j].punterosDirectos[0] = UINT_MAX;
                ultimoInodo = 1;
                printf("%d ", inodos[j].punterosDirectos[0]);
                break;
            }
        }
    }
    printf("\n");
    return 0;
}

/**
 * @brief  Muestra la información del Mapa de Bits de
 *
 * @return int
 */
int InfoMB() {
    printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS:\n");
    int reservado = reservar_bloque();  // Actualiza el SB
    bread(posSB, &SB);                  // Actualizar los valores del SB
    printf("Se ha reservado el bloque físico nº %i que era el 1º libre indicado por el MB.\n", reservado);
    printf("SB.cantBloquesLibres = %i\n", SB.cantBloquesLibres);
    liberar_bloque(reservado);
    bread(posSB, &SB);  // Actualizar los valores del SB
    printf("Liberamos ese bloque, y después SB.cantBloquesLibres = %i\n\n", SB.cantBloquesLibres);
    printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
    int bit = leer_bit(posSB);
    printf("leer_bit(%i) = %i\n\n", posSB, bit);
    bit = leer_bit(SB.posPrimerBloqueMB);
    printf("leer_bit(%i) = %i\n\n", SB.posPrimerBloqueMB, bit);
    bit = leer_bit(SB.posUltimoBloqueMB);
    printf("leer_bit(%i) = %i\n\n", SB.posUltimoBloqueMB, bit);
    bit = leer_bit(SB.posPrimerBloqueAI);
    printf("leer_bit(%i) = %i\n\n", SB.posPrimerBloqueAI, bit);
    bit = leer_bit(SB.posUltimoBloqueAI);
    printf("leer_bit(%i) = %i\n\n", SB.posUltimoBloqueAI, bit);
    bit = leer_bit(SB.posPrimerBloqueDatos);
    printf("leer_bit(%i) = %i\n\n", SB.posPrimerBloqueDatos, bit);
    bit = leer_bit(SB.posUltimoBloqueDatos);
    printf("leer_bit(%i) = %i\n\n", SB.posUltimoBloqueDatos, bit);
    
    return 0;
}

/**
 * @brief  Muestra la información de los inodos
 *
 * @return int
 */
int InfoInodo() {
    printf("\nDATOS DEL DIRECTORIO RAIZ\n");
    struct tm *time;
    char atime[80];
    char mtime[80];
    char ctime[80];
    struct inodo inodo;
    int ninodo = SB.posInodoRaiz;  // el directorio raiz es el inodo 0
    leer_inodo(ninodo, &inodo);
    time = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", time);
    time = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", time);
    time = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", time);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %i\n", inodo.permisos);
    printf("ID: %d \nATIME: %s \nMTIME: %s \nCTIME: %s\n", ninodo, atime, mtime, ctime);
    printf("nlinks: %i\n", inodo.nlinks);
    printf("Tamaño en bytes lógicos: %i\n", inodo.tamEnBytesLog);
    printf("Número de bloques ocupados: %i\n", inodo.numBloquesOcupados);
    
    return 0;
}

/**
 * @brief Muestra la traducción de los bloques lógicos a partir de los offset proporcionados
 *
 * @return int
 */
int InfoTraducirBloqueInodo() {
    int inodoReservado = reservar_inodo('f', 6);
    bread(posSB, &SB);

    printf("\nINODO %d - TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n", inodoReservado);
    traducir_bloque_inodo(inodoReservado, 8, 1);
    traducir_bloque_inodo(inodoReservado, 204, 1);
    traducir_bloque_inodo(inodoReservado, 30004, 1);
    traducir_bloque_inodo(inodoReservado, 400004, 1);
    traducir_bloque_inodo(inodoReservado, 468750, 1);

    printf("\nDATOS DEL INODO RESERVADO: %d\n", inodoReservado);
    struct tm *time;
    char atime[80];
    char mtime[80];
    char ctime[80];
    struct inodo inodo;
    leer_inodo(inodoReservado, &inodo);  // Leemos el Inodo reservado
    time = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", time);
    time = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", time);
    time = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", time);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %i\n", inodo.permisos);
    printf("ATIME: %s \nMTIME: %s \nCTIME: %s\n", atime, mtime, ctime);
    printf("nlinks: %i\n", inodo.nlinks);
    printf("Tamaño en bytes lógicos: %i\n", inodo.tamEnBytesLog);
    printf("Número de bloques ocupados: %i\n", inodo.numBloquesOcupados);
    printf("SB.posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    
    return 0;
}