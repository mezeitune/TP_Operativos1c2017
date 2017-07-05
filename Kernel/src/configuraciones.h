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

int config_quantum;
int config_quantumSleep;
char *config_algoritmo;
char **semId;
char **semInit;
char **shared_vars;
int stackSize;
int config_paginaSize;
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
	printf(	"QUANTUM:%d\nQUANTUM SLEEP:%d\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%d\nPAGINA_SIZE:%d\n",	config_quantum, config_quantumSleep, config_algoritmo, config_gradoMultiProgramacion, semId, semInit, shared_vars, stackSize, config_paginaSize);
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
	config_quantum = atoi(config_get_string_value(configuracion_kernel, "QUANTUM"));
	config_quantumSleep = atoi(config_get_string_value(configuracion_kernel,"QUANTUM_SLEEP"));
	config_algoritmo = config_get_string_value(configuracion_kernel, "ALGORITMO");
	config_gradoMultiProgramacion = atoi(config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION"));
	semId = config_get_array_value(configuracion_kernel, "SEM_IDS");
	semInit = config_get_array_value(configuracion_kernel, "SEM_INIT");
	shared_vars = config_get_array_value(configuracion_kernel, "SHARED_VARS");
	stackSize = atoi(config_get_string_value(configuracion_kernel, "STACK_SIZE"));
}


#endif /* CONFIGURACIONES_H_ */
