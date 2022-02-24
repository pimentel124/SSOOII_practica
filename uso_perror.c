// uso_perror.c - Funciones perror() y fprintf() con stderr y errno
 
#include <stdio.h>
#include <sys/file.h>
#include <errno.h> //errno
#include <string.h> //strerror()
 
int main()
{
   int fd;
   fd=open("/asdf", O_RDONLY); //No existe el fichero
   if (fd==-1) {          
       fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));  //o bien:
       perror("Error");
   }
   if ((fd=open("/", O_WRONLY))==-1) {
       fprintf(stderr, "Error %d: %s\n", errno, strerror(errno)); //o bien:
       perror("Error");
   }  
}
