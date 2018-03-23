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

void fonction1() {
	V(id1);
	n_printf("** ** ** V ** ** **\n");
	P(id1);
	n_printf("** ** ** P ** ** **\n");
}

int main() {
	id1 = SemCreate("sema1", 0);
	ThreadId thread1 = threadCreate("thread1", &fonction1);
  return 0;
}
