/******************************************************************************
    (c) 2000 Patrick Caulfield                 patrick@pandh.demon.co.uk

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
#include <list>
#include <queue>
#include <string>
#include <map>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"
#include "clientsession.h"

ClientSession::ClientSession(class LATConnection &p, 
			     unsigned char remid, unsigned char localid,
			     char *ttyname):
  LATSession(p, remid, localid)
{
    debuglog(("new client session: localid %d, remote id %d\n",
	    localid, remid));
    if (ttyname) strcpy(ltaname, ttyname);
}

int ClientSession::new_session(unsigned char *_remote_node, unsigned char c)
{
    credit = c;
    if (openpty(&master_fd,
		&slave_fd, NULL, NULL, NULL) != 0)
	return -1; /* REJECT */
  
    strcpy(remote_node, (char *)_remote_node);
    strcpy(ptyname, ttyname(slave_fd));
    strcpy(mastername, ttyname(master_fd));
    state = STARTING;
    
    // Check for /dev/lat & create it if necessary
    struct stat st;
    if (stat("/dev/lat", &st) == -1)
    {
	mkdir("/dev/lat", 0755);
    }

    // Link the PTY to the actual LTA name we were passed
    if (ltaname[0] == '\0')
    {
	sprintf(ltaname, "/dev/lat/lta%d", local_session);
    }
    unlink(ltaname);
    symlink(ptyname, ltaname);

    fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    debuglog(("made symlink %s to %s\n", ltaname, ptyname));
    LATServer::Instance()->add_pty(this, master_fd);
    return 0;
}

void ClientSession::connect_parent()
{
    parent.connect();
}

void ClientSession::connect()
{
    debuglog(("connecting client session to '%s'\n", remote_node));

    // OK, now send a Start message to the remote end.
    unsigned char buf[1600];
    memset(buf, 0, sizeof(buf));
    LAT_SessionData *reply = (LAT_SessionData *)buf;
    int ptr = sizeof(LAT_SessionData);
    
    buf[ptr++] = 0x01; // Service Class
    buf[ptr++] = 0x01; // Max Attention slot size
    buf[ptr++] = 0xfe; // Max Data slot size
    
    add_string(buf, &ptr, (unsigned char *)remote_node); 
    buf[ptr++] = 0x00; // Source service length/name

    buf[ptr++] = 0x01; // Param type 1
    buf[ptr++] = 0x02; // Param Length 2
    buf[ptr++] = 0x04; // Value 1024
    buf[ptr++] = 0x00; // 

    buf[ptr++] = 0x05; // Param type 5 (Local PTY name)
                       // INFO: Param type 4 is remote port name.
    add_string(buf, &ptr, (unsigned char *)ltaname);
    buf[ptr++] = 0x00; // NUL terminated (??)
 
    // Send message...
    reply->header.cmd          = LAT_CCMD_SDATA;
    reply->header.num_slots    = 1;
    reply->slot.remote_session = local_session;
    reply->slot.local_session  = remote_session;
    reply->slot.length         = ptr - sizeof(LAT_SessionData);
    reply->slot.cmd            = 0x9f;

    parent.send_message(buf, ptr, LATConnection::DATA);

}

void ClientSession::disconnect()
{
    
    // Close it all down so the local side gets EOF
    unlink(ltaname);
    
    close (slave_fd);
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
    
    // Now open it all up again ready for a new connection
    new_session((unsigned char *)remote_node, 0);    
}

ClientSession::~ClientSession()
{
    unlink(ltaname);
    
    close (slave_fd);
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
}

void ClientSession::do_read()
{
    debuglog(("ClientSession::do_read()\n"));
    if (!connected)
    {
	connect_parent();
	state = STARTING;

	// Disable reads on the PTY until we are connected (or it failed)
	LATServer::Instance()->set_fd_state(master_fd, true);
    }

    if (connected)
    {
	read_pty();
    }
}

void ClientSession::got_connection(unsigned char _remid)
{
    unsigned char buf[1600];
    unsigned char slotbuf[256];
    LAT_SessionReply *reply = (LAT_SessionReply *)buf;
    int ptr = 0;
    int slotptr = 0;

    debuglog(("ClientSession:: got connection for rem session %d\n", _remid));
    LATServer::Instance()->set_fd_state(master_fd, false);
    remote_session = _remid;

    // Send a data_b slot
    reply->header.cmd       = LAT_CCMD_SDATA;
    reply->header.num_slots = 0;
    reply->slot.length      = 0;
    reply->slot.cmd         = 0x0;
    reply->slot.local_session = 0;
    reply->slot.remote_session = 0;

    slotbuf[slotptr++] = 0x26; // Flags
    slotbuf[slotptr++] = 0x13; // Stop  output char XOFF
    slotbuf[slotptr++] = 0x11; // Start output char XON
    slotbuf[slotptr++] = 0x13; // Stop  input char  XOFF
    slotbuf[slotptr++] = 0x11; // Start input char  XON

    // data_b slots count against credit
    if (credit)
    {
	add_slot(buf, ptr, 0xaf, slotbuf, slotptr);
	credit--;
    }
    parent.queue_message(buf, ptr);


    connected = true;
}
