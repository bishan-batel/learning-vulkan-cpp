#include <spdlog/spdlog.h>
#include "App.hpp"

i32 main() {
  App app;
  app.run();

  try {
    app.run();
  } catch (const std::exception& e) {
    spdlog::error("Runtime Exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return 0;
}
