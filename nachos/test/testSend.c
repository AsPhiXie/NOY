#include "userlib/syscall.h"
#include "userlib/libnachos.h"

/*
	MASTER 1 SSR
	KHOUBI SINA
	BOUGAUD YVES

*/


int main() {
	int verif = TtySend("salut");
	n_printf(" On a envoye ce nombre de caractere = %d\n", verif);
	int veriff = TtySend("salut2fois");
	n_printf(" On a envoye ce nombre de caractere = %d\n", veriff);
	int verifff = TtySend("salut3fois");
	n_printf(" On a envoye ce nombre de caractere = %d\n", verifff);
	Halt();
	return 0;
}
