#include "utility/utility.hpp"

#ifdef APROJ_DEBUG
void exec_debug(std::function<void()>&& fun) {
    fun();
}
#else
void exec_debug(std::function<void()>&&) {}
#endif