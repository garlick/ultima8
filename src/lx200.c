/*****************************************************************************\
 *  Copyright (C) 2011 Jim Garlick
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

/* lx200.c - accept lx200 commands for SkySafari and SkyMap iPhone apps */

/* This is just a test program to get the LX200 protocol subset worked out.
 *
 * N.B. SkySafari likes to drop the connection between commands.
 * N.B. SkyMap locks up or crashes on network errors
 *
 * FIXME: properly handle commands that are not aligned on recv () message
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

#define PORT "4030"
#define BACKLOG 5

int send_str (int fd, char *s)
{
    int r;

    fprintf (stderr, "S: %s\n", s);
    r = send (fd, s, strlen(s), 0);
    if (r == -1)
        perror ("S:");
    return r;
}

void talk (int fd)
{
    ssize_t n;
    char buf[256];
    static int flag = 0;
    int a, b, c;
    float A, B, C;

    if (flag) {
        if (send_str (fd, "#") == -1)
            return;
        flag = 0;
    }

    do {
        if ((n = recv (fd, buf, sizeof (buf) - 1, 0)) == -1) {
            perror ("R: ");
            break;
        }
        if (n == 0) {
            //fprintf (stderr, "R: EOF\n");
            break;
        }
        /* special single char cmd */
        if (n == 1 && buf[0] == 0x6) {
            fprintf (stderr, "R: ACK\n",buf);
            if (send_str (fd, "P") == -1)
                break;
            continue;
        }
        buf[n] = '\0';
        fprintf (stderr, "R: %s\n",buf);
        /* set current site latitude (sDD*MM) (resp: 0=invalid, 1=valid) */
        if (sscanf (buf, ":St%d*%d#", &a, &b) == 2) {
            if (send_str (fd, "0") == -1)
                break;
        /* set current site longitude (DDD*MM) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":Sg%d*%d#", &a, &b) == 2) {
            if (send_str (fd, "0") == -1)
                break;
        /* set UTC offset (sHH.H) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":SG%f#", &A) == 1) {
            if (send_str (fd, "0") == -1)
                break;
        /* set local time (HH:MM:SS) (resp: 0=invalid, 1=valid) */
        } else if (sscanf (buf, ":SL%d:%d:%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "0") == -1)
                break;
        /* set handbox date (MM/DD/YY) (resp: 0#=invalid, 1str#=valid) */
        } else if (sscanf (buf, ":SC%d/%d/%d#", &a, &b, &c) == 3) {
            if (send_str (fd, "0#") == -1)
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
        /* halt all current slewing (resp: none) */
        } else if (!strcmp (buf, ":Q#")) {
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
        } else {
            fprintf (stderr, "unknown command\n");
        }
    } while (1);
}

/* Thanks Beej for your handy googleable socket example! */

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
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
        return 2;
    }

    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    while(1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("R: accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);

        //fprintf(stderr, "R: connect %s\n", s);
        talk (new_fd);
        close(new_fd);
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
