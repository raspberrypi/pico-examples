#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"           //to read ADC for soil moisture sensor, connected to pin 36 (3.3V), pin 33 (GND/AGND), pin 34 (GPIO28/ADC2)
#include "lwip/apps/http_client.h"  //http client
#include "pico/cyw43_arch/arch_threadsafe_background.h"  //to get access to void cyw43_thread_enter(void);  and void cyw43_thread_exit(void);

#ifndef CYW43_WL_GPIO2
#define CYW43_WL_GPIO2 2  //this is not defined in pico_w.h
#endif

const float conversion_factor = 3.3f / (1 << 12); //ADC_VREF=3.3, so this is the ADC resolution. 
const char ssid[] = "myWifiNetwork";
const char pass[] = "myWifiPassword";
const uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

//Http client variables
bool httpBusy = false;
char httpBuff[1000]; //if not big enough to read the result or headers, pico will hang.
httpc_connection_t settings;
ip_addr_t ip;
char apicall[64];


//handler for HTTP client result
void httpresult(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    printf("[1] local result=%d  http result=%d\n", httpc_result, srv_res);
}

//handler for HTTP client headers
err_t headers(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    pbuf_copy_partial(hdr, httpBuff, hdr->tot_len, 0);
    printf("[1] content length=%d,  header length %d, headers: %s\n", content_len, hdr_len, httpBuff);
    //free memory
	pbuf_free(hdr); 
    return ERR_OK;
}

//handler for HTTP client body
err_t body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    pbuf_copy_partial(p, httpBuff, p->tot_len, 0);
    printf("[1] body: %s", httpBuff);
    httpBusy = false;
    //free memory 
    pbuf_free(p);
    return ERR_OK;
}

//Set GPIO29 back to settings for wifi usage (shared pin with ADC3, so settings are changed when activating ADC3)
void SetGPIO29WifiStatus()
{
    gpio_set_function(29, GPIO_FUNC_PIO1); //7
    gpio_pull_down(29);
    gpio_set_slew_rate(29, GPIO_SLEW_RATE_FAST); //1
    gpio_set_drive_strength(29, GPIO_DRIVE_STRENGTH_12MA); //3
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(28);

    //Initialize wifi
    if (cyw43_arch_init()) {
        printf("WiFi init failed.\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    //connect to access point
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, auth, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }

    //set http client handlers
    settings.result_fn = httpresult;
    settings.headers_done_fn = headers;   
    //define IP address that hosts the web api to send the data
    IP4_ADDR(&ip, 192, 168, 0, 1);
    int ledStatus = 0;
    float vsys=0;

    while(true) {        
        //flip onboard led
        ledStatus = 1 - ledStatus;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledStatus);

        //Select ADC2 to read moisture sensor
        adc_select_input(2);
        uint16_t result = adc_read();
        float voltage = result * conversion_factor;
        printf("ADC2 value: 0x%03x, %i, voltage: %3.1f V\n", result, result, voltage);

        //make sure there is not HTTP request in progress or other wifi activity when we want to check the battery status    
        if (!httpBusy){            
            httpBusy = true;
            
            //prevent wifi background processing so we can use the shared pins
            cyw43_thread_enter();   //or macro: CYW43_THREAD_ENTER

            //WL_GPIO2, IP VBUS sense - high if VBUS is present, else low
            uint vbus = cyw43_arch_gpio_get(CYW43_WL_GPIO2); 

            //we could decide to only check the voltage if not powered by USB port/VBUS and perhaps not check it every cycle.
            //if (vbus==0)            
            //{
            //initialize and select ADC3/GPIO29                
            adc_gpio_init(29);
            adc_select_input(3);

            //Read ADC3, result is 1/3 of VSYS, so we still need to multiply the conversion factor with 3 to get the input voltage
            uint16_t adc3 = adc_read();
            vsys = adc3 * conversion_factor * 3; 
            printf("ADC3 value: 0x%03x, %i, voltage: %f V\n", adc3, adc3, vsys);
            //set GPIO29 back to the settings for wifi, otherwise cyw43 will not work correctly
            SetGPIO29WifiStatus();
            //}
            //else{                
                //USB/VSYS powered, do nothing (voltage should be somewhere around 5V)
            //}
            //exit to allow wifi communication again
            cyw43_thread_exit();    //or macro: CYW43_THREAD_EXIT

            sprintf(apicall, "/api/echo?m=%3.3f&v=%3.3f&b=%i", voltage, vsys, vbus);
            printf("Calling API: %s\n", apicall);
            err_t err = httpc_get_file(
                &ip,        //IP address hosting the API
                5131,       //port
                apicall,    //api call
                &settings,  //handlers for result/headers
                body,       //handler for body
                NULL,
                NULL
            );  
            printf("[1] http status %d \n", err);
        }
        else{
            printf("[1] http client is busy...\n");
        }

#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
       sleep_ms(5000);        
#endif        
    }

    cyw43_arch_deinit();
    return 0;
}
