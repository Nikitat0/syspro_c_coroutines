#include "coopmult.h"

#include <stdio.h>
#include <stdlib.h>

void _sorter(int n) {
  if (n == 0)
    return;
  _sorter(n - 1);
  coopmult_sleep();
}

void sorter(void* arg) {
  int n = *(int*) arg;
  _sorter(n);
  printf("%d\n", n);
}

int main() {
  size_t size = 0;
  size_t cap = 8;
  int* numbers = malloc(cap * sizeof(int));

  int i;
  do {
    scanf("%d", &i);
    numbers[size++] = i;
    if (size == cap)
      numbers = realloc(numbers, (cap *= 2) * sizeof(int));
  } while (i != 0);

  for (size_t i = 0; i < size; i++)
    coopmult_add_task(sorter, &numbers[i]);
  coopmult_run();

  free(numbers);
}
