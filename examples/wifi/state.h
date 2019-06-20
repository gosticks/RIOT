#include <stdint.h>
#include <stdbool.h>

#define REQUEST_QUEUE_SIZE 1

typedef struct DriverRequest
{
    uint16_t ID;
    uint16_t Opcode;
    bool Waiting;
    uint8_t *Buffer;
    uint16_t BufferSize;
} DriverRequest;

typedef struct DriverState
{
    uint8_t curReqCount;
    volatile struct DriverRequest *requestQueue[REQUEST_QUEUE_SIZE];
} DriverState;

extern DriverState state;

void addToQueue(volatile DriverRequest *req);
bool removeFromQueue(volatile DriverRequest *req);