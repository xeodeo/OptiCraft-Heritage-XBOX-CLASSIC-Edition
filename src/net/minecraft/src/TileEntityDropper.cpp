#include "TileEntityDropper.h"

TileEntityDropper::TileEntityDropper()
	: TileEntityDispenser()
{
}

std::string TileEntityDropper::getInvName()
{
	return "container.dropper";
}
