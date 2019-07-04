#include <stdint.h>
#include <stdbool.h>

#define REQUEST_QUEUE_SIZE 1

typedef struct ConnectionInfo {
  uint8_t type; // type of current connection
  uint8_t ssidLen;
  uint8_t ssid[32];
  uint8_t bssid[6];
  bool connected; 
} ConnectionInfo;

typedef struct DriverRequest
{
    uint8_t ID;
    uint16_t Opcode;
    
    // response description buffers
    uint8_t *DescBuffer;
    uint16_t DescBufferSize;

    // Payload buffers
    uint8_t *PayloadBuffer;
    uint16_t PayloadBufferSize;

    // current request state
    bool Waiting;

} DriverRequest;

typedef struct DriverState
{
    uint8_t curReqCount;
    volatile struct DriverRequest *requestQueue[REQUEST_QUEUE_SIZE];
    // connection info
    struct ConnectionInfo con;
} DriverState;

extern DriverState state;

void addToQueue(volatile DriverRequest *req);
bool removeFromQueue(volatile DriverRequest *req);