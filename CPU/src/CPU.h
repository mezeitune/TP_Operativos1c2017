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
static const char* PROGRAMA =
						"begin\n"
						"variables a, b\n"
						"a = 3\n"
						"b = 5\n"
						"a = b + 12\n"
						"end\n"
						"\n";
typedef struct{
	int pagina;
	int offset;
	int size;
}__attribute__((packed)) t_posMemoria;


typedef struct{
	char idVar;
	t_posMemoria* dirVar;
} t_variable;

typedef struct{
	t_list* args; //Lista de argumentos. Cada posicion representa un argumento en el orden de la lista
	t_list* vars; // Lista de t_variable
	int retPos;
	t_posMemoria* retVar;
} t_nodoStack;


typedef struct {
		int pid;
		t_puntero_instruccion programCounter;
		int cantidadPaginasCodigo;
		int cantidadInstrucciones;
		int** indiceCodigo;
		int cantidadEtiquetas;
		char* indiceEtiquetas;
		t_list* indiceStack;
		int exitCode;
	}pcbAUtilizar;


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
char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i);

//-----------------------------------------------------------------------------------------------------------------

void recibirPCByEstablecerloGlobalmente(int socketKernel);//Falta implementar , y que reciba con serializacion desde Kernel
									//En caso de que el PCB se haya seteado en 0 , deberia quedar a la espera
									//de nuevos PCB hasta poder ejecutar algo
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler(int socketKernel);
void cargarPcbActual(pcbAUtilizar* pidEstructura, int pid, int cantidadPaginas, int offset);
void comenzarEjecucionNuevoPrograma();
void signalSigusrHandler(int signum);
void serializarPCByEnviar(int socket, char comandoInicializacion, pcbAUtilizar *unPcbAEliminar, void* pcbAEliminar);
void finalizar();
pcbAUtilizar* recibirYDeserializarPcb(int socketKernel);
void deserializarStack(void* pcbSerializado, t_list** indiceStack);
//utilizacion de la memoria
int pedirBytesMemoria(pcbAUtilizar* pcb);
int almacenarDatosEnMemoria(pcbAUtilizar* pcb,char* buffer, int size);
int pedirBytesYAlmacenarEnMemoria();
void conseguirDatosMemoria (pcbAUtilizar* pcb, int paginaSolicitada, int size);
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden);
int calcularIndiceCodigoSize(int cantidadInstrucciones);
int calcularIndiceEtiquetasSize(int cantidadEtiquetas);
void connectionHandlerKernel(int socketAceptado, char orden);
//utilizacion de la memoria


t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;


//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *loggerSinPantalla;
t_log *loggerConPantalla;
//----------------------------//



//------------------Sockets Globales-------//
int socketMemoria;
int socketKernel;
//-----------------------------------------//
t_list* listaPcb;
pthread_t HiloConexionMemoria;


int counterPCBAsignado=0;//Cuando esto incremente a 1 , significa que ya recibio un PCB correcto
					//si queda en 0 significa que no hay todavia. Cuando la CPU se libere del PCB actual porque
					//ya realizo todas sus operaciones correspondientes , entonces se vuelve a setear en 0
