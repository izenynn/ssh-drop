#include "signal_guard.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#endif

namespace drop {

namespace {

std::atomic<bool>* g_running = nullptr;

#ifdef _WIN32

BOOL WINAPI console_handler(DWORD event)
{
	if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT ||
	    event == CTRL_CLOSE_EVENT) {
		if (g_running)
			g_running->store(false, std::memory_order_relaxed);
		return TRUE;
	}
	return FALSE;
}

#else

void signal_handler(int /*sig*/)
{
	if (g_running)
		g_running->store(false, std::memory_order_relaxed);
}

#endif

} // namespace

SignalGuard::SignalGuard(std::atomic<bool>& running)
{
	g_running = &running;

#ifdef _WIN32
	SetConsoleCtrlHandler(console_handler, TRUE);
#else
	struct sigaction sa{};
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
#endif
}

SignalGuard::~SignalGuard()
{
#ifdef _WIN32
	SetConsoleCtrlHandler(console_handler, FALSE);
#else
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
#endif
	g_running = nullptr;
}

} // namespace drop
