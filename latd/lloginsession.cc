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
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "clientsession.h"
#include "lloginsession.h"
#include "lat_messages.h"

lloginSession::lloginSession(class LATConnection &p, 
			     unsigned char remid, unsigned char localid,
			     int fd):
  ClientSession(p, remid, localid, "", clean)
{
    master_fd = dup(fd);
    // This is the socket FD - we dup it because the original
    // will be closed when it changes from a LLOGIN_FD to a PTY
    debuglog(("new llogin session: localid %d, remote id %d\n",
	    localid, remid));
}

int lloginSession::new_session(unsigned char *_remote_node, unsigned char c)
{
    credit = c;

    // Make it non-blocking so we can poll it
    fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    LATServer::Instance()->add_pty(this, master_fd);
    return 0;
}

int lloginSession::connect_parent()
{
    debuglog(("connecting parent for llogin\n"));
    return parent.connect();
}

void lloginSession::connect(char *service, char *port)
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
    
    add_string(buf, &ptr, (unsigned char *)service);
    buf[ptr++] = 0x00; // Source service length/name

    buf[ptr++] = 0x01; // Param type 1
    buf[ptr++] = 0x02; // Param Length 2
    buf[ptr++] = 0x04; // Value 1024
    buf[ptr++] = 0x00; // 

    buf[ptr++] = 0x05; // Param type 5 (Local PTY name)
    add_string(buf, &ptr, (unsigned char*)"llogin");

    // If the user wanted a particular port number then add it 
    // into the message
    if (port[0] != '\0')
    {
	buf[ptr++] = 0x04; // Param type 4 (Remote port name)
	add_string(buf, &ptr, (unsigned char *)port);
	buf[ptr++] = 0x00; // NUL terminated (??)
    }
 
    // Send message...
    reply->header.cmd          = LAT_CCMD_SDATA;
    reply->header.num_slots    = 1;
    reply->slot.remote_session = local_session;
    reply->slot.local_session  = remote_session;
    reply->slot.length         = ptr - sizeof(LAT_SessionData);
    reply->slot.cmd            = 0x9f;

    parent.send_message(buf, ptr, LATConnection::DATA);

}

// Disconnect the local socket
void lloginSession::disconnect_sock()
{
// TODO THIS!!
    LATServer::Instance()->set_fd_state(master_fd, false);
    LATServer::Instance()->delete_session(&parent, local_session, master_fd);
}


// Remote end disconnects or EOF on local PTY
void lloginSession::disconnect_session(int reason)
{
    debuglog(("lloginSession::disconnect_session()\n"));
    // If the reason was some sort of error then send it to 
    // the PTY
    if (reason > 1)
    {
	char *msg = lat_messages::session_disconnect_msg(reason);
	write(master_fd, msg, strlen(msg));
	write(master_fd, "\n", 1);
    }
    connected = false;
    disconnect_sock();
    return;
}


lloginSession::~lloginSession()
{
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
}

void lloginSession::do_read()
{
    debuglog(("lloginSession::do_read(), connected: %d\n", connected));
    if (!connected)
    {
	if (!connect_parent())
	{
	    state = STARTING;
	    
	    // Disable reads on the PTY until we are connected (or it fails)
	    LATServer::Instance()->set_fd_state(master_fd, true);
	}
	else
	{
	    // Service does not exist or we haven't heard of it yet.
	    disconnect_sock();
	}
    }

    if (connected)
    {
	read_pty();
    }
}

void lloginSession::got_connection(unsigned char _remid)
{
    unsigned char buf[1600];
    unsigned char slotbuf[256];
    LAT_SessionReply *reply = (LAT_SessionReply *)buf;
    int ptr = 0;
    int slotptr = 0;

    debuglog(("lloginSession:: got connection for rem session %d\n", _remid));
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

// Called from the slave connection - return the master fd so it can 
// can do I/O on it and close the slave so it gets EOF notification.
int lloginSession::get_port_fd()
{
    return master_fd;
}
