#include "userlib/syscall.h"
#include "userlib/libnachos.h"

/*
	MASTER 1 SSR
	KHOUBI SINA
	BOUGAUD YVES

*/


int main() {
	char msg[20];
	char msg2[20];
	int verif = TtyReceive(msg, 20);
	n_printf("On a reçu : %s avec nb caractère reçu = %d\n", msg, verif);
	int veriff = TtyReceive(msg2, 20);
	n_printf("On a reçu : %s avec nb caractère reçu = %d\n", msg2, veriff);
	int verifff = TtyReceive(msg, 10);
	n_printf("On a reçu : %s avec nb caractère reçu = %d\n", msg, verifff);
	Halt();
	return 0;
}
