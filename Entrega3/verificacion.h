/**
 * @file verificacion.h
 * @author Álvaro Pimentel, Andreu Marqués
 * @brief Cabecera de verificacion.c
 */

#include "simulacion.h"

#define NUMENTRADAS 100
#define NUMREGISTROS 200

struct INFORMACION
{
   int pid;
   unsigned int nEscrituras; // validadas
   struct REGISTRO PrimeraEscritura;
   struct REGISTRO UltimaEscritura;
   struct REGISTRO MenorPosicion;
   struct REGISTRO MayorPosicion;
};
void escribir_info(struct INFORMACION info, char *buffer);