#ifndef IR_INTERNAL_H
#define IR_INTERNAL_H

#include "subsystem3.h"

extern ID event_id, mutex_id, cyclic_id;
void ir_tick(void *exinf);
void ir_worker(INT stacd, void *exinf);

#endif
