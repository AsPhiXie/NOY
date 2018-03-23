/* hello.c
 *	Simple hello world program
 *
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.
//  See copyright_insa.h for copyright notice and limitation
//  of liability and disclaimer of warranty provisions.
 */

#include "userlib/syscall.h"
#include "userlib/libnachos.h"

SemId id1;
SemId id2;

void fonction1() {
	n_printf("** P bloquant fonction1 **\n");
	P(id2);
	//n_printf("resSema = %d", resSema1);
	n_printf("** En attente du sémaphore **\n");
	V(id1);
	//n_printf("resSema = %d", resSema2);
	int tmp = SemDestroy(id1);
	int tmp2 = SemDestroy(id2);
	n_printf("** fini **%d et %d\n", tmp, tmp2);
}

void fonction2() {
	n_printf("** Avant de libéré **\n");
	V(id2);
	//n_printf("resSema = %d", resSema3);
	n_printf("** V() libéré **\n");
	n_printf("** P bloquant fonction2 **\n");
	P(id1);
	//n_printf("resSema = %d", resSema4);
	int tmp = SemDestroy(id1);
	int tmp2 = SemDestroy(id2);
	n_printf("** fini **%d et %d\n", tmp, tmp2);
}

int main() {
	id1 = SemCreate("sema1", 0);
	id2 = SemCreate("sema2", 0);
  n_printf("** ** ** Nachos ** ** **\n");
	ThreadId thread1 = threadCreate("thread1", &fonction1);
	ThreadId thread2 = threadCreate("thread2", &fonction2);

  return 0;
}
