#include "TileEntityDropper.h"

TileEntityDropper::TileEntityDropper()
	: TileEntityDispenser()
{
}

std::string TileEntityDropper::getInvName()
{
	return isInvNameLocalized() ? customName : "container.dropper";
}
