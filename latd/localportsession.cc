/******************************************************************************
    (c) 2001 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <list>
#include <queue>
#include <string>
#include <map>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "clientsession.h"
#include "lloginsession.h"
#include "localportsession.h"
#include "lat_messages.h"

localportSession::localportSession(class LATConnection &p, LocalPort *port,
				   unsigned char remid, unsigned char localid,
				   char *lta, int fd):
    lloginSession(p, remid, localid, lta, clean),
    localport(port)
{
    master_fd = fd; 

    debuglog(("new localport session: localid %d, remote id %d\n",
	      localid, remid));

    state = STARTING;
}


localportSession::~localportSession()
{
    close (master_fd);

    // Invalidate the FD number so it doesn't get closed when
    // the superclass destructors get called.
    master_fd = -1;
    LATServer::Instance()->remove_fd(master_fd);

    // Restart PTY so it can accept new connections
    debuglog(("Restarting PTY for local session\n"));
    localport->restart_pty();
}