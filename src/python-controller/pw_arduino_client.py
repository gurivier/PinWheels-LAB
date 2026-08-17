"""
The Python controller of PinWheels@LAB.

File: pw_arduino_client.py
Author: Guillaume RIVIERE
Date: February 2021, May 2023, August 2026
"""

import serial
import termios
import os

import time

arduino_serial = None
valid_codes = ('0', '1', '2', '3', '4')

def is_valid_datagram(datagram):
    global valid_codes
    if len(datagram) == 6:
        return (datagram[:-2].isnumeric() and datagram[0] in valid_codes)
    elif len(datagram) == 7:
        return (datagram[2:-2].isnumeric() and datagram[0] in valid_codes and datagram[1] == '-')
    return False

def arduino_init():
    global arduino_serial
    path = '/dev/ttyUSB0' if os.path.exists('/dev/ttyUSB0') else '/dev/ttyUSB1'
    print(f'serial = {path}')
    os.system(f'stty -F {path} -hupcl')
    with open(path) as f:
        attrs = termios.tcgetattr(f)
        attrs[2] = attrs[2] & ~termios.HUPCL
        termios.tcsetattr(f, termios.TCSAFLUSH, attrs)
    arduino_serial = serial.Serial(port=path, baudrate=9600) #, timeout=0.001
    arduino_serial.flushInput()
    arduino_serial.flushOutput()

def arduino_send(message):
    global arduino_serial
    datagrams = message.split('|')
    for datagram in datagrams:
        datagram += '|'
        if is_valid_datagram(datagram):
            print(f'Transfering datagram: {datagram}')
            arduino_serial.write(datagram.encode('ascii'))
            time.sleep(0.001)
        elif datagram != '|':
            print(f'Ignoring ill-formed datagram: {datagram}')

def arduino_stop():
    global arduino_serial
    arduino_send('00000|')
    arduino_serial.flushOutput()
