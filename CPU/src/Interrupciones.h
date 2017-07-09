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
};

int interrupcion=SIN_INTERRUPCION;
int procesoFinalizado = 1;

int verificaInterrupcion();
void expropiar();


int verificaInterrupcion(){


	if(interrupcion != SIN_INTERRUPCION) return -1;
	return 0;
}

void expropiar(){


	switch(interrupcion){
	case FINALIZADO_VOLUNTARIAMENTE: expropiarPorKernel();
		break;
	case RES_EJEC_NEGATIVO: expropiarPorKernel();
		break;
	default: break;

	}
}

void expropiarPorKernel(){
	log_warning(logConsolaPantalla, "El proceso ANSISOP de PID %d ha sido expropiado por Kernel", pcb_actual->pid);

	serializarPcbYEnviar(pcb_actual,socketKernel);

	send(socketKernel,&cantidadInstruccionesEjecutadas,sizeof(int),0);

	free(pcb_actual);

	procesoFinalizado=1;
}
#endif /* INTERRUPCIONES_H_ */
