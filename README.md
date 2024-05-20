# Individual Assignment, IoT 2024
Pedro Moreira - 2162606

## Introduction

This project aims to create an IoT system that collects sensor data, processes it locally, and communicates aggregated values to a nearby server. The system adapts its sampling frequency to save energy and reduce communication overhead. The IoT device is based on an ESP32 prototype board, and the firmware is developed using FreeRTOS. The implementation is done using IoT-LAB.

### Objectives

1. **Signal Generation**: Generate an input signal as a sum of sinusoidal functions.
2. **Maximum Sampling Frequency**: Identify the maximum sampling frequency of the ESP32 hardware.
3. **Optimal Sampling Frequency**: Compute the FFT to determine the dominant frequency and adjust the sampling frequency accordingly.
4. **Aggregate Function**: Compute the average value of the sampled signal over a specified window.
5. **Secure Communication**: Transmit the aggregate value securely to a nearby server using MQTT.
6. **Performance Evaluation**: Measure energy savings, communication overhead, and end-to-end latency.

This project demonstrates an efficient and adaptive IoT system capable of real-time data processing and communication.


## Signal Generation

In this project, signal generation is performed using a Python script that sends a sinusoidal signal to an IoT device via TCP. The signal is generated as a sum of sinusoidal functions. Below is an explanation of the key components and decisions behind the implementation, along with relevant code snippets.

### Authentication

First, we authenticate to IoT-LAB using the command line tool to ensure secure access to the platform.

#### Code Snippet: Authentication

```python
def authenticate(username, password):
    """Authenticate to IoT-LAB using the command line tool."""
    command = ['iotlab-auth', '-u', username, '-p', password]
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode == 0:
        print("Authentication successful")
        return True
    else:
        print("Authentication failed:", result.stderr)
        return False
```

### Signal Generation and Transmission

The `send_signal` function generates a sinusoidal signal and sends it periodically to the IoT device. This function calculates the signal value based on the current time and the defined frequency components, then sends the signal over a TCP socket.

#### Code Snippet: Signal Generation

```python
def send_signal(sock, start_time, duration, frequency, amplitude):
    """Send a sinusoidal signal periodically to the IoT device."""
    while time.time() < start_time + duration:
        t = time.time() - start_time
        signal = 2 * np.sin(2 * np.pi * frequency * t) + 4 * np.sin(2 * np.pi * frequency * t)
        message = f'{signal}\n'
        sock.sendall(message.encode())
        time.sleep(1 / SEND_RATE)
```

### Receiving Responses

The `receive_response` function listens for responses from the IoT device and publishes them to an MQTT broker. This ensures that the data is securely transmitted to an AWS IoT Core endpoint.

#### Code Snippet: Receiving Responses

```python
def receive_response(sock, mqtt_client):
    """Receive responses from the IoT device and publish to AWS IoT Core via MQTT."""
    while True:
        response = sock.recv(1024)
        if response:
            decoded_response = response.decode()
            print("Received response from device:", decoded_response)
            mqtt_client.publish("sdk/test/python", json.dumps({"response": decoded_response}))
        else:
            break
```

### Establishing Connection and Communication

The `send_signal_to_device` function sets up the TCP connection and manages separate threads for sending signals and receiving responses. This ensures efficient and simultaneous handling of both tasks.

#### Code Snippet: Connection Setup

```python
def send_signal_to_device(ip, port, duration, frequency, amplitude, mqtt_client):
    """Send a sinusoidal signal periodically to the IoT device and read responses."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
    sock.connect((ip, port))
    start_time = time.time()
    send_thread = threading.Thread(target=send_signal, args=(sock, start_time, duration, frequency, amplitude))
    receive_thread = threading.Thread(target=receive_response, args=(sock, mqtt_client))
    send_thread.start()
    receive_thread.start()
    send_thread.join()
    receive_thread.join()
    sock.close()
```

### Secure MQTT Connection

The MQTT connection is set up using TLS for secure communication with AWS IoT Core. This ensures that the data transmitted to the server is encrypted and secure.

#### Code Snippet: MQTT Setup

```python
mqtt_client = mqtt_client.Client(client_id)
mqtt_client.tls_set(root_ca, certfile=certificate, keyfile=private_key, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2)
mqtt_client.connect(aws_iot_endpoint, port, keepalive=60)
mqtt_client.loop_start()
```

By following these steps, we ensure that the signal generation and communication processes are secure, efficient, and reliable. We could have seperated the functionalities for sending and receiving signal into seperate files (it would actually be easier since I wouldn't need to deal with threads). However due to the nature of FIT/IoT-Lab that proved very tricky so I decided to implement everything in just one file.


Certainly! Here is the complete chapter, including the sections on identifying the maximum sampling frequency and optimizing the sampling frequency based on FFT analysis:


## Maximum Sampling Frequency

We aim to identify the maximum sampling frequency of the ESP32 hardware. This involves sampling at a high frequency, performing a Fast Fourier Transform (FFT) on the sampled data, and dynamically adjusting the sampling rate based on the detected dominant frequency.

### Methodology

To achieve high-frequency sampling, we use FreeRTOS tasks to manage the timing and execution of our sampling process. The FFT is implemented using the Cooley-Tukey approach to analyze the frequency components of the sampled signal, and the sampling rate is adjusted accordingly to ensure efficient and accurate data collection.

### Implementation Details

#### Buffer and Sample Rate Initialization

We start by initializing buffers to store the sampled data and defining an initial sample rate. The sample buffer is used to store complex numbers for the FFT, while the window buffer is used to compute averages over a specified window of time.

```c
#define SAMPLE_SIZE 16 // Adjusted to a power of 2 for FFT
#define INITIAL_SAMPLE_RATE 100 
#define WINDOW_SIZE_SECONDS 5

static Complex *sample_buffer;
static unsigned int sample_index = 0;
static float sample_rate = INITIAL_SAMPLE_RATE;

static float *window_buffer;
static unsigned int window_index = 0;
static unsigned int window_sample_count = 0;
```

#### FFT Implementation

The FFT is implemented using the Cooley-Tukey approach, a recursive algorithm that divides the problem into smaller parts and combines the results. This allows us to efficiently compute the frequency components of the signal.

```c
void fft(Complex *x, int n) {
    if (n <= 1) return;

    Complex even[n/2];
    Complex odd[n/2];
    for (int i = 0; i < n/2; i++) {
        even[i] = x[i*2];
        odd[i] = x[i*2 + 1];
    }

    fft(even, n/2);
    fft(odd, n/2);

    for (int k = 0; k < n/2; k++) {
        Complex t = complex_mul(complex_from_polar(1.0, -2 * M_PI * k / n), odd[k]);
        x[k] = complex_add(even[k], t);
        x[k + n/2] = complex_sub(even[k], t);
    }
}
```

#### Finding the Maximum Frequency

After performing the FFT, we find the index with the maximum magnitude, which corresponds to the dominant frequency in the signal. This frequency is then used to adjust the sampling rate.

```c
static void compute_fft() {
    fft(sample_buffer, SAMPLE_SIZE);

    float max_magnitude = 0;
    int max_index = 0;
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float magnitude = sqrt(sample_buffer[i].real * sample_buffer[i].real + sample_buffer[i].imag * sample_buffer[i].imag);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    float max_frequency = (float)max_index * sample_rate / SAMPLE_SIZE;
    printf("Max frequency: %f Hz\n", max_frequency);

    if (max_frequency > 0) {
        sample_rate = 2 * max_frequency;
        printf("Adjusted sampling rate: %f Hz\n", sample_rate);
    } else {
        sample_rate = INITIAL_SAMPLE_RATE;
        printf("Default sampling rate: %f Hz\n", sample_rate);
    }
}
```

### Task Management

An RTOS task is created to handle the sampling process. This task reads data from the UART, processes it, and periodically performs the FFT to adjust the sampling rate.

```c
static void app_task(void *param) {
    printf("FFT serial server started.\n");
    while (1){
        read_line();
        vTaskDelay(configTICK_RATE_HZ / sample_rate);
    } 
}
```

### Optimizing Sampling Frequency

To ensure the optimal sampling frequency, the system computes the FFT on the sampled data to identify the dominant frequency. The sampling frequency is then adjusted to twice the maximum frequency found in the signal to meet the Nyquist criterion and ensure accurate data representation.

### Conclusion

By implementing this approach, we can dynamically adjust the sampling rate based on the dominant frequency detected in the signal. This allows us to achieve high-frequency sampling efficiently while minimizing energy consumption and communication overhead.

Sure! Here is the paragraph for computing the aggregate function over a window, along with relevant code snippets and explanations:

## Computing Aggregate Function Over a Window

We compute the average of the sampled signal over a specified window of time, such as 5 seconds. This aggregate function provides a smoothed representation of the signal, which is useful for reducing noise and identifying trends in the data.

### Methodology

The process involves storing the sampled values in a buffer and computing the average value over the window once the buffer is full. This approach ensures that we maintain a rolling average of the signal, updating it as new data arrives.

### Implementation Details

#### Buffer Initialization

We initialize a buffer to store the sampled values over the defined window period. The buffer size is calculated based on the window duration and the current sampling rate.

```c
#define WINDOW_SIZE_SECONDS 5

static float *window_buffer;
static unsigned int window_index = 0;
static unsigned int window_sample_count = 0;
```

#### Storing Samples

As new samples are received, they are stored in the window buffer. If the buffer is full, the oldest sample is removed to make space for the new one, maintaining a rolling window of the most recent samples.

```c
static void interpret_line(char *line) {
    // Convert the input line to a float and store it in the window buffer
    float value = atof(line);
    if (window_index < (unsigned int)(WINDOW_SIZE_SECONDS * sample_rate)) {
        window_buffer[window_index++] = value;
        window_sample_count++;
    } else {
        // Shift the window buffer left to make space for new samples
        memmove(window_buffer, window_buffer + 1, (WINDOW_SIZE_SECONDS * sample_rate - 1) * sizeof(float));
        window_buffer[(unsigned int)(WINDOW_SIZE_SECONDS * sample_rate) - 1] = value;
    }

    // Compute the average if the window is full
    if (window_sample_count >= WINDOW_SIZE_SECONDS * sample_rate) {
        compute_average();
    }
}
```

#### Computing the Average

The `compute_average` function calculates the average of the values stored in the window buffer. This function is called whenever the buffer is full, ensuring that the aggregate value is updated in real-time as new data arrives.

```c
static void compute_average() {
    float sum = 0;
    unsigned int i;
    for (i = 0; i < window_sample_count; i++) {
        sum += window_buffer[i];
    }
    float average = sum / window_sample_count;
    printf("Average over window: %f\n", average);
}
```

### Conclusion

By computing the average of the sampled signal over a specified window, we obtain a smoothed representation of the data that reduces noise and highlights underlying trends. This approach is particularly useful for applications that require stable and reliable signal analysis.

## Communicating the Aggregate Value to the Nearby Server

We transmit the computed aggregate value (i.e., the average) of the sampled signal to a nearby edge server using MQTT. This ensures that the data collected and processed by the IoT device is securely sent to the server for further analysis or action.

#### Note on IoT-LAB Limitation
Due to the limitation of IoT-LAB not being able to connect to the internet, the code for establishing a Wi-Fi connection and initializing the MQTT client on the ESP32 is commented out. Instead, the aggregate value is transmitted to a nearby edge device, which then forwards it over MQTT. 

#### Transmitting the Aggregate Value
The computed average is `transmitted` to the MQTT broker using the after being sent to the edge device using the prontf command. The edge device is listening and as soon as it receives the data sends it to the to the broker by taking advatange of the **paho.mqtt** library. 

```python
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
```

However if we're running on a ESP32 board we can directly send to the mqtt broker. Firstly we have to connect to the wifi:

```c

// WiFi
static void
wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static int retry_num = 0;

    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        if (retry_num < 5)
        {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        xSemaphoreGive(xSemaphoreWifiConnected);
        printf("Wifi got IP...\n\n");
    }
}

void wifi_connection()
{                      
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                   
    esp_netif_create_default_wifi_sta(); // WiFi station                      
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
        }};
    strcpy((char *)wifi_configuration.sta.ssid, ssid);
    strcpy((char *)wifi_configuration.sta.password, pass);;
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // Wi-Fi Connect Phase
    esp_wifi_connect();
    printf("wifi_init_softap finished. SSID:%s  password:%s", ssid, pass);
}
```

After the semaphore will allow us to initialize the mqtt and we can then send the data when it's ready to a topic of our choice as long as our credential and certificates are valid.

```c
// MQTT

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;      // here esp_mqtt_event_handle_t is a struct which receieves struct event from mqtt app start funtion
    esp_mqtt_client_handle_t client = event->client; // making obj client of struct esp_mqtt_client_handle_t and assigning it the receieved event client
    if (event->event_id == MQTT_EVENT_CONNECTED)
    {
        mqtt_connected = 1;
        ESP_LOGI(APP_NAME, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, topic, 0); // in mqtt we require a topic to subscribe and client is from event client and 0 is quality of service it can be 1 or 2
        ESP_LOGI(APP_NAME, "sent subscribe successful");
    }
    else if (event->event_id == MQTT_EVENT_DATA)
    {
        ESP_LOGI(APP_NAME, "MQTT_EVENT_DATA");
        char json[100];
        memcpy(json, event->data, event->data_len);
        json[event->data_len] = 0;
        printf("Received data from MQTT: %s", json);
    }
    else if (event->event_id == MQTT_EVENT_DISCONNECTED)
    {
        mqtt_connected = 0;
        ESP_LOGI(APP_NAME, "MQTT_EVENT_DISCONNECTED"); // if disconnected
    }
    else if (event->event_id == MQTT_EVENT_SUBSCRIBED)
    {
        ESP_LOGI(APP_NAME, "MQTT_EVENT_SUBSCRIBED");
    }
    else if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) // when subscribed
    {
        ESP_LOGI(APP_NAME, "MQTT_EVENT_UNSUBSCRIBED");
    }
    else if (event->event_id == MQTT_EVENT_DATA) // when unsubscribed
    {
        ESP_LOGI(APP_NAME, "MQTT_EVENT_DATA");
    }
    else if (event->event_id == MQTT_EVENT_ERROR) // when any error
    {
        ESP_LOGI(APP_NAME, "MQTT_EVENT_ERROR");
    }
}

static void mqtt_initialize(void)
{ 
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = aws_uri, 
            .verification = {
                .certificate = root_ca_cert, 
            }},
        .credentials = {.authentication = {.certificate = cert, .key = privkey}, .client_id = client_id}
        
    };
    client = esp_mqtt_client_init(&mqtt_cfg);                                           
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL); 
    esp_mqtt_client_start(client);                                                      
}
```