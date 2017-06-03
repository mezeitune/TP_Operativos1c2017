#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "conexiones.h"
#include "dummy_ansisop.h"
#include <parser/metadata_program.h>
#include "PCB.h"
//-----------------------------------------------------------------------------------------------------------------
char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i);
char* obtener_instruccion(t_pcb * pcb);
int almacenarDatosEnMemoria(t_pcb* pcb,char* buffer, int size,int paginaAGuardar,int offset);
int conseguirDatosMemoria (char** instruccion, t_pcb* pcb, int paginaSolicitada,int offset,int size);

//-----------------------------------------------------------------------------------------------------------------
void establecerPCB();
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(t_pcb pcb);
void cargarPcbActual(t_pcb* pidEstructura, int pid, int cantidadPaginas, int offset);
void recibirTamanioPagina();
void recibirPCB();
void signalHandler(int signum);
void imprimirPCB(t_pcb * pcb);
int cantidadPaginasTotales(t_pcb * pcb);
void esperarPCB();
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden);
void connectionHandlerKernel(int socketAceptado, char orden);
void ejecutarInstruccion(t_pcb* pcb);
void interfazHandler(t_pcb * pcb);
void EjecutarProgramaMedianteAlgoritmo(t_pcb* pcb);
void expropiarPorQuantum(t_pcb * pcb);
void CerrarPorSignal();
void stackOverflow();
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
int cpuOcupada=1;
int cpuFinalizada=1;


//-------------------------------------------------------------------------PRIMITIVAS------------------------------------//
//---------Primitivas Comunes----------//
t_puntero definirVariable(t_nombre_variable variable);
t_puntero obtenerPosicionVariable(t_nombre_variable variable);
void asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable dereferenciar(t_puntero puntero);
void finalizar();
void retornar(t_valor_variable retorno);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void irAlLabel(t_nombre_etiqueta etiqueta);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);

//---------Primitivas Kernel----------//
void wait(t_nombre_semaforo identificador_semaforo);
void escribir(t_descriptor_archivo descriptor_archivo, t_valor_variable valor, t_valor_variable tamanio);
void signal_Ansisop(t_nombre_semaforo identificador_semaforo);
t_puntero reservar (t_valor_variable espacio);
//-------------------------------------------------------------------------PRIMITIVAS------------------------------------//

AnSISOP_funciones functions = {  //TODAS LAS PRIMITIVAS TIENEN QUE ESTAR ACA
	.AnSISOP_definirVariable	=definirVariable,
	.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
	.AnSISOP_finalizar =finalizar,
	.AnSISOP_dereferenciar	= dereferenciar,
	.AnSISOP_asignar	= asignar,
	 .AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
	 .AnSISOP_asignarValorCompartida = asignarValorCompartida,
	 .AnSISOP_irAlLabel = irAlLabel,
	 .AnSISOP_llamarSinRetorno=llamarSinRetorno,
	 .AnSISOP_llamarConRetorno = llamarConRetorno,
	 .AnSISOP_retornar = retornar
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait= wait,

		.AnSISOP_signal = signal_Ansisop,
		.AnSISOP_reservar = reservar,
		/*.AnSISOP_liberar
		.AnSISOP_abrir
		.AnSISOP_borrar
		.AnSISOP_cerrar
		.AnSISOP_moverCursor
		*/.AnSISOP_escribir = escribir
		//.AnSISOP_leer


};

