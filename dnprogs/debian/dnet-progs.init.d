#!/bin/sh
#
# dnet-progs.sh 
#
# Starts/stops DECnet processes
#
# --------------------------------------------------------------------------
#
#
# Daemons to start are defined in /etc/default/dnet-progs
#

. /etc/default/decnet

case $1 in
   start)
     # Don't issue any messages if DECnet is not configured as
     # dnet-common will have taken care of those.
     if [ ! -f /etc/decnet.conf -o ! -f /proc/net/decnet ]
     then
       exit 1
     fi

     echo -n "Starting DECnet daemons: "

     for i in $DNET_DAEMONS
     do
       if [ -f /usr/sbin/$i ]
       then
         start-stop-daemon --start --quiet --exec /usr/sbin/$i
         echo -n " $i"
       fi
     done
     echo "."
     ;;

   stop)
     echo -n "Stopping DECnet... "
     for i in $DNET_DAEMONS
     do
       start-stop-daemon --stop --quiet --exec /usr/sbin/$i
     done
     echo "done."
     ;;

   restart|force-reload)
     echo -n "Restarting DECnet: "
     for i in $DNET_DAEMONS
     do
       start-stop-daemon --stop --quiet --exec /usr/sbin/$i
       start-stop-daemon --start --quiet --exec /usr/sbin/$i
       echo -n " $i"
     done
     echo " done."
     ;;

   *)
     echo "Usage $0 {start|stop|restart|force-reload}"
     ;;
esac

exit 0