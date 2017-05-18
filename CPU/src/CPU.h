#include "PCB.h"


AnSISOP_funciones functions = {  //TODAS LAS PRIMITIVAS TIENEN QUE ESTAR ACA
	.AnSISOP_definirVariable	= dummy_definirVariable,
	.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
	.AnSISOP_finalizar = dummy_finalizar,
	.AnSISOP_dereferenciar	= dummy_dereferenciar,
	.AnSISOP_asignar	= dummy_asignar,
	/*
	 .AnSISOP_obtenerValorCompartida
	 .AnSISOP_asignarValorCompartida
	 .AnSISOP_irAlLabel
	 .AnSISOP_llamarSinRetorno
	 .AnSISOP_llamarConRetorno
	 .AnSISOP_retornar
	 */
};

AnSISOP_kernel kernel_functions = {/*
		.AnSISOP_wait
		.AnSISOP_signal
		.AnSISOP_reservar
		.AnSISOP_liberar
		.AnSISOP_abrir
		.AnSISOP_borrar
		.AnSISOP_cerrar
		.AnSISOP_moverCursor
		.AnSISOP_escribir
		.AnSISOP_leer
		*/

};
//-----------------------------------------------------------------------------------------------------------------
char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i);
char* obtener_instruccion(t_pcb * pcb);

int almacenarDatosEnMemoria(t_pcb* pcb,char* buffer, int size);
char* conseguirDatosMemoria (t_pcb* pcb, int paginaSolicitada, int size,int offset);

//-----------------------------------------------------------------------------------------------------------------
void establecerPCB();
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(t_pcb pcb);
void cargarPcbActual(t_pcb* pidEstructura, int pid, int cantidadPaginas, int offset);
void recibirTamanioPagina();
void recibirPCB();
void signalSigusrHandler(int signum);

void finalizar();

void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden);
void connectionHandlerKernel(int socketAceptado, char orden);
void leerInstrucciones(t_pcb* pcb);
void interfazHandler(t_pcb * pcb);
//-----------------------------------------------------------------------------------------------------------------
t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;

//----------------------------//

//------------------Sockets Globales-------//
int socketMemoria;
int socketKernel;
//-----------------------------------------//
t_list* listaPcb;
pthread_t HiloConexionMemoria;
int tamanio_pagina;
int cpuOcupada=1;
