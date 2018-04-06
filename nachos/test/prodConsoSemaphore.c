#include "userlib/syscall.h"
#include "userlib/libnachos.h"

#define SIZE 3

int buffer[10];
//int compteur = 500;

SemId vide;
SemId plein;

void producteur() {
  int i = 0;
  while(i < 10) {
    P(vide);
    /*if(i == 3) {
      i = i%3;
    }*/
    buffer[i] = 1;
    i++;
    V(plein);
  }
}

void consommateur() {
  int j = 0;
  while(j < 10) {
    P(plein);
   /* if(j == 3) {
      j = j%3;
    }*/
    buffer[j] = 0;
    j++;
    V(vide);
  }
}

int main() {
  ThreadId p, c;

  vide = SemCreate("vide", SIZE);
  plein  = SemCreate("plein", 0);

  c = threadCreate("Consommateur", &consommateur);
  p = threadCreate("Producteur", &producteur);

  /*SemDestroy(vide);
  SemDestroy(plein);*/
  return 0;
}
