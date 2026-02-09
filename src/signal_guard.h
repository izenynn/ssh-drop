#ifndef SSH_DROP_SIGNAL_GUARD_H_
#define SSH_DROP_SIGNAL_GUARD_H_

#include <atomic>

namespace drop {

class SignalGuard {
public:
	explicit SignalGuard(std::atomic<bool>& running);
	~SignalGuard();

	SignalGuard(const SignalGuard&)		   = delete;
	SignalGuard& operator=(const SignalGuard&) = delete;
	SignalGuard(SignalGuard&&)		   = delete;
	SignalGuard& operator=(SignalGuard&&)	   = delete;
};

} // namespace drop

#endif // SSH_DROP_SIGNAL_GUARD_H_
