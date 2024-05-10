import numpy as np
import socket
import argparse

def generate_signal(duration, sampling_rate):
    """
    Generates the signal 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t) for a given duration and sampling rate.
    
    :param duration: Duration of the signal in seconds.
    :param sampling_rate: Sampling rate in Hz.
    :return: Array of time steps and corresponding signal values.
    """
    t = np.linspace(0, duration, int(duration * sampling_rate), endpoint=False)
    signal = 2 * np.sin(2 * np.pi * 3 * t) + 4 * np.sin(2 * np.pi * 5 * t)
    return t, signal

def send_data_to_m3(signal, host, port):
    """
    Sends the generated signal data to the M3 node via TCP socket.
    
    :param signal: Array of signal values to send.
    :param host: Hostname of the M3 node.
    :param port: Port number to connect to.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((host, port))
        for value in signal:
            message = f"{value}\n".encode('utf-8')
            sock.sendall(message)
        print("Data sent successfully")

def main():
    # Setting up Argument Parser
    parser = argparse.ArgumentParser(description="Send signal data to IoT-Lab M3 node.")
    parser.add_argument("host", type=str, help="Hostname of the M3 node, e.g., 'm3-100'")
    parser.add_argument("--port", type=int, default=20000, help="Port number to connect to, default is 20000")
    args = parser.parse_args()

    # Parameters
    duration = 5  # seconds
    sampling_rate = 100  # Hz

    # Generate the signal
    t, signal = generate_signal(duration, sampling_rate)

    # Send the signal to the M3 node
    send_data_to_m3(signal, args.host, args.port)

if __name__ == "__main__":
    main()
