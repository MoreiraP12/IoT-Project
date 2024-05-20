#include <FreeRTOS.h>
#include <platform.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <task.h>
#include <queue.h>
#include <uart.h>

#define SAMPLE_SIZE 16 // Adjusted to a power of 2 for FFT
#define INITIAL_SAMPLE_RATE 100 
#define WINDOW_SIZE_SECONDS 5

typedef struct {
    double real;
    double imag;
} Complex;

// Buffer for storing samples
static Complex *sample_buffer;
static unsigned int sample_index = 0;
static float sample_rate = INITIAL_SAMPLE_RATE;

// Buffer for storing the window of samples for averaging
static float *window_buffer;
static unsigned int window_index = 0;
static unsigned int window_sample_count = 0;

// Complex number operations
Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

Complex complex_sub(Complex a, Complex b) {
    Complex result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

Complex complex_mul(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

Complex complex_from_polar(double magnitude, double phase) {
    Complex result;
    result.real = magnitude * cos(phase);
    result.imag = magnitude * sin(phase);
    return result;
}

// const char *ssid = "";
// const char *pass = "";

// SemaphoreHandle_t xSemaphoreWifiConnected;

// esp_mqtt_client_handle_t client;
// int mqtt_connected = 0;

// const char *aws_uri = "mqtts://[id]-ats.iot.us-east-1.amazonaws.com";
// const char *client_id = "basicPubSub";
// char *topic = "sdk/test/python";

// const char *root_ca_cert = "-----BEGIN CERTIFICATE-----\n"
//                            ""
//                            "-----END CERTIFICATE-----\n";

// const char *cert = "-----BEGIN CERTIFICATE-----\n"
//                    ""
//                    "-----END CERTIFICATE-----\n";

// const char *privkey = "-----BEGIN RSA PRIVATE KEY-----\n"
//                       ""
//                       "-----END RSA PRIVATE KEY-----\n";

// // WiFi
// static void
// wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
// {
//     static int retry_num = 0;

//     if (event_id == WIFI_EVENT_STA_START)
//     {
//         printf("WIFI CONNECTING....\n");
//     }
//     else if (event_id == WIFI_EVENT_STA_CONNECTED)
//     {
//         printf("WiFi CONNECTED\n");
//     }
//     else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
//     {
//         printf("WiFi lost connection\n");
//         if (retry_num < 5)
//         {
//             esp_wifi_connect();
//             retry_num++;
//             printf("Retrying to Connect...\n");
//         }
//     }
//     else if (event_id == IP_EVENT_STA_GOT_IP)
//     {
//         xSemaphoreGive(xSemaphoreWifiConnected);
//         printf("Wifi got IP...\n\n");
//     }
// }

// void wifi_connection()
// {
//     //                          s1.4
//     // 2 - Wi-Fi Configuration Phase
//     esp_netif_init();
//     esp_event_loop_create_default();     // event loop                    s1.2
//     esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
//     wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
//     esp_wifi_init(&wifi_initiation); //
//     esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
//     esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
//     wifi_config_t wifi_configuration = {
//         .sta = {
//             .ssid = "",
//             .password = "",
//         }};
//     strcpy((char *)wifi_configuration.sta.ssid, ssid);
//     strcpy((char *)wifi_configuration.sta.password, pass);
//     // esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
//     esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
//     // 3 - Wi-Fi Start Phase
//     esp_wifi_start();
//     esp_wifi_set_mode(WIFI_MODE_STA);
//     // 4- Wi-Fi Connect Phase
//     esp_wifi_connect();
//     printf("wifi_init_softap finished. SSID:%s  password:%s", ssid, pass);
// }

// // MQTT

// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//     esp_mqtt_event_handle_t event = event_data;      // here esp_mqtt_event_handle_t is a struct which receieves struct event from mqtt app start funtion
//     esp_mqtt_client_handle_t client = event->client; // making obj client of struct esp_mqtt_client_handle_t and assigning it the receieved event client
//     if (event->event_id == MQTT_EVENT_CONNECTED)
//     {
//         mqtt_connected = 1;
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_CONNECTED");
//         esp_mqtt_client_subscribe(client, topic, 0); // in mqtt we require a topic to subscribe and client is from event client and 0 is quality of service it can be 1 or 2
//         ESP_LOGI(APP_NAME, "sent subscribe successful");
//     }
//     else if (event->event_id == MQTT_EVENT_DATA)
//     {
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_DATA");
//         char json[100];
//         memcpy(json, event->data, event->data_len);
//         json[event->data_len] = 0;
//         printf("Received data from MQTT: %s", json);
//     }
//     else if (event->event_id == MQTT_EVENT_DISCONNECTED)
//     {
//         mqtt_connected = 0;
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_DISCONNECTED"); // if disconnected
//     }
//     else if (event->event_id == MQTT_EVENT_SUBSCRIBED)
//     {
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_SUBSCRIBED");
//     }
//     else if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) // when subscribed
//     {
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_UNSUBSCRIBED");
//     }
//     else if (event->event_id == MQTT_EVENT_DATA) // when unsubscribed
//     {
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_DATA");
//     }
//     else if (event->event_id == MQTT_EVENT_ERROR) // when any error
//     {
//         ESP_LOGI(APP_NAME, "MQTT_EVENT_ERROR");
//     }
// }

// static void mqtt_initialize(void)
// { /*Depending on your website or cloud there could be more parameters in mqtt_cfg.*/
//     const esp_mqtt_client_config_t mqtt_cfg = {
//         .broker = {
//             .address.uri = aws_uri, // Uniform Resource Identifier includes path,protocol
//             .verification = {
//                 .certificate = root_ca_cert, // described above event handler
//             }},
//         .credentials = {.authentication = {.certificate = cert, .key = privkey}, .client_id = client_id}
//         // .username = /*your username*/,                                 // your username
//         // .password = /*your adafruit password*/,                        // your adafruit io password
//     };
//     client = esp_mqtt_client_init(&mqtt_cfg);                                           // sending struct as a parameter in init client function
//     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL); // registering event handler
//     esp_mqtt_client_start(client);                                                      // starting the process
// }

// FFT implementation
void fft(Complex *x, int n) {
    if (n <= 1) return;

    // Divide
    Complex even[n/2];
    Complex odd[n/2];
    int i;
    for (i = 0; i < n/2; i++) {
        even[i] = x[i*2];
        odd[i] = x[i*2 + 1];
    }

    // Conquer
    fft(even, n/2);
    fft(odd, n/2);

    // Combine
    int k;
    for (k = 0; k < n/2; k++) {
        Complex t = complex_mul(complex_from_polar(1.0, -2 * M_PI * k / n), odd[k]);
        x[k] = complex_add(even[k], t);
        x[k + n/2] = complex_sub(even[k], t);
    }
}

// Function to perform FFT and find max frequency
static void compute_fft() {
    fft(sample_buffer, SAMPLE_SIZE);

    // Find the index with the maximum magnitude
    float max_magnitude = 0;
    int max_index = 0;
    int i;
    for (i = 0; i < SAMPLE_SIZE / 2; i++) {
        float magnitude = sqrt(sample_buffer[i].real * sample_buffer[i].real + sample_buffer[i].imag * sample_buffer[i].imag);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    // Calculate the corresponding frequency
    float max_frequency = (float)max_index * sample_rate / SAMPLE_SIZE;
    printf("Max frequency: %f Hz\n", max_frequency);

    // Adjust the sample rate to twice the maximum frequency found
    if (max_frequency > 0) {
        sample_rate = 2 * max_frequency;
        printf("Adjusted sampling rate: %f Hz\n", sample_rate);
    } else {
        sample_rate = INITIAL_SAMPLE_RATE;
        printf("Default sampling rate: %f Hz\n", sample_rate);
    }
}

// Function to compute the average of the sampled signal over the window
static void compute_average() {
    float sum = 0;
    unsigned int i;
    for (i = 0; i < window_sample_count; i++) {
        printf("Number: %f\n", window_buffer[i]);
        sum += window_buffer[i];
    }
    float average = sum / window_sample_count;
    //send over mqtt
    // esp_mqtt_client_publish(client, "sdk/test/python", average, 0, 1, 0);
    printf("Average over window: %f\n", average);
}

// Command interpreter
static void interpret_line(char *line) {
    // Convert the input line to a float and store it in the sample buffer
    float value = atof(line);
    if (sample_index < SAMPLE_SIZE) {
        sample_buffer[sample_index].real = value;
        sample_buffer[sample_index].imag = 0;
        sample_index++;
    }

    // Store the value in the window buffer
    if (window_index < (unsigned int)(WINDOW_SIZE_SECONDS * sample_rate)) {
        window_buffer[window_index++] = value;
        window_sample_count++;
    } else {
        // Shift the window buffer left to make space for new samples
        memmove(window_buffer, window_buffer + 1, (WINDOW_SIZE_SECONDS * sample_rate - 1) * sizeof(float));
        window_buffer[(unsigned int)(WINDOW_SIZE_SECONDS * sample_rate) - 1] = value;
    }

    // If the buffer is full, compute the FFT
    if (sample_index == SAMPLE_SIZE) {
        compute_fft();
        sample_index = 0; // Reset the buffer index for new samples
    }

    // Compute the average if the window is full
    if (window_sample_count >= WINDOW_SIZE_SECONDS * sample_rate) {
        compute_average();
    }
}

// UART receive handler
static xQueueHandle char_queue;
static void char_rx(void *arg, uint8_t c) {
    xQueueSendFromISR(char_queue, &c, 0);
}

// Buffer handling
static unsigned int buff_index = 0;
static char buff[1024];
static void flush_buff() {
    if (buff_index == sizeof(buff)) return; // Prevent buffer overflow

    buff[buff_index] = '\0';
    interpret_line(buff);
    buff_index = 0;
}

static void read_line() {
    char value;
    while (xQueueReceive(char_queue, &value, 0) == pdTRUE) {
        switch (value) {
        case '\n':
            flush_buff();
            break;
        default:
            if (buff_index < sizeof(buff))
                buff[buff_index++] = value;
            else
                flush_buff();
        }
    }
}

// RTOS task
static void app_task(void *param) {
    printf("FFT serial server started.\n");
    while (1){
        read_line();
        vTaskDelay(configTICK_RATE_HZ / sample_rate);
    } 
        
}

// Main function
int main() {
    platform_init();
    
    // xSemaphoreWifiConnected = xSemaphoreCreateBinary();
    // wifi_connection();
    // 
    // // MQTT
    // xSemaphoreTake(xSemaphoreWifiConnected, portMAX_DELAY);
    // mqtt_initialize();

    char_queue = xQueueCreate(8, sizeof(char));
    uart_set_rx_handler(uart_print, char_rx, NULL);
    
    // Allocate memory for the sample buffer
    sample_buffer = (Complex *)malloc(SAMPLE_SIZE * sizeof(Complex));
    if (sample_buffer == NULL) {
        printf("Failed to allocate memory for sample buffer\n");
        return -1;
    }

    // Allocate memory for the window buffer
    window_buffer = (float *)malloc(WINDOW_SIZE_SECONDS * INITIAL_SAMPLE_RATE * sizeof(float));
    if (window_buffer == NULL) {
        printf("Failed to allocate memory for window buffer\n");
        free(sample_buffer);
        return -1;
    }

    // Create the app task with increased stack size
    xTaskCreate(app_task, (const signed char * const)"app", 1024, NULL, 1, NULL);

    platform_run();

    // Free memory for the sample buffer and window buffer
    free(sample_buffer);
    free(window_buffer);
    return 0;
}
