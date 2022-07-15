# vnc gateway
You can connect to vnc from outside of intranet. You need one Linux server with public IP.

To build :<br>
  $ make
  
To Use :<br>
  In a VNC Server :<br>
    $ ./gw 2 port3(for local vnc server) port2(for gateway server) IP-ADDRESS(gateway's)<br>
  In a gateway Server:<br>
    $ //gw 1 port1(VNC client will connect to this) port2(gw at VNC server will connect to this one.)<br>
<br>    
Whe you use VNC client:<br>
  Enter IP-ADDRESS:port1.<br>
<br> 
That is all.<br>
<br>
This project is very simple. But it works.
