#include "hardware/uart.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define MAX_NMEA_LENGTH 256

typedef struct
{
    float latitude;
    float longitude;
    bool is_valid;
} GPSData;


void uart_gps_init()
{
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);
}

bool validate_nmea_checksum(char *nmea_string)
{
    int len = strlen(nmea_string);
    printf("Checksum Validation - String Length: %d\n", len);
    printf("Full NMEA String: %s", nmea_string);

    if (len < 7)
    {
        printf("Invalid: Too short (< 7 chars)\n");
        return false;
    }

    char *checksum_ptr = strchr(nmea_string, '*');
    if (!checksum_ptr)
    {
        printf("Invalid: No checksum marker (*) found\n");
        return false;
    }

    uint8_t calculated_checksum = 0;
    for (char *p = nmea_string + 1; p < checksum_ptr; p++)
    {
        calculated_checksum ^= *p;
    }

    char hex_checksum[3];
    snprintf(hex_checksum, sizeof(hex_checksum), "%02X", calculated_checksum);

    printf("Calculated Checksum: %s\n", hex_checksum);
    printf("Received Checksum: %s\n", checksum_ptr + 1);

    bool is_valid = strncmp(hex_checksum, checksum_ptr + 1, 2) == 0;
    printf("Checksum Validation: %s\n", is_valid ? "VALID" : "INVALID");

    return is_valid;
}

void convert_nmea_to_decimal(char *nmea_coord, float *decimal_coord)
{
    float degrees = atof(nmea_coord) / 100.0;
    int int_degrees = (int)degrees;
    float minutes = (degrees - int_degrees) * 100.0;

    *decimal_coord = int_degrees + (minutes / 60.0);
}

bool parse_nmea_gps(char *nmea_string, GPSData *gps_data)
{
    if (!validate_nmea_checksum(nmea_string))
    {
        printf("Invalid NMEA Checksum\n");
        return false;
    }

    // Ignore all NMEA strings that are not of type GPRMC
    if (strncmp(nmea_string, "$GPRMC", 6) != 0)
    {
        return false;
    }
    
    char *token;
    char *tokens[12] = {0};
    int token_count = 0;

    // Tokenize the string
    token = strtok(nmea_string, ",");
    while (token != NULL && token_count < 12)
    {
        tokens[token_count++] = token;

        token = strtok(NULL, ",");
    }

    // Check if we have enough tokens and data is valid
    if (token_count >= 12 && strcmp(tokens[2], "A") == 0)
    {
        if (strlen(tokens[3]) > 0)
        {
            convert_nmea_to_decimal(tokens[3], &gps_data->latitude);
            if (tokens[4][0] == 'S')
                gps_data->latitude = -gps_data->latitude;
        }

        if (strlen(tokens[5]) > 0)
        {
            convert_nmea_to_decimal(tokens[5], &gps_data->longitude);
            if (tokens[6][0] == 'W')
                gps_data->longitude = -gps_data->longitude;
        }

        gps_data->is_valid = true;
        return true;
    }
}

void process_gps_data(GPSData *gps_data)
{
    char nmea_buffer[MAX_NMEA_LENGTH];
    int chars_read = 0;

    while (uart_is_readable(UART_ID) && chars_read < MAX_NMEA_LENGTH - 1)
    {
        char received_char = uart_getc(UART_ID);
        nmea_buffer[chars_read] = received_char;

        // NMEA strings are terminated by a newline character
        if ((int)received_char == 10)
        {
            nmea_buffer[chars_read + 1] = '\0';

            if (parse_nmea_gps(nmea_buffer, gps_data))
            {
                printf("Valid GPS Data Received\n");
            }

            chars_read = 0;
            break;
        }
        else
        {
            chars_read++;
        }
    }
}