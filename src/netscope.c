/*****************************************************************************\
 *  Copyright (C) 2012 Jim Garlick
 *  
 *  This file is part of ultima8drivecorrector, replacement base electronics
 *  for the Celestron Ultima 8 telescope.  For details, see
 *  <http://code.google.com/p/ultima8drivecorrector>.
 *  
 *  ultima8drivecorrector is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  ultima8drivecorrector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with ultima8drivecorrector; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* netscope.c - accept commands for Sky Safari and SkyMap iPhone apps */

/* cc -o netscope netscope.c
 * ./netscope -d to test without PIC 
 */

/* FIXME: properly handle commands that are not aligned on recv () message
 * boundaries.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>

typedef enum { MODE_ENC, MODE_LX200 } emumode_t;

#define PORT "4030"
#define BACKLOG 5

static int enc_ra_res = 2160;
static int enc_dec_res = 2160;

static int debug = 0;

int send_str (int fd, char *s)
{
    int r;

    if (debug)
        fprintf (stderr, "S: %s\n", s);
    r = send (fd, s, strlen(s), 0);
    if (r == -1 && debug)
        perror ("S:");
    return r;
}

void query_encoders (int *ra, int *dec)
{
    /* FIXME: send    "::Q\n" */
    /* FIXME: expect  "%d\t%d\r" */
    *ra = 0;
    *dec = 0;
}

/* Sky Safari "Basic Encoder" or "NGC Max".
 * AKA the Tangent/BBox protocol.
 */
void enc_srv (int fd)
{
    ssize_t n;
    char buf[256];
    int ra, dec;

    for (;;) {
        if ((n = recv (fd, buf, sizeof (buf) - 1, 0)) == -1) {
            if (debug)
                perror ("R: ");
            break;
        }
        if (n == 0) {
            if (debug)
                fprintf (stderr, "R: EOF\n");
            break;
        }
        buf[n] = '\0';
        if (debug)
            fprintf (stderr, "R: %s\n",buf);
        /* Sky Safari: "QQQQQQQQQQQQ" sent on first connect, "Q" after */
        if ((buf[0] == 'Q')) {       /* get encoder position */
            query_encoders (&ra, &dec);
            snprintf (buf, sizeof(buf), "%+.5d\t%+.5d\r", ra, dec);
            send_str (fd, buf);
        } else if (buf[0] == 'H') {  /* get encoder resolution */
            snprintf (buf, sizeof(buf), "%+.5d\t%+.5d\r",
                enc_ra_res, enc_dec_res);
            send_str (fd, buf);
        /* Sky Safari: not used far as I can tell */
        } else if (buf[0] == 'Z') {  /* set encoder resolution */
            if (sscanf (buf + 1, "%d%d", &enc_ra_res, &enc_dec_res) != 2) {
                if (debug)
                    perror ("sscanf error");
            } else 
                send_str (fd, "*\r");
        }
    }
}

/* Ref: "Meade Telescope Serial Command Protocol, Revision L", 9 October 2002.
 * Emulate a subset of the LX200<16" ("classic") protocol.
 * TODO: only reached the point where the protocol subset used by the
 * two iphone apps is figured out.  No actual functionality yet!
 */
void lx200_srv (int fd)
{
    ssize_t n;
    char buf[256];
    static int flag = 0;
    int a, b, c;
    float A, B;

    if (flag) {
        if (send_str (fd, "#") == -1)
            return;
        flag = 0;
    }

    do {
        if ((n = recv (fd, buf, sizeof (buf) - 1, 0)) == -1) {
            if (debug)
                perror ("R: ");
            break;
        }
        if (n == 0) {
            if (debug)
                fprintf (stderr, "R: EOF\n");
            break;
        }
        /* special single char cmd */
        if (n == 1 && buf[0] == 0x6) {
            if (debug)
                fprintf (stderr, "R: ACK\n");
            if (send_str (fd, "P") == -1)
                break;
            continue;
        }
        buf[n] = '\0';
        if (debug)
            fprintf (stderr, "R: %s\n",buf);
        /* set current site latitude (sDD*MM) (resp: 0=invalid, 1=valid) */
        if (sscanf (buf, ":St%d*%d#", &a, &b) == 2) {
            if (send_str (fd, "1") == -1)
                break;
        /* set current site longitude (DDD*MM) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sg%d*%d#", &a, &b) == 2) {
            if (send_str (fd, "1") == -1)
                break;
        /* set UTC offset (sHH.H) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":SG%f#", &A) == 1) {
            if (send_str (fd, "1") == -1)
                break;
        /* set local time (HH:MM:SS) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":SL%d:%d:%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "1") == -1)
                break;
        /* set handbox date (MM/DD/YY) (resp: 0#=invalid, 1str#=valid) */
        } else if (sscanf (buf, ":SC%d/%d/%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "1#") == -1)
                break;
            flag = 1; /* SkySafari expects unsolicited str after reconnect */
        /* get telescope RA (resp: HH:MM.T or HH:MM:SS) */
        } else if (!strcmp (buf, ":GR#")) {
            if (send_str (fd, "00:00:00#") == -11)
                break;
        /* set fast slew (resp: none) */
        } else if (!strcmp (buf, ":RS#")) {
        /* set slew rate to find rate (2nd fastest) (resp: none) */
        } else if (!strcmp (buf, ":RM#")) {
        /* set slew rate to centering rate (2nd slowest) (resp: none) */
        } else if (!strcmp (buf, ":RC#")) {
        /* set slew rate to guiding rate (slowest) (resp: none) */
        } else if (!strcmp (buf, ":RG#")) {
        /* get telescope product name (resp: str#) */
        } else if (!strcmp (buf, ":GVP#")) {
            if (send_str (fd, "ultima8drivecorrector#") == -1)
                break;
        /* get telescope DEC (resp: sDD*MM or sDD*MM'SS) */
        } else if (!strcmp (buf, ":GD#")) {
            if (send_str (fd, "+01*01'01#") == -1)
                break;
        /* FIXME: combined halt, get DEC (skysafari) */
        } else if (!strcmp (buf, ":Q#:GD#")) {
            if (send_str (fd, "+01*01'01#") == -1)
                break;
        /* set target object RA (HH:MM.T) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sr%d:%f#", &a, &B) == 2) {
            if (send_str (fd, "1") == -1)
                break;
        /* set target object RA (HH:MM:SS) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sr%d:%d:%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "1") == -1)
                break;
        /* set target object DEC (sDD*MM) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sd%d*%d#", &a, &b) == 2) {
            if (send_str (fd, "1") == -1)
                break;
        /* set target object DEC (sDD*MM:SS) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sd%d*%d:%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "1") == -1)
                break;
        /* slew to target object (resp: 0=valid, 1str#=below horiz,
           2str#=below higher(?)) */
        } else if (!strcmp (buf, ":MS#")) {
            if (send_str (fd, "0") == -1)
                break;
            /* SkySafari will issue :GD# and :GR# until target is reached,
               or :Q# if "stop" is pressed. */
        /* sync telescope's position with currently selected db object
           coordinates (resp: str#) */
        } else if (!strcmp (buf, ":CM#")) {
            if (send_str (fd, "happy fun object#") == -1)
                break;
        /* move east (:Q# to stop) at current slew rate (resp: none) */
        } else if (!strcmp (buf, ":Me#")) {
        /* move west at current slew rate (resp: none) */
        } else if (!strcmp (buf, ":Mw#")) {
        /* move north at current slew rate (resp: none) */
        } else if (!strcmp (buf, ":Mn#")) {
        /* move south at current slew rate (resp: none) */
        } else if (!strcmp (buf, ":Ms#")) {
        /* halt all current slewing (resp: none) */
        } else if (!strcmp (buf, ":Q#")) {
        /* halt east slew (resp: none) */
        } else if (!strcmp (buf, ":Qe#")) {
        /* halt west slew (resp: none) */
        } else if (!strcmp (buf, ":Qw#")) {
        /* halt north slew (resp: none) */
        } else if (!strcmp (buf, ":Qn#")) {
        /* halt south slew (resp: none) */
        } else if (!strcmp (buf, ":Qs#")) {
        } else {
            if (debug)
                fprintf (stderr, "unknown command\n");
        }
    } while (1);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int
setup_service (void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit (1);
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit (1);
    }
    freeaddrinfo(servinfo);
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

int
accept_connection (int sockfd)
{
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    int new_fd = -1;
    char s[INET6_ADDRSTRLEN];

    while (new_fd == -1) { 
        sin_size = sizeof (their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
            perror("R: accept");
    }
    if (debug) {
        inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        fprintf(stderr, "R: connect %s\n", s);
    }

    return new_fd;
}

void usage (void)
{
    fprintf (stderr,
"Usage: netscope [-m lx200|enc] [-d] [-s serial_dev]\n"
    );
    exit (1);
}

int main(int argc, char *argv[])
{
    int c;
    emumode_t mode = MODE_ENC; 
    int svc_fd, new_fd;

    while ((c = getopt (argc, argv, "dm:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'm':
                if (strcmp (optarg, "enc") == 0)
                    mode = MODE_ENC;
                else if (strcmp (optarg, "lx200") == 0)
                    mode = MODE_LX200;
                else
                    usage ();
                break;
            default:
                usage ();
        }
    } 

    svc_fd = setup_service ();
    for (;;) {
        /* Sky Safari: reconnects for each command. */
        new_fd = accept_connection (svc_fd);
        switch (mode) {
            case MODE_ENC:
                enc_srv (new_fd);
                break;
            case MODE_LX200:
                lx200_srv (new_fd);
                break;
        }
        close(new_fd);
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
