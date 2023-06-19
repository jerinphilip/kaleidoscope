#include <iostream>

extern "C" {
double kl_math_sqrt(double);
double kl_math_log(double);
double kl_std_max(double, double);
double kl_std_min(double, double);
double kl_putchard(double);
}

#define TRACE(x)                                \
  do {                                          \
    std::cout << #x << " " << (x) << std::endl; \
  } while (0)

int main() {
  TRACE(kl_math_sqrt(9.0));
  TRACE(kl_math_log(10.0));
  TRACE(kl_std_max(3.0, 4.0));
  TRACE(kl_std_min(3.0, 4.0));

  kl_putchard(3.0);
  return 0;
}
