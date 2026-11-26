#include "marian.h"
#include "translator/beam_search.h"
#include "translator/translator.h"
#include "common/timer.h"
#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char** argv) {
  using namespace marian;
  // The command-line tools own their process, so they install Marian's
  // std::terminate and signal handlers (see createLoggers()).
  setInstallErrorHandlers(true);
  auto options = parseOptions(argc, argv, cli::mode::translation);
  auto task = New<Translate<BeamSearch>>(options);

  timer::Timer timer;
  task->run();
  LOG(info, "Total time: {:.5f}s wall", timer.elapsed());

  return 0;
}
