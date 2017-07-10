/*
 * Interrupciones.h
 *
 *  Created on: 9/7/2017
 *      Author: utnso
 */

#ifndef INTERRUPCIONES_H_
#define INTERRUPCIONES_H_


enum{
	SIN_INTERRUPCION,
	FINALIZADO_VOLUNTARIAMENTE,
	RES_EJEC_NEGATIVO,
	STACKOVERFLOW,
	DIRECCION_INVALIDA,
	EXCEPCION_MEMORIA,
};

int interrupcion=SIN_INTERRUPCION;
int procesoFinalizado = 1;

int verificaInterrupcion();
void expropiar();


int verificaInterrupcion(){

	int interrupcionesEnKernel=0;
	char comandoConsultarInterrupciones = 'I';
	send(socketKernel,&comandoConsultarInterrupciones,sizeof(char),0);
	recv(socketKernel,&interrupcionesEnKernel,sizeof(int),0);

	printf("Interrupciones:%d\n",interrupcionesEnKernel);

	if(interrupcionesEnKernel) {
		interrupcion = FINALIZADO_VOLUNTARIAMENTE;
		pcb_actual->exitCode = -8;
	}


	if(interrupcion != SIN_INTERRUPCION) return -1;
	return 0;
}

void expropiar(){


	switch(interrupcion){
	case FINALIZADO_VOLUNTARIAMENTE: expropiadoVoluntariamente();
		break;
	case RES_EJEC_NEGATIVO: expropiarPorKernel();
		break;
	case STACKOVERFLOW: expropiarPorStackOverflow();
		break;
	case DIRECCION_INVALIDA: expropiarPorDireccionInvalida();
		break;
	case EXCEPCION_MEMORIA: /*TODO> Ver como seguir aca*/
		break;
	default: break;

	}
}

void stackOverflow(){
		log_error(logConsolaPantalla, "El proceso ANSISOP de PID %d sufrio stack overflow", pcb_actual->pid);
		interrupcion = STACKOVERFLOW;
}

void direccionInvalida(){
	log_error(logConsolaPantalla,"No se pudo solicitar el contenido\n");
	interrupcion = DIRECCION_INVALIDA;
}

void excepcionMemoria(){ /**TODO: Preguntar que hacer aca*/
	log_error(logConsolaPantalla,"No se pudo almacenar el contenido\n");
	interrupcion = EXCEPCION_MEMORIA;
}

void expropiadoVoluntariamente(){
	log_warning(logConsolaPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por Kernel", pcb_actual->pid);
	char comandoExropiadoVoluntariamente = 'E';

	send(socketKernel,&comandoExropiadoVoluntariamente,sizeof(char),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);

	send(socketKernel,&cantidadInstruccionesEjecutadas,sizeof(int),0);

	free(pcb_actual);

	procesoFinalizado=1;
}


void expropiarPorKernel(){
	log_warning(logConsolaPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por Kernel", pcb_actual->pid);

	serializarPcbYEnviar(pcb_actual,socketKernel);

	send(socketKernel,&cantidadInstruccionesEjecutadas,sizeof(int),0);

	free(pcb_actual);

	procesoFinalizado=1;
}

void expropiarPorDireccionInvalida(){
	log_warning(logConsolaPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por intentar acceder a una referencia en memoria invalida", pcb_actual->pid);
	char interruptHandler= 'X';
	char caseDireccionInvalida= 'M';

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&caseDireccionInvalida,sizeof(char),0);
	serializarPcbYEnviar(pcb_actual,socketKernel);
	send(socketKernel,&cantidadInstruccionesEjecutadas,sizeof(int),0);

	free(pcb_actual);

	procesoFinalizado = 1;
}

void expropiarPorStackOverflow(){
	char interruptHandler= 'X';
	char caseStackOverflow = 'K';

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&caseStackOverflow,sizeof(char),0);

	serializarPcbYEnviar(pcb_actual,socketKernel);
	send(socketKernel,&cantidadInstruccionesEjecutadas,sizeof(int),0);
	log_warning(logConsolaPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por StackOverflow\n", pcb_actual->pid);

	free(pcb_actual);

	procesoFinalizado = 1;
}

#endif /* INTERRUPCIONES_H_ */
