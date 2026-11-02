#pragma once
#include <atomic>
#include <csignal>
#include <clingo.h>

// Stopping a run on SIGINT / SIGTERM safely: the signal handler only sets stopRequested and interrupts the solve call
// in progress (both are async-signal-safe). ClingoSolver::solve then returns an interrupted result without calling
// clingo, the searches end as on any interrupted call, and the program prints its best answer set, writes its
// statistics and exits in normal code.
// Grounding cannot be interrupted: a signal during grounding takes effect at the first solve call.
namespace fiore::clingo {

    // set by the signal handler
    inline volatile std::sig_atomic_t stopRequested = 0;

    // the control of the program being solved: set by Program::ProgramGrounder::ground, reset by ~Program before
    // clingo_control_free. A lock-free atomic, so the handler may read it.
    inline std::atomic<clingo_control_t*> controlToInterrupt{nullptr};
    static_assert(std::atomic<clingo_control_t*>::is_always_lock_free);

} // namespace fiore::clingo
