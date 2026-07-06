/*
 * arguments.cpp - implementation of the rsUtility command-line argument
 * helpers declared in arguments.h. The original implementation was not
 * present in the rslibs repository, so this is a clean re-implementation
 * for rss-port, matching the documented semantics:
 *
 *   findArgument:      return index of the named argument, or -1
 *   getArgumentsValue: fetch the value following the named argument,
 *                      optionally clamped to [minimum, maximum];
 *                      return the value's index, or -1 if absent/unparsable
 *
 * LGPL 2.1, same as the rest of rslibs.
 */

#include "arguments.h"

#include <cstdlib>

int findArgument(int argc, char* argv[], std::string arg)
{
	for (int i = 1; i < argc; i++)
		if (arg == argv[i])
			return i;
	return -1;
}

int getArgumentsValue(int argc, char* argv[], std::string arg, std::string& value)
{
	const int i = findArgument(argc, argv, arg);
	if (i < 0 || i + 1 >= argc)
		return -1;
	value = argv[i + 1];
	return i + 1;
}

template <class T>
static int getValueT(int argc, char* argv[], const std::string& arg, T& value)
{
	std::string s;
	const int idx = getArgumentsValue(argc, argv, arg, s);
	if (idx < 0)
		return -1;
	T parsed;
	if (!from_string(parsed, s, std::dec))
		return -1;
	value = parsed;
	return idx;
}

template <class T>
static int getValueClampedT(int argc, char* argv[], const std::string& arg,
                            T& value, T minimum, T maximum)
{
	const int idx = getValueT(argc, argv, arg, value);
	if (idx < 0)
		return -1;
	if (value < minimum) value = minimum;
	if (value > maximum) value = maximum;
	return idx;
}

int getArgumentsValue(int argc, char* argv[], std::string arg, int& value)
{ return getValueT(argc, argv, arg, value); }

int getArgumentsValue(int argc, char* argv[], std::string arg, int& value, int minimum, int maximum)
{ return getValueClampedT(argc, argv, arg, value, minimum, maximum); }

int getArgumentsValue(int argc, char* argv[], std::string arg, float& value)
{ return getValueT(argc, argv, arg, value); }

int getArgumentsValue(int argc, char* argv[], std::string arg, float& value, float minimum, float maximum)
{ return getValueClampedT(argc, argv, arg, value, minimum, maximum); }

int getArgumentsValue(int argc, char* argv[], std::string arg, double& value)
{ return getValueT(argc, argv, arg, value); }

int getArgumentsValue(int argc, char* argv[], std::string arg, double& value, double minimum, double maximum)
{ return getValueClampedT(argc, argv, arg, value, minimum, maximum); }
