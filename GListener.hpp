#ifndef _GLISTENER_H_
#define _GLISTENER_H_
#include "megaapi.h"

class GListener : public mega::MegaListener {
   public:
	bool finished{};
};

#endif
