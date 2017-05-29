/*
 * configuraciones.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef CONFIGURACIONES_H_
#define CONFIGURACIONES_H_


void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
t_config* configuracion_kernel;

char *quantum;
char *quantumSleep;
char *algoritmo;
char *semIds;
char *semInit;
char *sharedVars;
int stackSize;
int config_paginaSize; // todo: Tiene que obtenerlo por handshake con memoria
int config_gradoMultiProgramacion;
int gradoMultiProgramacion;
char *ipServidor;
char *ipMemoria;
char *ipFileSys;
char *puertoServidor;
char *puertoMemoria;
char *puertoFileSys;



void imprimirConfiguraciones() {
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP FS:%s\nPUERTO FS:%s\n",ipMemoria,puertoMemoria,ipFileSys,puertoFileSys);
	printf("---------------------------------------------------\n");
	printf(	"QUANTUM:%s\nQUANTUM SLEEP:%s\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%d\nPAGINA_SIZE:%d\n",	quantum, quantumSleep, algoritmo, config_gradoMultiProgramacion, semIds, semInit, sharedVars, stackSize, config_paginaSize);
	printf("---------------------------------------------------\n");

}

void leerConfiguracion(char* ruta) {

	configuracion_kernel = config_create(ruta);

	ipServidor = config_get_string_value(configuracion_kernel, "IP_SERVIDOR");
	puertoServidor = config_get_string_value(configuracion_kernel,"PUERTO_SERVIDOR");
	ipMemoria = config_get_string_value(configuracion_kernel, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel, "IP_FS");
	puertoFileSys = config_get_string_value(configuracion_kernel, "PUERTO_FS");
	quantum = config_get_string_value(configuracion_kernel, "QUANTUM");
	quantumSleep = config_get_string_value(configuracion_kernel,"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion_kernel, "ALGORTIMO");
	config_gradoMultiProgramacion = atoi(config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION"));
	semIds = config_get_string_value(configuracion_kernel, "SEM_IDS");
	semInit = config_get_string_value(configuracion_kernel, "SEM_INIT");
	sharedVars = config_get_string_value(configuracion_kernel, "SHARED_VARS");
	stackSize = atoi(config_get_string_value(configuracion_kernel, "STACK_SIZE"));
	config_paginaSize = atoi(config_get_string_value(configuracion_kernel,"PAGINA_SIZE"));
}


#endif /* CONFIGURACIONES_H_ */
