# Time Synchronizer

An external interrupt is generated on GPIO of TX2 to make a time synchronizer for hardware, like sensors. Pin GPIO8_ALS_PROX_INT of TX2 is used for this feature. In Linux Kernel this pin is mapped to gpio388 (http://www.jetsonhacks.com/nvidia-jetson-tx2-j21-header-pinout/). On the developer kit this pin is breakout to J21 pin 37.

## Test on Developer Kit

### Testbench for the test of the time synchronizer
![Optional Text](../master/photo_interrupter_2_TX2.jpg)

### Build loadable kernel module (LKM) for the time synchronizer
```
$sudo mkdir ~/lkm/time_synchronizer
```
Copy the code of the LKM into this folder:
```
$cd ~/lkm/time_synchronizer
$sudo su
$make
```
Do sudo without password:
```
$sudo visudo
```
Add this line at the end:
```
nvidia ALL=(ALL) NOPASSWD: ALL
```

### Insert LKM at bootup and remove it at shutdown or reboot

Create file '.startup.commands' in the /home/nvidia:
```
#!/bin/sh
#
# Insert time synchronizer kernel module
sudo insmod /home/nvidia/lkm/time_synchronizer/time_synchronizer_lkm.ko
# Enable write access to the sysfs variables
sudo chmod 0666 /sys/ts_lkm/gpio388/timeInterrupt
sudo chmod 0666 /sys/ts_lkm/gpio388/numberInterrupts
sudo chmod 0666 /sys/ts_lkm/gpio388/isDebounce
```

The LKM will be insert at bootup time. The writable accessibility of the LKM variables to the user space applications is enabled here, as in the new Linux kernel, writing to these variables is restricted and the kernel module code cannot pass the compilation with writable accessibility. 

Create file '.shutdown.commands' in /home/nvidia folder:
```
#!/bin/sh
#
 
# Remove the kernel modules before shutdown
sudo rmmod time_synchronizer_lkm
```

With this script, at shutdown period the LKM will be removed.

Creat file 'start_and_stop.service' in /etc/systemd/system/:
```
[Unit]
Description=Run Scripts at Start and Stop
 
[Service]
Type=oneshot
RemainAfterExit=true
ExecStart=/home/nvidia/.startup_commands
ExecStop=/home/nvidia/.shutdown_commands
 
[Install]
WantedBy=multi-user.target
```

Enable the above serive:
```
$systemctl enable start_and_stop
```

### Test the time synchronizer in ROS on developer kit
Wire the hardware as in above figure.

Run the following command:
```
$roslaunch vb_comms time_synchronizer.launch
```
Use black paper to go through the photo interrupter, interrupt time can be shown up. White paper board seems not work as well, the high level logic only goes to 1.2V.
