#include "ficheros.h"




// Definición de errores
#define ERROR_CAMINO_INCORRECTO -1
#define ERROR_PERMISO_LECTURA -2
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA -3
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO -4
#define ERROR_PERMISO_ESCRITURA -5
#define ERROR_ENTRADA_YA_EXISTENTE -6
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO -7

#define COLOR_ERROR "\x1b[91m"
#define COLOR_RESET "\33[0m"

//VARIABLES NIVEL 8
#define TAMFILA 100

#define PROFARBOL 32  //maxima profundidad del árbol de directorios
#define TAMNOMBRE 60 // tamaño del nombre de directorio o fichero, en Ext2 = 256

struct entrada{
    char nombre[TAMNOMBRE];
    unsigned int ninodo;
};

#define TAMENTRADA sizeof(struct entrada)

struct UltimaEntrada
{
    char camino[TAMNOMBRE * PROFARBOL];
    int p_inodo;
};

#define CACHE_ULT 10

// FUNCIONES NIVEL 7
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);

void mostrar_error_buscar_entrada(int error);

void mostrar_buscar_entrada(char *camino, char reservar);

// FUNCIONES NIVEL 8
int mi_creat(const char *camino,unsigned char permisos);
int mi_dir(const char *camino,char *buffer, char tipo);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);

// FUNCIONES NIVEL 9
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);

// FUNCIONES NIVEL 10

int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);