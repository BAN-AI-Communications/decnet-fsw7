/******************************************************************************
    (c) 2002 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#define HAVE_AF_LINK
// interfaces-bpf.h
// BPF implementation of LATinterfaces
class BPFInterfaces : public LATinterfaces
{
 public:

    BPFInterfaces() { _latd_bpf_interface_name = NULL; };
    ~BPFInterfaces() {};

    // Initialise
    virtual int Start(int proto);

    // Return a list of valid interface numbers and the count
    virtual void get_all_interfaces(int *ifs, int &num);

    // Print the name of an interface
    virtual std::string ifname(int ifn);

    // Find an interface number by name
    virtual int find_interface(char *name);

    // true if this class defines one FD for each active
    // interface, false if one fd is used for all interfaces.
    virtual bool one_fd_per_interface();

    // Return the FD for this interface (will only be called once for
    // select if above returns false)
    virtual int get_fd(int ifn);

    // Send a packet to a given macaddr
    virtual int send_packet(int ifn, unsigned char macaddr[], unsigned char *data, int len);

    // Receive a packet from a given interface
    virtual int recv_packet(int sockfd, int &ifn, unsigned char macaddr[], unsigned char *data, int maxlen);

    // Open a connection on an interface
    virtual int set_lat_multicast(int ifn);

    // Close an interface.
    virtual int remove_lat_multicast(int ifn);

    // Bind a socket to an interface
    virtual int bind_socket(int interface);

    /* these are bad, but they get the job done for now.  we only
       support using a single interface: */
    int _latd_bpf_fd;
    char *_latd_bpf_interface_name;
    unsigned char _latd_bpf_interface_addr[ETHER_ADDR_LEN];
    unsigned int _latd_bpf_buffer_size;
    unsigned char *_latd_bpf_buffer;
    unsigned int _latd_bpf_buffer_end;
    unsigned int _latd_bpf_buffer_offset;

 private:
};
