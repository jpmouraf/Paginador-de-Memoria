#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "uvm.h"

int main(void) {
	size_t PAGESIZE = sysconf(_SC_PAGESIZE);
	uvm_create();
	
	// Aloca 5 páginas para forçar remoções com 4 frames
	char *page0 = uvm_extend();
	char *page1 = uvm_extend();
	char *page2 = uvm_extend();
	char *page3 = uvm_extend();
	char *page4 = uvm_extend();
	
	// Escreve em todas as páginas para ativar o bit dirty
	for (int i = 0; i < PAGESIZE; i++) {
		page0[i] = 'A';
		page1[i] = 'B';
		page2[i] = 'C';
		page3[i] = 'D';
	}
	
	// Page fault usando algoritmo de relógio para uma página dirty, e com todas as proteções ativadas
	for (int i = 0; i < PAGESIZE; i++) {
		page4[i] = 'E';
	}
	
	// Lê páginas removidas para verificar escrita/leitura do disco 
	printf("Reading back page0[0]: %c\n", page0[0]);
	printf("Reading back page1[100]: %c\n", page1[100]);
	printf("Reading back page2[500]: %c\n", page2[500]);
	
	// Verifica integridade dos dados
	uvm_syslog(page0, 10);
	uvm_syslog(page4, 10);
	
	exit(EXIT_SUCCESS);
}
