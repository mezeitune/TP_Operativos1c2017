
#ifndef DUMMY_ANSISOP_H_
#define DUMMY_ANSISOP_H_
#include <parser/parser.h>
#include <stdio.h>
#include <stdbool.h>

t_puntero dummy_definirVariable(t_nombre_variable variable);
t_puntero dummy_obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable dummy_dereferenciar(t_puntero puntero);
void dummy_asignar(t_puntero puntero, t_valor_variable variable);
void dummy_finalizar(void);
bool terminoElPrograma(void);



#endif

