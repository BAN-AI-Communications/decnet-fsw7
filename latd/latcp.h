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

/* Definitions of LATCP commands */

const int LATCP_STARTLAT     =  1;
const int LATCP_SHUTDOWN     =  2;
const int LATCP_SETRESPONDER =  3;
const int LATCP_SETRATING    =  4;
const int LATCP_SETNODENAME  =  5;
const int LATCP_SETMULTICAST =  6;
const int LATCP_ZEROCOUNTS   =  7;
const int LATCP_VERSION      =  8;
const int LATCP_ADDSERVICE   =  9;
const int LATCP_REMSERVICE   = 10;
const int LATCP_SHOWCHAR     = 11;
const int LATCP_SHOWSERVICE  = 12;
const int LATCP_SETGROUPS    = 13;
const int LATCP_ADDPORT      = 14;
const int LATCP_REMPORT      = 15;
const int LATCP_ACK          = 16;
const int LATCP_ERRORMSG     = 17; // Fatal