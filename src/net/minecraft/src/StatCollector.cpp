#include "StatCollector.h"

#include "StringTranslate.h"

// StringTranslate::getInstance() must not run until StatCollector is first
// actually used. It used to be cached in a StatCollector::localizedName static
// initialized at namespace scope, which -- being a plain, non-local static --
// is constructed during C++ static initialization, before main() runs. On PS2
// that is before Ps2Bootstrap::initialize() brings up IOP file services and the
// asset locator, so the resulting StringTranslate ctor's setLanguage("en_US")
// silently failed to open the language file and left the translation table
// empty.
//
// AchievementList::initialize() runs early in Minecraft's ctor but safely after
// that bootstrap, and bakes each achievement's translated name/description into
// the Achievement object once at construction through this same lookup -- so it
// kept the empty-table result even after GameSettings reloaded the table
// correctly later. That is why achievement and tutorial text specifically
// showed raw keys while ordinary menus, which call translateKey() fresh on
// every render, did not.
//
// getInstance() is itself a lazy singleton, so calling it directly defers first
// construction to first real use instead of forcing it before main().
std::string StatCollector::translateToLocal(const std::string &s)
{
	return StringTranslate::getInstance()->translateKey(s);
}

std::string StatCollector::translateToLocalFormatted(const std::string &s, const std::string &arg)
{
	return StringTranslate::getInstance()->translateKeyFormat(s, arg);
}

std::string StatCollector::translateToLocalFormatted(const std::string &s, const std::string &arg1, const std::string &arg2)
{
	return StringTranslate::getInstance()->translateKeyFormat(s, arg1, arg2);
}

bool StatCollector::func_94522_b(const std::string &s)
{
	return StringTranslate::getInstance()->containsTranslateKey(s);
}

bool StatCollector::canTranslate(const std::string &s)
{
	return StringTranslate::getInstance()->containsTranslateKey(s);
}
