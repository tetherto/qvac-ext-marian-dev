#include "catch.hpp"
#include "common/logging.h"

#include <exception>
#ifdef __unix__
#include <signal.h>
#endif

namespace {

void sentinelTerminate() {}

#ifdef __unix__
void sentinelSignalHandler(int, siginfo_t*, void*) {}

// Installs the sentinel for SIGSEGV and SIGFPE, as a host such as a JVM would,
// and puts the previous handlers back when it goes out of scope.
struct SentinelSignalHandlers {
  struct sigaction previousSegv = {};
  struct sigaction previousFpe = {};

  SentinelSignalHandlers() {
    struct sigaction sentinel = {};
    sigemptyset(&sentinel.sa_mask);
    sentinel.sa_flags = SA_SIGINFO;
    sentinel.sa_sigaction = sentinelSignalHandler;
    sigaction(SIGSEGV, &sentinel, &previousSegv);
    sigaction(SIGFPE, &sentinel, &previousFpe);
  }

  ~SentinelSignalHandlers() {
    sigaction(SIGSEGV, &previousSegv, nullptr);
    sigaction(SIGFPE, &previousFpe, nullptr);
  }
};

bool isSentinel(int signal) {
  struct sigaction current = {};
  sigaction(signal, nullptr, &current);
  return (current.sa_flags & SA_SIGINFO) && current.sa_sigaction == sentinelSignalHandler;
}
#endif

// Restores the terminate handler and the install flag after a test.
struct RestoreProcessState {
  std::terminate_handler terminate = std::get_terminate();
  bool install = marian::getInstallErrorHandlers();

  ~RestoreProcessState() {
    std::set_terminate(terminate);
    marian::setInstallErrorHandlers(install);
  }
};

}  // namespace

TEST_CASE("createLoggers leaves the host's error handlers alone by default", "[logging]") {
  RestoreProcessState restore;
  CHECK_FALSE(marian::getInstallErrorHandlers());

  std::set_terminate(sentinelTerminate);
#ifdef __unix__
  SentinelSignalHandlers sentinels;
#endif

  createLoggers();

  CHECK(std::get_terminate() == sentinelTerminate);
#ifdef __unix__
  CHECK(isSentinel(SIGSEGV));
  CHECK(isSentinel(SIGFPE));
#endif
}

TEST_CASE("createLoggers installs its error handlers when asked to", "[logging]") {
  RestoreProcessState restore;
  marian::setInstallErrorHandlers(true);

  std::set_terminate(sentinelTerminate);
#ifdef __unix__
  SentinelSignalHandlers sentinels;
#endif

  createLoggers();

  CHECK(std::get_terminate() != sentinelTerminate);
#ifdef __unix__
  CHECK_FALSE(isSentinel(SIGSEGV));
  CHECK_FALSE(isSentinel(SIGFPE));
#endif
}
