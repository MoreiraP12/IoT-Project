import subprocess
import socket
import time
import math

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

def send_signal_to_device(ip, port, duration, frequency, amplitude):
    """Send a sinusoidal signal periodically to the IoT device and read responses."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((ip, port))
        start_time = time.time()
        current_time = start_time
        end_time = start_time + duration
        
        while current_time < end_time:
            current_time = time.time()
            t = current_time - start_time  # Time variable t
            # Calculate the signal value
            signal = sum(a * math.sin(2 * math.pi * f * t) for a, f in zip(amplitude, frequency))
            message = f'{signal}\n'
            sock.sendall(message.encode())
            print(f"Signal sent: {signal}")
            
            # Read response from the device
            response = sock.recv(1024)
            if response:
                print("Received response from device:", response.decode())
            else:
                print("No response received from device.")
            
    except Exception as e:
        print("Failed to send signal:", str(e))
    finally:
        sock.close()

# Securely managing credentials
username = 'pferreir'
password = 'Peter5h3!'

# Authenticate first
if authenticate(username, password):
    # Device connection details
    device_ip = 'm3-102'  # Device IP address or hostname
    port = 20000          # Port number
    duration = 8         # Duration to send the signal in seconds
    frequency = [3, 5]    # Frequencies for each component of the signal
    amplitude = [2, 4]    # Amplitudes for each component of the signal
    
    # Send the sinusoidal signal to the device and receive responses
    send_signal_to_device(device_ip, port, duration, frequency, amplitude)
else:
    print("Cannot proceed without authentication.")
