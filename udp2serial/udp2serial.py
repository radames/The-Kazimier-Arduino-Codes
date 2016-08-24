#from https://github.com/cylinderlight/udp2serial
#working with pyserial on MACOSX

import socket,select
import slip
import time
import serial

UDP_PORT_IN = 9000 # UDP server port
UDP_PORT_OUT = 9001
MAX_UDP_PACKET=512 # max size of incoming packet to avoid sending too much data to the micro
SERIAL_PORT='/dev/tty.usbserial-A9007Rap'
BROADCAST_MODE=False #set to True if you want a broadcast replay instead of unicast


udp_client_address=('127.0.0.1',UDP_PORT_OUT) #where to forward packets coming from serial port
udp_server_address = ('',UDP_PORT_IN) #udp server
udp_broadcast=('<broadcast>',UDP_PORT_OUT) #broadcast address

ser = serial.Serial(SERIAL_PORT,19200) #open the file corrisponding to YUN serial port

udp_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
udp_socket.bind(udp_server_address)
if BROADCAST_MODE:
        udp_socket.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)

slip_processor=slip.slip()

while True:
        (rlist, wlist, xlist) = select.select([udp_socket,ser], [], [])
        if ser in rlist:
                serial_data = ser.read(1)
                slip_processor.append(serial_data)
                slip_packets=slip_processor.decode()
                for packet in slip_packets:
                        if BROADCAST_MODE:
                                udp_socket.sendto(packet,udp_broadcast)
                        else:
                                udp_socket.sendto(packet,udp_client_address)

        if udp_socket in rlist:
                udp_data,udp_client = udp_socket.recvfrom(MAX_UDP_PACKET)
                slip_data=slip_processor.encode(udp_data)
                ser.write(slip_data)
                ser.flush()
                udp_client_address=(udp_client[0],UDP_PORT_OUT)
                time.sleep(0.001) #avoid flooding the microcontroller (to be controlled)



