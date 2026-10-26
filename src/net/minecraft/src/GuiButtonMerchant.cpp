#include "GuiButtonMerchant.h"
GuiButtonMerchant::GuiButtonMerchant(int_t id, int_t x, int_t y, int_t delta, const std::string &label)
	: GuiButton(id, x, y, 20, 20, label), recipeDelta(delta) {}
