#ifndef EPOLL_FUNCTION_H
#define EPOLL_FUNCTION_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

int setnonblocking( int fd );
void addfd( int epollfd, int fd, bool oneshot );
void removefd( int epollfd, int fd );
void modfd( int epollfd, int fd, int ev );

#endif