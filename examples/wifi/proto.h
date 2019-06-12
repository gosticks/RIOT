#include <stdint.h>

typedef uint16_t  CommandCode;

typedef struct
{
	uint16_t  Status;
	uint16_t  DeviceSetId;
	uint16_t  Option;
	uint16_t  ConfigLen;
}_DeviceSetGet_t;

typedef struct
{
    CommandCode      cmd;
    uint8_t     TxDescLen;
    uint8_t     RxDescLen;
}ControlCommand;

typedef struct {
    uint16_t TxPayloadLen;
    uint16_t RxPayloadLen;
    uint16_t ActualRxLen;
    uint8_t *TxPayload; 
    uint8_t *RxPayload; 
} CommandPayload;