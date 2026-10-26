#pragma once
#include "GuiButton.h"
class GuiButtonMerchant : public GuiButton
{
public:
	GuiButtonMerchant(int_t id, int_t x, int_t y, int_t delta, const std::string &label);
	int_t recipeDelta;
};
