#include "state.h"
#include "mutex.h"
#include <stddef.h>

// #define REQUEST_MUX MUTEX_INIT

DriverState state = { .requestQueue = { NULL },
                      .curReqCount  = 0,
                      .con          = { .connected = 0 } };

void addToQueue(volatile DriverRequest *req)
{
    // wait till the queue is free
    while (state.curReqCount >= REQUEST_QUEUE_SIZE) {
    }
    state.requestQueue[state.curReqCount] = req;
    state.curReqCount++;
}

bool removeFromQueue(volatile DriverRequest *req)
{
    for (uint8_t i = 0; i < REQUEST_QUEUE_SIZE; i++) {
        if (state.requestQueue[i] == req) {
            state.requestQueue[i] = NULL;
            state.curReqCount--;
            return true;
        }
    }
    return false;
}
