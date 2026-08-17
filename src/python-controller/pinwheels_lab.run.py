#!/usr/bin/env python3

"""
The Python controller of PinWheels@LAB.

File: pinwheels_lab.py
Author: Guillaume RIVIERE
Date: February 2021, May 2023, August 2026
"""

import time

from pw_arduino_client import arduino_init, arduino_send, arduino_stop

pname = 'PinWheels@LAB'

def get_program_parameters():
    import argparse, textwrap
    
    parser = argparse.ArgumentParser(
        prog='PinWheels@LAB: Raspberry Pi Client',
        description='This program drives PinWheels@LAB.',
        epilog='Guillaume RIVIERE, 2021-2026')
    
    subparsers = parser.add_subparsers(dest="subparser_name", help='sub-command help')
    
    parsers = {
        'mqtt': subparsers.add_parser('mqtt', help="Execute the messages from an MQTT broker."),
        'prompt': subparsers.add_parser('prompt', help="Execute the messages from user input."),
        'demo': subparsers.add_parser('demo', help="Execute some predefined messages for demonstration.")
    }

    parsers['mqtt'].add_argument('--host', type=str, nargs=1, required=False, help='Host of the MQTT broker (hostname or IP address).')
    
    information = textwrap.dedent(f'''\
    further information:
      project directory    https://github.com/gurivier/PinWheels-LAB
      documentation        https://github.com/gurivier/PinWheels-LAB/wiki
    ''')
    
    usages = ''.join(['  '+p.format_usage().strip('usage: ') for p in parsers.values()])
    parser.formatter_class = argparse.RawTextHelpFormatter
    parser.epilog = textwrap.dedent(f'commands usage:\n{usages}\n{information}')
    
    for subparser in parsers.values():
        subparser.formatter_class = argparse.RawTextHelpFormatter
        subparser.epilog = information
        
    return parser.parse_args()

def usage():
    print('Use option --help to display usage.')
    
def help():
    print('---')
    print('Examples of valid messages:')
    print('  * One frame:        NXYYY|')
    print('  * Several frames:   NXYYY|NXYYY|NXYYY|')
    print('  * Reverse rotation: N-XYYY|')
    print('  * Quit:             q')    
    print('with:')
    print('  * N in [0..4]')
    print('  * X.YYY in [0.000 .. 1.000]')
    print('---')

def run_demo():
    arduino_send('10500|')
    time.sleep(2.0)
    arduino_send('20500|')
    time.sleep(2.0)
    arduino_send('30500|')
    time.sleep(2.0)
    arduino_send('40500|')
    time.sleep(3.0)
    arduino_send('00900|')
    time.sleep(3.0)
    arduino_stop()
    print('End.')

def run_from_prompt():
    prompting = True
    print('>> PROMPT MODE <<')
    help()
    while (prompting):
        prompt = input('Give bytes: ')
        if prompt != 'q':
            arduino_send(prompt)
        else:
            arduino_stop()
            prompting = False
    print('End.')            

def run_from_mqtt(host):
    import paho.mqtt.client as mqtt

    #host = '10.3.141.1'
    port  = 1883
    topic = 'topic-pinwheels'
    
    def on_connect(client, userdata, flags, reason_code, properties):
        print(f"Connected with result code {reason_code}")
        client.subscribe(topic, qos=2)
 
    def on_message(client, userdata, databytes):
        message = str(databytes.payload.decode())
        if message == 'q':
            arduino_stop()
            client.disconnect()
        else:
            arduino_send(message)

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    print(f"Connecting to MQTT host: {host}")
    client.connect(host, port, 60)
    client.loop_forever()
    print('End.')
        
def main():
    args = get_program_parameters()
    if args.subparser_name is not None:
        arduino_init()
    if args.subparser_name == 'mqtt':
        host = args.host[0] if args.host is not None else '127.0.0.1'
        run_from_mqtt(host)
    elif args.subparser_name == 'prompt':
        run_from_prompt()
    elif args.subparser_name == 'demo':
        run_demo()
    else:
        usage()

if __name__ == '__main__':
    import sys
    exit_code = main()
    sys.exit(exit_code)
