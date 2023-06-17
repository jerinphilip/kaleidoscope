#include <iostream>

extern "C" {
double kl_id(double);
bool kl_true(void);
}

#define TRACE(x)                                \
  do {                                          \
    std::cout << #x << " " << (x) << std::endl; \
  } while (0)

int main() {
  TRACE(kl_id(3.0));
  TRACE(kl_true());
  return 0;
}
