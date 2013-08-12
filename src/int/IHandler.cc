#include "int/IEvent.h"
#include "int/IHandler.h"

#include <cstdint>

void IHandler::setEvent(IEvent* e) {
  event = e;
}
