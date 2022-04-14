#include <cstring>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <librpitx/librpitx.h>

bool running = true;

static void terminate(int num)
{
    running = false;
    fprintf(stderr, "Caught signal - Terminating\n");
}

void print_usage()
{
    fprintf(stderr,"sendfsk : a program to send FSK (Frequency-Shift Keying) with a Raspberry PI.\n\
usage : sendfsk [options] \"binary code\"\n\
Options:\n\
-h : this help\n\
-v : verbose (-vv : more verbose)\n\
-f freq : frequency in Hz (default : 433920000)\n\
-d nb : deviation (default : 15000)\n\
-c nb : channel (default : 14)\n\
-r nb : symbol rate (default : 1000)\n\
-t nb : repeat nb times the message (default : 3)\n\
-p nb : pause between each message (default : 1000us=1ms)\n\
\n\
\"binary code\":\n\
  a serie of 0 or 1 char (space allowed and ignored)\n\
\n\
Examples:\n\
  sendfsk -f 868300000 -t 3 -p 5000 1010101001010101\n\
    send 0xaa55 three times on 868.3MHz. Pause 5ms between transmissions.\n\
");
}

int main(int argc, char *argv[])
{
    uint64_t frequency = 433920000;
    int deviation = 15000;
    int channel = 14;
    int symbolRate = 1000;
    uint32_t fifoSize = 0;
    int times = 3;
    int pause = 1000;

    char *bits = NULL;
    unsigned char symbols[4096];

    for (int i = 0; i < 64; i++)
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

    int c;
    while ((c = getopt(argc, argv, "hvf:d:c:r:t:p:")) != -1)
    {
        switch (c)
        {
            case 'h':
                print_usage();
                exit(1);
                break;
            case 'v':
                dbg_setlevel(dbg_getlevel() + 1);
                break;
            case 'f':
                frequency = atoi(optarg);
                break;
            case 'd':
                deviation = atoi(optarg);
                break;
            case 'c':
                channel = atoi(optarg);
                break;
            case 'r':
                symbolRate = atoi(optarg);
                break;
            case 't':
                times = atoi(optarg);
                break;
            case 'p':
                pause = atoi(optarg);
                break;
            default:
                print_usage();
                exit(1);
                break;
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Missing bit message.\n");
        return 1;
    }

    bits = argv[optind];
    for (size_t i = 0; i < strlen(bits); i++)
    {
        char b = bits[i];
        if (b == '0')
        {
            symbols[i] = 0;
            fifoSize++;
        }
        else if (b == '1')
        {
            symbols[i] = 1;
            fifoSize++;
        }
    }

    dbg_printf(1, "frequency:  %lu\n", frequency);
    dbg_printf(1, "deviation:  %d\n", deviation);
    dbg_printf(1, "symbolRate: %d\n", symbolRate);
    dbg_printf(1, "channel:    %d\n", channel);
    dbg_printf(1, "times:      %d\n", times);
    dbg_printf(1, "pause:      %d\n", pause);

    fskburst burst(frequency, symbolRate, deviation, channel, fifoSize);

    for (int i = 0; i < times; i++)
    {
        dbg_printf(2, "Transmission %d/%d\n", i+1, times);
        burst.SetSymbols(symbols, fifoSize);

        if (i < times - 1)
        {
            usleep(pause);
        }

        if (!running)
        {
            break;
        }
    }

    burst.stop();

    return 0;
}
