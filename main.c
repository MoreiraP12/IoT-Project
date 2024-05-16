#include <platform.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <uart.h>

#define SAMPLE_SIZE 100 // Adjust this to a smaller value to fit in the stack
#define SAMPLE_RATE 1000 // Adjust this to your actual sample rate

typedef struct {
    double real;
    double imag;
} Complex;

// Buffer for storing samples
static Complex *sample_buffer;
static unsigned int sample_index = 0;

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
    float max_frequency = (float)max_index * SAMPLE_RATE / SAMPLE_SIZE;
    printf("Max frequency: %f Hz\n", max_frequency);
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

    // If the buffer is full, compute the FFT
    if (sample_index == SAMPLE_SIZE) {
        compute_fft();
        sample_index = 0; // Reset the buffer index for new samples
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
        case '\r':
            break; // Ignore and handle only \n
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
    while (1) read_line();
}

// Main function
int main() {
    platform_init();
    char_queue = xQueueCreate(8, sizeof(char));
    uart_set_rx_handler(uart_print, char_rx, NULL);
    
    // Allocate memory for the sample buffer
    sample_buffer = (Complex *)malloc(SAMPLE_SIZE * sizeof(Complex));
    if (sample_buffer == NULL) {
        printf("Failed to allocate memory for sample buffer\n");
        return -1;
    }

    // Create the app task with increased stack size
    xTaskCreate(app_task, (const signed char * const)"app", 1024, NULL, 1, NULL);


    platform_run();

    // Free memory for the sample buffer
    free(sample_buffer);
    return 0;
}
