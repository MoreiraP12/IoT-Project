import subprocess
import socket
import time
import math
import threading

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
            signal = sum(a * math.sin(2 * math.pi * f * t) for a, f in zip(amplitude, frequency))
            message = f'{signal}\n'
            sock.sendall(message.encode())
            print(f"Signal sent: {signal}")
            time.sleep(0.1)  # Adjust the sleep time as needed
    except Exception as e:
        print("Failed to send signal:", str(e))

def receive_response(sock):
    """Receive responses from the IoT device."""
    try:
        while True:
            response = sock.recv(1024)
            if response:
                print("Received response from device:", response.decode())
            else:
                print("No response received from device.")
                break
    except Exception as e:
        print("Failed to receive response:", str(e))

def send_signal_to_device(ip, port, duration, frequency, amplitude):
    """Send a sinusoidal signal periodically to the IoT device and read responses."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((ip, port))
        start_time = time.time()
        
        # Create threads for sending signals and receiving responses
        send_thread = threading.Thread(target=send_signal, args=(sock, start_time, duration, frequency, amplitude))
        receive_thread = threading.Thread(target=receive_response, args=(sock,))
        
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

# Authenticate first
if authenticate(username, password):
    # Device connection details
    device_ip = 'm3-95'  # Device IP address or hostname
    port = 20000          # Port number
    duration = 100          # Duration to send the signal in seconds
    frequency = [3, 5]    # Frequencies for each component of the signal (3 Hz and 5 Hz)
    amplitude = [2, 4]    # Amplitudes for each component of the signal (2 and 4)
    
    # Send the sinusoidal signal to the device and receive responses
    send_signal_to_device(device_ip, port, duration, frequency, amplitude)
else:
    print("Cannot proceed without authentication.")

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
