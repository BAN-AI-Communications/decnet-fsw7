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
#include <unistd.h>
#include <syslog.h>

#include <strstream>
#include <list>
#include <string>
#include <map>
#include <queue>
#include <iterator>

#include "lat.h"
#include "latcp.h"
#include "utils.h"
#include "services.h"
#include "session.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "llogincircuit.h"
#include "server.h"


LLOGINCircuit::LLOGINCircuit(int _fd):
    Circuit(_fd)
{
}


LLOGINCircuit::~LLOGINCircuit()
{
}

bool LLOGINCircuit::do_command()
{
    unsigned char head[3];
    bool retval = true;
    int ptr;

    debuglog(("llogin: do_command on fd %d\n", fd));
    
    // Get the message header (cmd & length)
    if (read(fd, head, sizeof(head)) != 3)
	return false; // Bad header
    
    int len = head[1] * 256 + head[2];
    unsigned char *cmdbuf = new unsigned char[len];

    // Read the message buffer
    if (read(fd, cmdbuf, len) != len)
    {
	delete[] cmdbuf;
	return false; // Bad message
    }

    // Have we completed negotiation? 
    if (head[0] != LATCP_VERSION &&
	state != RUNNING)
    {
	delete[] cmdbuf;
	return false;
    }
    
    debuglog(("llogin: do_command %d\n", head[0]));
    
    // Do the command
    switch (head[0])
    {
    case LATCP_VERSION:
	if (strcmp(VERSION, (char*)cmdbuf) == 0)
	{
	    state = RUNNING; // Versions match
	    send_reply(LATCP_VERSION, VERSION, -1);
	}
	else
	{
	    debuglog(("Connect from invalid llogin version %s\n", cmdbuf));
	    send_reply(LATCP_ERRORMSG, "login version does not match latd version " VERSION, -1);
	    retval = false;
	}
	break;

    case LATCP_SHOWSERVICE:
    {
	int verbose = cmdbuf[0];
	ostrstream st;

	debuglog(("llogin: show_services(verbose=%d)\n", verbose));

	LATServices::Instance()->list_services(verbose?true:false, st);
	send_reply(LATCP_SHOWSERVICE, st.str(), st.pcount());
    }
    break;

    case LATCP_TERMINALSESSION:
    {
	// Close the llogincircuit and pretend to be a PTY
	// connected to a lloginsession.
	ptr = 0;
	bool queued = cmdbuf[ptr++];
	get_string((unsigned char*)cmdbuf, &ptr, (unsigned char*)service);
	get_string((unsigned char*)cmdbuf, &ptr, (unsigned char*)node);
	get_string((unsigned char*)cmdbuf, &ptr, (unsigned char*)port);

	// Do the biz
	if (LATServer::Instance()->make_llogin_connection(fd,
							  service, 
							  node,
							  port,
							  queued) < 0)
	{
	    debuglog(("sending failure back to llogin\n"));
	    send_reply(LATCP_ERRORMSG, "Error creating client service, service unknown", -1);
	}
	else
	{
	    send_reply(LATCP_ACK, "", -1);
	    state = TERMINAL;
	}
    }
    break;

    default:
	syslog(LOG_INFO, "Unknown llogin command %d\n", head[0]);
	retval = false;
	break;
    }

    delete[] cmdbuf;
    return retval;
}

