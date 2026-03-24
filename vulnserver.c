/*
 * vulnserver-linux.c — deliberately vulnerable TCP server for exploitation exercises
 *
 * A Linux-native port of vulnserver by Stephen Bradshaw (thegreycorner.com).
 * Original: https://github.com/stephenbradshaw/vulnserver
 *
 * THIS SOFTWARE IS INTENTIONALLY VULNERABLE.
 * Run only in isolated lab environments. Never expose to untrusted networks.
 *
 * Compile:
 *   gcc vulnserver-linux.c -o vulnserver \
 *       -m32 -fno-stack-protector -z execstack -no-pie -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define VERSION      "1.00"
#define DEFAULT_PORT 9999
#define BUFLEN       4096

/* ── Vulnerable helper functions (mirror of original vulnserver) ── */

void Function1(char *Input) {
    char Buffer2S[140];
    strcpy(Buffer2S, Input);
}

void Function2(char *Input) {
    char Buffer2S[60];
    strcpy(Buffer2S, Input);
}

void Function3(char *Input) {
    char Buffer2S[2000];
    strcpy(Buffer2S, Input);   /* ← intentional overflow */
}

void Function4(char *Input) {
    char Buffer2S[1000];
    strcpy(Buffer2S, Input);
}

/* ── Deliberate JMP ESP gadget for exploitation exercises ── */

void __attribute__((used, noinline)) jmp_esp_gadget(void) {
    __asm__ volatile ("jmp *%esp");
}

/* ── Connection handler (runs in its own thread) ── */

void *ConnectionHandler(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char RecvBuf[BUFLEN];
    int  i;

    send(client, "Welcome to Vulnerable Server! Enter HELP for help.\n", 51, 0);

    while (1) {
        memset(RecvBuf, 0, BUFLEN);
        int n = recv(client, RecvBuf, BUFLEN - 1, 0);
        if (n <= 0) break;

        if (strncmp(RecvBuf, "HELP ", 5) == 0) {
            send(client, "Command specific help has not been implemented\n", 47, 0);

        } else if (strncmp(RecvBuf, "HELP", 4) == 0) {
            const char *cmds =
                "Valid Commands:\nHELP\nSTATS [stat_value]\nRTIME [rtime_value]\n"
                "LTIME [ltime_value]\nSRUN [srun_value]\nTRUN [trun_value]\n"
                "GMON [gmon_value]\nGDOG [gdog_value]\nKSTET [kstet_value]\n"
                "GTER [gter_value]\nHTER [hter_value]\nLTER [lter_value]\n"
                "KSTAN [lstan_value]\nEXIT\n";
            send(client, cmds, strlen(cmds), 0);

        } else if (strncmp(RecvBuf, "STATS ", 6) == 0) {
            send(client, "STATS VALUE NORMAL\n", 19, 0);

        } else if (strncmp(RecvBuf, "RTIME ", 6) == 0) {
            send(client, "RTIME VALUE WITHIN LIMITS\n", 26, 0);

        } else if (strncmp(RecvBuf, "LTIME ", 6) == 0) {
            send(client, "LTIME VALUE HIGH, BUT OK\n", 25, 0);

        } else if (strncmp(RecvBuf, "SRUN ", 5) == 0) {
            send(client, "SRUN COMPLETE\n", 14, 0);

        } else if (strncmp(RecvBuf, "TRUN ", 5) == 0) {
            char *TrunBuf = malloc(3000);
            memset(TrunBuf, 0, 3000);
            for (i = 5; i < BUFLEN; i++) {
                if (RecvBuf[i] == '.') {
                    strncpy(TrunBuf, RecvBuf, 3000);
                    Function3(TrunBuf);   /* ← vulnerable call */
                    break;
                }
            }
            free(TrunBuf);
            send(client, "TRUN COMPLETE\n", 14, 0);

        } else if (strncmp(RecvBuf, "GMON ", 5) == 0) {
            for (i = 5; i < BUFLEN; i++) {
                if (RecvBuf[i] == '/') {
                    if (strlen(RecvBuf) > 3950) Function3(RecvBuf);
                    break;
                }
            }
            send(client, "GMON STARTED\n", 13, 0);

        } else if (strncmp(RecvBuf, "GDOG ", 5) == 0) {
            send(client, "GDOG RUNNING\n", 13, 0);

        } else if (strncmp(RecvBuf, "KSTET ", 6) == 0) {
            char *KstetBuf = malloc(100);
            strncpy(KstetBuf, RecvBuf, 100);
            Function2(KstetBuf);
            free(KstetBuf);
            send(client, "KSTET SUCCESSFUL\n", 17, 0);

        } else if (strncmp(RecvBuf, "GTER ", 5) == 0) {
            char *GterBuf = malloc(180);
            strncpy(GterBuf, RecvBuf, 180);
            Function1(GterBuf);
            free(GterBuf);
            send(client, "GTER ON TRACK\n", 14, 0);

        } else if (strncmp(RecvBuf, "LTER ", 5) == 0) {
            char LterBuf[BUFLEN];
            memset(LterBuf, 0, BUFLEN);
            for (i = 0; RecvBuf[i]; i++)
                LterBuf[i] = ((unsigned char)RecvBuf[i] > 0x7f)
                             ? RecvBuf[i] - 0x7f : RecvBuf[i];
            for (i = 5; i < BUFLEN; i++) {
                if (LterBuf[i] == '.') { Function3(LterBuf); break; }
            }
            send(client, "LTER COMPLETE\n", 14, 0);

        } else if (strncmp(RecvBuf, "KSTAN ", 6) == 0) {
            send(client, "KSTAN UNDERWAY\n", 15, 0);

        } else if (strncmp(RecvBuf, "EXIT", 4) == 0) {
            send(client, "GOODBYE\n", 8, 0);
            break;

        } else {
            send(client, "UNKNOWN COMMAND\n", 16, 0);
        }
    }

    close(client);
    return NULL;
}

/* ── Main ── */

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc == 2) port = atoi(argv[1]);

    signal(SIGPIPE, SIG_IGN);  /* prevent crash when client closes early */

    printf("Starting vulnserver version %s\n\n", VERSION);
    printf("This is vulnerable software!\n"
           "Do not allow access from untrusted systems or networks!\n\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port)
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    printf("Waiting for client connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        printf("Received a client connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_t thread;
        pthread_create(&thread, NULL, ConnectionHandler, client_fd);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}
