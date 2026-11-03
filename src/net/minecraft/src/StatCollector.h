#pragma once

#include <string>

// net.minecraft.src.StatCollector
class StatCollector
{
public:
	static std::string translateToLocal(const std::string &s);
	static std::string translateToLocalFormatted(const std::string &s, const std::string &arg);
	static std::string translateToLocalFormatted(const std::string &s, const std::string &arg1, const std::string &arg2);
	static bool func_94522_b(const std::string &s);
	static bool canTranslate(const std::string &s);
};
