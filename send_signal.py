import subprocess
import socket
import time
import math
import threading
import json
import ssl
import paho.mqtt.client as mqtt_client
import numpy as np 

SEND_RATE = 2000

def authenticate(username, password):
    """Authenticate to IoT-LAB using the command line tool."""
    try:
        command = ['iotlab-auth', '-u', username, '-p', password]
        result = subprocess.run(command, text=True, capture_output=True)
        
        if result.returncode == 0:
            print("Authentication successful")
            return True
        else:
            print("Authentication failed:", result.stderr)
            return False
    except Exception as e:
        print("An error occurred while trying to authenticate:", str(e))
        return False

def send_signal(sock, start_time, duration, frequency, amplitude):
    """Send a sinusoidal signal periodically to the IoT device."""
    try:
        current_time = start_time
        end_time = start_time + duration
        
        while current_time < end_time:
            current_time = time.time()
            t = current_time - start_time  # Time variable t
            # Calculate the signal value
            signal = 2*np.sin(2*np.pi*frequency[0]*t)+4*np.sin(2*np.pi*frequency[1]*t)
            message = f'{signal}\n'
            sock.sendall(message.encode())
            # print(f"Signal sent: {signal}")

            # Sleep to control sending rate
            time.sleep(1 / SEND_RATE)
            current_time += (1 / SEND_RATE)

    except Exception as e:
        print("Failed to send signal:", str(e))

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
    else:
        print(f"Failed to connect, return code {rc}")

def on_publish(client, userdata, mid):
    print("Message Published...")

def receive_response(sock, mqtt_client):
    """Receive responses from the IoT device and publish to AWS IoT Core via MQTT."""
    try:
        while True:
            response = sock.recv(1024)
            if response:
                decoded_response = response.decode()
                print("Received response from device:", decoded_response)
                # Publish the response to the MQTT topic

                result = mqtt_client.publish("sdk/test/python", json.dumps({"response": decoded_response}))
                print("Test message publish result:", result.rc)
            else:
                print("No response received from device.")
                break
    except Exception as e:
        print("Failed to receive response:", str(e))

def send_signal_to_device(ip, port, duration, frequency, amplitude, mqtt_client):
    """Send a sinusoidal signal periodically to the IoT device and read responses."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
    try:
        sock.connect((ip, port))
        start_time = time.time()
        
        # Create threads for sending signals and receiving responses
        send_thread = threading.Thread(target=send_signal, args=(sock, start_time, duration, frequency, amplitude))
        receive_thread = threading.Thread(target=receive_response, args=(sock, mqtt_client))
        
        # Start threads
        send_thread.start()
        receive_thread.start()
        
        # Wait for threads to complete
        send_thread.join()
        receive_thread.join()
        
    except Exception as e:
        print("Failed to send signal:", str(e))
    finally:
        sock.close()

# Securely managing credentials
username = 'pferreir'
password = 'Peter5h3!'

client_id = "basicPubSub"
aws_iot_endpoint = "a52peqbfn4vsr-ats.iot.eu-north-1.amazonaws.com"  # replace with your AWS IoT Core endpoint
port = 8883  # Typically, AWS IoT uses port 8883 for secure MQTT connections
root_ca = "./root-CA.crt"  # replace with the path to your root CA certificate
private_key = "./private.key"  # replace with the path to your private key
certificate = "./certificate.pem"  # replace with the path to your certificate

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    if rc == 0:
        print("Connection successful")
    else:
        print("Connection failed")

def on_publish(client, userdata, mid):
    print(f"Message published: {mid}")

mqtt_client = mqtt_client.Client(client_id)
mqtt_client.on_connect = on_connect
mqtt_client.on_publish = on_publish

mqtt_client.tls_set(root_ca, certfile=certificate, keyfile=private_key, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)
mqtt_client.tls_insecure_set(False)

try:
    print("Connecting to MQTT broker...")
    mqtt_client.connect(aws_iot_endpoint, port, keepalive=60)
    print("Connection to MQTT broker initiated.")
except Exception as e:
    print("Failed to connect to MQTT broker:", str(e))

# Start the MQTT client network loop in a separate thread
mqtt_client.loop_start()
print("MQTT client network loop started.")



# Authenticate first
if authenticate(username, password):
    # Device connection details
    device_ip = 'm3-102'  # Device IP address or hostname
    port = 20000         # Port number
    duration = 20        # Duration to send the signal in seconds
    frequency = [10, 15]    # Frequencies for each component of the signal 
    amplitude = [1, 5]    # Amplitudes for each component of the signal 
    
    # Send the sinusoidal signal to the device and receive responses
    send_signal_to_device(device_ip, port, duration, frequency, amplitude, mqtt_client)
else:
    print("Cannot proceed without authentication.")
