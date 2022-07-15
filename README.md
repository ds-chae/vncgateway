# vnc gateway
You can connect to vnc from outside of intranet. You need one Linux server with public IP.

To build :
  $ make
  
To Use :<br>
  In a VNC Server :
    $ ./gw 2 port3(for local vnc server) port2(for gateway server) IP-ADDRESS(gateway's)
  In a gateway Server:
    $ //gw 1 port1(VNC client will connect to this) port2(gw at VNC server will connect to this one.)
    
Whe you use VNC client:
  Enter IP-ADDRESS:port1.
 
That is all.

This project is very simple. But it works.
