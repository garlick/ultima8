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

/* lx200.c - accept lx200 commands on port 4030 for SkySafari iPhone app */

/* This is just a prototype to see if we can interface with SkySafari
   on the iPhone.  We get lots of command sent but eventually it gets
   unhappy with us.

   Starmap Pro is happy but uses way fewer commands.  We want both to
   work with our push-to.
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
#include <sys/wait.h>
#include <signal.h>

#define PORT "4030"
#define BACKLOG 10

int send_str (int fd, char *s)
{
    int r = send (fd, s, strlen(s), 0);

    if (r == -1)
        perror ("send");

    return r;
}

void talk (int fd)
{
    ssize_t n;
    char buf[256];
    int lat_deg, lat_min;
    int long_deg, long_min;
    int utc_offset, l_hh, l_mm, l_ss;
    int d_mm, d_dd, d_yy;

    do {
        if ((n = recv (fd, buf, sizeof (buf) - 1, 0)) == -1) {
            perror ("recv");
            break;
        }
        if (n == 0) {
            printf ("received EOF, disconnecting\n");
            break;
        } else if (n == 1 && buf[0] == 0x6) { /* special single char cmd */
            printf ("received ACK cmd\n",buf);
            if (send_str (fd, "P") == -1)
                break;
            continue;
        }
        buf[n] = '\0';
        printf ("'%s'\n",buf);
        fflush (stdout);

        if (sscanf (buf, ":St%d*%d#", &lat_deg, &lat_min) == 2) {
            printf ("Latitude: %d;%d'\n", lat_deg, lat_min);
            if (send_str (fd, "0") == -1)
                break;
        } else if (sscanf (buf, ":Sg%d*%d#", &long_deg, &long_min) == 2) {
            printf ("Longitude: %d;%d'\n", long_deg, long_min);
            if (send_str (fd, "0") == -1)
                break;
        } else if (sscanf (buf, ":SG%d#", &utc_offset) == 1) {
            printf ("UTC offset: %d\n", utc_offset);
            if (send_str (fd, "0") == -1)
                break;
        /* ':SL15:25:08#' */
        } else if (sscanf (buf, ":SL%d:%d:%d#", &l_hh, &l_mm, &l_ss) == 3) {
            printf ("Local time: %2d:%2d:%2d\n", l_hh, l_mm, l_ss);
            if (send_str (fd, "0") == -1)
                break;
        /* ':SC12/31/11#' */
        } else if (sscanf (buf, ":SC%d/%d/%d#", &d_mm, &d_dd, &d_yy) == 3) {
            printf ("Handbox date: %2d/%2d/%2d\n", d_mm, d_dd, d_yy);
            if (send_str (fd, "0Updating Planetary Data#") == -1)
                break;
        } else if (!strcmp (buf, ":GR#")) { /* read RA */
            snprintf (buf, sizeof (buf), "00:00:00#");
            if (send_str (fd, buf) == 1)
                break;
        } else if (!strcmp (buf, ":RS#")) { /* set fast slew (no resp) */
        } else if (!strcmp (buf, ":GVP#")) { /* get tel model */
            snprintf (buf, sizeof (buf), "LX200-not-really#");
            if (send_str (fd, buf) == 1)
                break;
        } else if (!strcmp (buf, ":Q#:GD#'")) { /* halt cur slew (no resp) */
                                                /* get dec */
            snprintf (buf, sizeof (buf), "00*00'00#");
            if (send_str (fd, buf) == 1)
                break;
        }

        /* ':GD#' */
    } while (1);
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

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
    struct sigaction sa;
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

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            talk (new_fd);
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
