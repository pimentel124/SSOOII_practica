#include "directorios.h"
#define DEBUG8 1
// Programa que cambia los permisos de un fichero o directorio, llamando a la función mi_chmod().
// Por parametros entrada "int argc, char **argv".
// Variables usadas en la funcion "unsigned int permisos".
// Por parametros salida "0" caso SUCCESS "-1" caso FAILURE.
int main(int argc, char **argv)
{
    // Comprobamos sintaxis
	if(argc != 4)
    { 
        fprintf(stderr, COLOR_ERROR "Error Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n" COLOR_RESET);
        return -1;
    }
    unsigned char perm = atoi(argv[2]);
    // Miramos los permisos que sean validos
    if((perm <= 7) && (perm >= 0))
    {
    	// Montamos el disco
        if(bmount(argv[1])==-1)
        {
            printf("Error (mi_chmod.c) en montar el disco \n");
            return -1;
        }
        #if DEBUG8
        switch (perm)
        {
        case 0:
            printf("Cambio a permisos: %d, Sin permisos", perm);
            break;
        case 1:
            printf("Cambio a permisos: %d, Solo ejecucion", perm);
            break;
        case 2:
            printf("Cambio a permisos: %d, Solo escritura", perm);
            break;
        case 3:
            printf("Cambio a permisos: %d, Solo escritura y ejecucion", perm);
            break;
        case 4:
            printf("Cambio a permisos: %d, Solo lectura", perm);
            break;
        case 5:
            printf("Cambio a permisos: %d, Solo lectura y ejecucion", perm);
            break;
        case 6:
            printf("Cambio a permisos: %d, Solo escritura y escritura", perm);
            break;
        case 7:
            printf("Cambio a permisos: %d, Todos los permisos", perm);
            break;
        
        
        default:
            break;
        }
        #endif
        // Cambiamos los permisos
   		int a = mi_chmod(argv[3], perm);
        if(a < 0)
        {
            printf("Error al cambiar los permisos\n");
            return -1;
        }
        // Desmontamos el disco
        bumount();
        return 0;
    }
    printf("El valor de permisos esta fuera del rango");
    return -1;
}