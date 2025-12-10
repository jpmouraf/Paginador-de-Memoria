#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "uvm.h"

int main(void) {
	uvm_create();
	
	// Aloca 5 páginas (mais que os 4 frames disponíveis)
	char *page0 = uvm_extend();
	char *page1 = uvm_extend();
	char *page2 = uvm_extend();
	char *page3 = uvm_extend();
	char *page4 = uvm_extend();
	
	// Acessa page0 apenas para leitura */
	printf("page0[0] = %c\n", page0[0]);
	
	// Escreve nas páginas 1, 2, 3 para torná-las dirty
	page1[0] = '1';
	page2[0] = '2';
	page3[0] = '3';
	
	// Acessa page4 - deve acionar o algoritmo de relógio
	page4[0] = '4';
	
	// Acessa page0 novamente - deve ter sido removida
	printf("Re-reading page0[0] = %c\n", page0[0]);
	
	// Acessa todas as páginas em round-robin para testar movimento do ponteiro do relógio
	for (int i = 0; i < 3; i++) {
		printf("Round %d: ", i);
		printf("%c ", page1[0]);
		printf("%c ", page2[0]);
		printf("%c ", page3[0]);
		printf("%c ", page4[0]);
		printf("%c\n", page0[0]);
	}
	
	exit(EXIT_SUCCESS);
}
