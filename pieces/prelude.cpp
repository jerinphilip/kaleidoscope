#include <stdio.h>

void constants() {
  bool kl_true = true;
  bool kl_false = false;

  printf("true %d false %d\n", kl_true, kl_false);

  int kl_int = -42;
  unsigned int kl_uint = 42u;

  printf("kl_int %d kl_uint %d\n", kl_int, kl_uint);

  float kl_float = 1.0F;
  float kl_double = 1.00;
  printf("kl_float %f kl_double %lf", kl_float, kl_double);
}

int kl_max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
}
