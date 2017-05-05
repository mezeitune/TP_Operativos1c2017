#include "dummy_ansisop.h"
static const char* PROGRAMA =
						"begin\n"
						"variables a, b\n"
						"a = 3\n"
						"b = 5\n"
						"a = b + 12\n"
						"end\n"
						"\n";
AnSISOP_funciones functions = {
	.AnSISOP_definirVariable	= dummy_definirVariable,
	.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
	.AnSISOP_finalizar = dummy_finalizar,
	.AnSISOP_dereferenciar	= dummy_dereferenciar,
	.AnSISOP_asignar	= dummy_asignar,
};

AnSISOP_kernel kernel_functions = { };
char *const conseguirDatosDeLaMemoria(char *start, t_puntero_instruccion offset, t_size i);

//-----------------------------------------------------------------------------------------------------------------





int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logCPU.txt");
	listaPcb = list_create();


	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);



	//Lo primero que habria que hacer en realidad es aca pedirle un PCB al kernel
	//a travez de serializacion y que este me envie uno si es que tiene programas pendientes de CPU
	//en caso de no tener ningun programa pendiente , simplemente se imprime un mensaje
	//Cuando ya tengo ese PCB (yo diria que establecido globalmente en CPU)
	//Le pido info a memoria desplazandome en cuanto a sus instrucciones y ejecutando las primitivas necesarias
	//para responderle al kernel todos los resultados y que se manden a consola

	/* Ejemplo en el dummy sobre como empezar a pedir cosas y leer linea por linea
	 	char *programa = strdup(PROGRAMA);
t_metadata_program *metadata = metadata_desde_literal(programa);
int programCounter = 0;
while(!terminoElPrograma()){
char* const linea = conseguirDatosDeLaMemoria(programa,
metadata->instrucciones_serializado[programCounter].start,
metadata->instrucciones_serializado[programCounter].offset);
printf("\t Evaluando -> %s", linea);
analizadorLinea(linea, &functions, &kernel_functions);
free(linea);
programCounter++;
}
metadata_destruir(metadata);
printf("================\n");
	 */


	signal(SIGUSR1, signalSigusrHandler);//Para mandar la signal:
				//htop->pararse sobre el proceso a mandarle signal->k->SIGUSR1->ENTER
	comenzarEjecucionNuevoPrograma();

	return 0;
}






