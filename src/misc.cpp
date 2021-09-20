// TODO: get rid of this file

#include "misc.hpp"
#include <stdexcept>
void fatal_error(const char *msg)
{
	throw std::runtime_error(msg);
}
