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

class PortSession: public ServerSession
{
 public:
  PortSession(class LATConnection &p,
	      LAT_SessionStartCmd *cmd,
	      int port_fd,
	      unsigned char remid, unsigned char localid);

  virtual int new_session(unsigned char *remote_node, unsigned char c);

 private:  

};