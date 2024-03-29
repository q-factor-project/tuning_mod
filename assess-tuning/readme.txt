		Notes on using the dtntune utility
		----------------------------------

dtntune is use to tune the Linux host for networking purposes.
Please type "sudo ./dtnmenu" and follow instructions to tune system

Additional Notes:
There are 4 files that are used in conjunction with the Tuning Module: 
i.   user_config.txt 
ii.  gdv.sh 
iii. gdv_100.sh
iv. /tmp/tuningLog 

user_config.txt 
===============
In the user_config.txt, the operator can use certain well known values to 
control how the Tuning Module operates.  So far, there are five parameters
that can be used.  The following is an explanation for them: 

a. learning_mode_only
The learning_mode_only parameter is a no-op parameter for this utility.
It will be removed in the next release. 

b. apply_bios_tuning
The apply_bios_tuning parameter is use to tell the Tuning Module if after
evaluating the BIOS configuration, it should apply the recommendations
itself or not. The default is "n" which means it should make the
recommendations to the DTN operator, but not apply them itself.

c. apply_nic_tuning
The apply_nic_tuning parameter is use to tell the Tuning Module if after
evaluating the NIC configuration, it should apply the recommendations
itself or not. The default is "n" which means it should make the
recommendations to the DTN operator, but not apply them itself.

d. apply_default_system_tuning 
The apply_default_system_tuning parameter is used to tell the Tuning Module if 
it should apply the initial tuning recommendations during the ASSESSMENT phase 
or not. The default value is “n” which means it should not apply the initial
tuning recommendations.                  

e. make_default_system_tuning_perm
The make_default_system_tuning_perm is used to tell the Tuning Module if
it should make permanent any system tunings applied permanent or not. The 
default value is "n" which means it should not make permanent any system
tunings applied. Note that a setting of "y" is only valid if 
apply_default_system_tuning also have a setting of "y".

gdv.sh 
======
The gdv.sh file is a simple shell script file that get default values using the 
“sysctl” utility which is used to configure kernel parameters at runtime. 
The script saves the current values of certain setting we are interested in, 
in a file called “/tmp/current_config.orig”. The Tuning Module then looks at 
these settings during the ASSESSMENT phase and makes recommendations based on 
suggestions from the  https://fasterdata.es.net/ website. The following is an 
explanation for each of the settings that we are currently interested in. 
Note that some settings have a minimum, default and maximum values: 

a. net.core.rmem_max 
The net.core.rmem_max attribute defines the size of the buffer that receives 
UDP packets. The recommended value is 1147483647. 
Note: We found concerning information in the literature that says that setting 
this attribute over 26 MB caused increased packet drop internally in the 
Linux kernel. Additional review and evaluation is needed for rmem_max. (some
sites have conflicting descriptions – mention this as overall size of buffer 
whether tcp or udp.)

b. net.core.wmem_max 
The net.core.wmem_max attribute defines the size of the buffer that writes UDP 
packets. The recommended value is 1147483647. 

c. net.ipv4.tcp_congestion_control 
The net.ipv4.tcp_congestion_control attribute is used to achieve congestion 
avoidance. Transmission Control Protocol (TCP) uses a network 
congestion-avoidance algorithm that includes various aspects of an additive 
increase/multiplicative decrease(AIMD) scheme, along with other schemes 
including slow start and congestion window, to achieve congestion avoidance. 
The TCP	congestion-avoidance algorithm is the primary basis for congestion 
control in the Internet.[1][2][3][4] Per the end-to-end principle, congestion 
control is largely a function of internet hosts, not the network itself. 
There are several variations and versions of the algorithm implemented in 
protocol stacks of operating systems of computers that connect to the Internet. 
Cubic is usually the default in most Linux distribution, but we have found htcp 
usually works better.  
	You might also want to try BBR if it’s available on your system.  
The recommended value is bbr. 

d. net.ipv4.tcp_mtu_probing 
The net.ipv4.tcp_mtu_probing attribute works by sending small packets initially 
and if they are acknowledged successfully, gradually increasing the packet size 
until the correct Path MTU can be found. It is recommended for hosts with jumbo 
frames enabled. 
Note that there are some downsides to jumbo frames as well. All hosts in a 
single broadcast domain must be configured with the same MTU, and this can be 
difficult and error-prone.  Ethernet has no way of detecting an MTU mismatch - 
this is a layer 3 function that requires ICMP signaling in order to work 
correctly. The recommended setting is “1”. 

e. net.core.default_qdisc              
The net.core.default_qdisc attribute sets the default queuing mechanism for 
Linux networking. It has very significant effects on network performance and 
latency. fq_codel is the current best queuing discipline for performance and 
latency on Linux machines. The recommended setting is “fq”. 

f. net.ipv4.tcp_rmem        
The net.ipv4.tcp_rmem attribute is the amount of memory in bytes for read 
(receive) buffers per open socket. It contains the minimum, default and maximum 
values.  The recommended values are 4096 87380 1147483647. 

g. net.ipv4.tcp_wmem 
The net.ipv4.tcp_wmem attribute is the amount of memory in bytes for write 
(transmit) buffers per open socket. It contains the minimum, default and maximum
values.  The recommended values are 4096 65536 1147483647. 

gdv_100.sh 
==========
The gdv_100.sh file is similar to the gdv.sh file, but it used when there is
100 Gig card in play. It also has a couple of additional tunables:

a. net.core.netdev_max_backlog 
This parameter sets the maximum size of the network interface's receive queue. 
The queue is used to store received frames after removing them from the network 
adapter's ring buffer.

b. net.ipv4.tcp_no_metrics_save
By default, TCP saves various connection metrics in the route cache when the connection 
closes, so that connections established in the near future can use these to set initial 
conditions. Usually, this increases overall performance, but may sometimes cause performance 
degradation. If set, TCP will not cache metrics on closing connections.

/tmp/tuningLog 
==============
The tuningLog file contains the output of all the logging that the 
Tuning Module does. 

