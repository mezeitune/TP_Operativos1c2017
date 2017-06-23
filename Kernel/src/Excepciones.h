/*
 * Excepciones.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef EXCEPCIONES_H_
#define EXCEPCIONES_H_
#define CANTIDADEXCEPCIONES 10
typedef struct{
	int value;
	char* mensaje;
}t_exitCode;

enum {
	EXIT_OK,
	EXIT_RESOURCE,
	EXIT_FILE_NOT_FOUND,
	EXIT_READ_PERMISSIONS,
	EXIT_WRITE_PERMISSIONS,
	EXIT_MEMORY_EXCEPTION,
	EXIT_DISCONNECTED_CONSOLE,
	EXIT_END_OF_PROCESS,
	EXIT_PAGE_OVERSIZE,
	EXIT_PAGE_LIMIT
};


t_exitCode* exitCodeArray [CANTIDADEXCEPCIONES];



#endif /* EXCEPCIONES_H_ */
