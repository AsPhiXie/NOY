#include "userlib/syscall.h"
#include "userlib/libnachos.h"

#define SIZE 10

//int buffer[SIZE];

LockId verrou;

void producteur() {
  int i = 0;
  while(i < 10) {
    LockAcquire(verrou);
    /*if(i == 10) {
      i = i%10;
    }
    buffer[i] = 1;*/
    i++;
    LockRelease(verrou);
  }
}

void consommateur() {
  int j = 0;
  while(j < 10) {
    LockAcquire(verrou);
    /*if(j == 10) {
      j = j%10;
    }
    buffer[j] = 0;*/
    j++;
    LockRelease(verrou);
  }
}

int main() {
  ThreadId p, c;

  verrou = LockCreate("Lock");

  c = threadCreate("Consommateur", &consommateur);
  p = threadCreate("Producteur", &producteur);

  return 0;
}
