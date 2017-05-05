
#include "dummy_ansisop.h"
static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;





bool termino = false;
t_puntero dummy_definirVariable(t_nombre_variable variable) {
printf("definir la variable %c\n", variable);
return POSICION_MEMORIA;
}
t_puntero dummy_obtenerPosicionVariable(t_nombre_variable variable) {
printf("Obtener posicion de %c\n", variable);
return POSICION_MEMORIA;
}
void finalizar (){

	//comando para  avisarle al kernel que debe eliminar
	char comandoInicializacion = 'F';

	void* pcbAEliminar= malloc(sizeof(char) + sizeof(int) * 2); //pido memoria para el comando que deba usar el kernel + los 2 int de la estructura del pcb

	//agarro el pcb que quiero eliminar
	pcbAUtilizar *unPcbAEliminar = list_get(listaPcb,0);

	//pido memoria para ese pcb
	pcbAUtilizar *infoPcbAEliminar = malloc(sizeof(pcbAUtilizar));

	//asigno pcb en memoria
	unPcbAEliminar->pid = infoPcbAEliminar->pid;
	unPcbAEliminar->cantidadPaginas = infoPcbAEliminar->cantidadPaginas;

	printf("%d%d",infoPcbAEliminar->pid,unPcbAEliminar->cantidadPaginas);

	serializarPCByEnviar(socketKernel,comandoInicializacion,&unPcbAEliminar,pcbAEliminar);

	list_destroy_and_destroy_elements(listaPcb, free);

	counterPCBAsignado=0;
}
bool terminoElPrograma(void){
return termino;
}
t_valor_variable dummy_dereferenciar(t_puntero puntero) {
printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
return CONTENIDO_VARIABLE;
}
void dummy_asignar(t_puntero puntero, t_valor_variable variable) {
printf("Asignando en %d el valor %d\n", puntero, variable);
}
