
#include <stdio.h>
#include <string.h>

#include "vendor/rom.h"

#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/pktdump.h"
#include "msg.h"
#include "shell.h"
#include "periph/gpio.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

int test(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("WOW");
    return 0;
}

static const shell_command_t shell_commands[] = { { "test", "test shell",
                                                    test } };

/* Forward declarations */
int16_t start_pairing(int16_t sd);

extern void send(char *addr_str, char *port_str, char *data, unsigned int num,
                 unsigned int delay);

int main(void)
{
    int16_t status = 0;
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);

    send("2001:0db8:85a3:0000:0000:8a2e:0370:7334", "8080", "TEST TEST TEST TEST ", 100, 10000);

    /* start shell */
    // printf("All up, running the shell now\n");
    // char line_buf[SHELL_DEFAULT_BUFSIZE];
    // shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
