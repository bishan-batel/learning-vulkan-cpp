#include "App.hpp"

i32 main() {
  App app;
  app.run();

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << "Failure: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
