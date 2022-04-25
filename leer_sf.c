#include "ficheros_basico.h"
void datos_inodo(struct inodo inodo);
int main(int argc, char const *argv[])
{
    char nombre_dispositivo[1024];
    // struct inodo inodo_reservado;
    // int ninodo;
    struct superbloque SB;
    if (argc != 2)
    {
        fprintf(stderr, "Sintaxis: ./leer_sf <nombre_dispositivo>\n");
        return -1;
    }
    
    if (bmount(argv[1]) == -1){
        fprintf(stderr,"Error while mounting\n");
        return -1;

    }
    if (bread(posSB, &SB) == -1){
        fprintf(stderr,"Error while reading superblock\n");
        exit(EXIT_FAILURE);
    }
    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n\n", SB.totInodos);

    if (bumount(argv[1]) == -1)
        exit(EXIT_FAILURE);
    return 0;
}