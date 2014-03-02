#include "../main.h"

#include "gen-cpp/headers.h"

class TestServiceImpl : virtual public MarvinTest::TestServiceIf {
 public:
  int32_t forty_two() {
    return 42;
  }
};

int main(int argc, char** argv) {
  marvin::main<TestServiceImpl, MarvinTest::TestServiceProcessor>();
}
