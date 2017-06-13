#ifndef _CAPAFS_
#define _CAPAFS_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/mman.h>


#include <commons/bitarray.h>


unsigned char *mmapDeBitmap;
t_bitarray * bitarray;

char *puertoFS;
char *puntoMontaje;
char *puerto_Kernel;
char *tamanioBloques;
char *cantidadBloques;
char* magicNumber;
char *ipFS;
t_config* configuracion_FS;

void testeommap(){
	unsigned char *f;
    int size;
    struct stat s;
    const char * file_name = "../metadata/Bitmap.bin";
    int fd = open ("../metadata/Bitmap.bin", O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;
    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    int i;
    for (i = 0; i < size; i++) {
        char c;

        c = f[i];
        printf("%c",c);
    }


    //Obtener el page size:
   // int pagesize = getpagesize();
    //size = s.st_size;
    //size += pagesize-(size%pagesize);
}

void inicializarMmap(){
	int size;
	struct stat s;
	const char * file_name = "../metadata/Bitmap.bin";
	int fd = open ("../metadata/Bitmap.bin", O_RDONLY);

	/* Get the size of the file. */
	int status = fstat (fd, & s);
	mmapDeBitmap = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
}



void leerConfiguracion(char* ruta){
	configuracion_FS = config_create(ruta);

	puertoFS= config_get_string_value(configuracion_FS,"PUERTO_FS");
	ipFS= config_get_string_value(configuracion_FS, "IP_FS");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");
	puerto_Kernel= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
}





void leerConfiguracionMetadata(char* ruta){
	configuracion_FS = config_create(ruta);

	tamanioBloques= config_get_string_value(configuracion_FS,"TAMANIO_BLOQUES");
	cantidadBloques= config_get_string_value(configuracion_FS, "CANTIDAD_BLOQUES");
	magicNumber = config_get_string_value(configuracion_FS,"MAGIC_NUMBER");


}


void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP FS:%s\nPUERTO FS:%s\nPUNTO MONTAJE:%s\n",ipFS,puertoFS,puntoMontaje);
		printf("\n \nTAMANIO BLOQUS:%s\nCANTIDAD BLQOUES:%s\nMAGIC NUMBER:%s\n",tamanioBloques,cantidadBloques,magicNumber);
		printf("---------------------------------------------------\n");
}



#endif


