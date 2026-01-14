#include "cli/cli.h"
#include <iostream>

int main(int argc, char** argv) {
  try {
    return cli::run(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return 1;
  }
}

