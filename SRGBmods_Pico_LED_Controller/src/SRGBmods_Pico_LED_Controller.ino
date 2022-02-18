#include "Adafruit_TinyUSB.h"
#include "FastLED.h"

#define DATA_PIN 3
#define NUM_LEDS 300
#define HARDWARE_LIGHTING 1
#define RETURN_TO_HWL 1
#define RETURN_TO_HWL_AFTER 20

uint8_t const desc_hid_report[] =
{
	TUD_HID_REPORT_DESC_GENERIC_INOUT(64)
};

Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, true);

CRGB leds[NUM_LEDS];
const int ledsPerPacket = 20;
bool updateLeds = false;
int packetCount = 0;
unsigned long lastPacketRcvd;

bool hardwareLighting = true;

void setup()
{
	usb_hid.setStringDescriptor("SRGBmods Pico LED Controller");
	USBDevice.setSerialDescriptor("A1B2C3D4E5F6A7B8C9D0");
	USBDevice.setManufacturerDescriptor("SRGBmods.net");
	usb_hid.setReportCallback(get_report_callback, set_report_callback);
	usb_hid.begin();
	while(!TinyUSBDevice.mounted()) delay(1);
	FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, 0, NUM_LEDS);
	FastLED.setBrightness(255);
}

void loop()
{
	if(updateLeds)
	{
		updateLeds = false;
		packetCount = 0;
		FastLED.show();
	}
	else if(HARDWARE_LIGHTING == 1 && hardwareLighting)
	{
		FastLED.setBrightness(75);
		playHardwareLighting();
	}
	else if(HARDWARE_LIGHTING == 1 && !hardwareLighting && RETURN_TO_HWL == 1)
	{
		unsigned long currentMillis = millis();
		if(currentMillis - lastPacketRcvd >= RETURN_TO_HWL_AFTER * 1000)
		{
			hardwareLighting = true;
		}
	}
}

uint16_t get_report_callback (uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) reqlen;
	return 0;
}

void set_report_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
	(void) report_id;
	(void) report_type;
	lastPacketRcvd = millis();
	if(buffer[1])
	{
		FastLED.setBrightness(255);
		hardwareLighting = false;
		packetCount = 0;
		FastLED.clear();
		return;
	}
	if(HARDWARE_LIGHTING == 1 && hardwareLighting)
	{
		hardwareLighting = false;
	}
	if(buffer[0] == packetCount+1)
	{
		int ledsSent = packetCount * ledsPerPacket;
		if(ledsSent < NUM_LEDS)
		{
			for(int ledIdx = 0; ledIdx < ledsPerPacket; ledIdx++)
			{
				if(ledsSent + 1 <= NUM_LEDS)
				{
					leds[ledIdx+((buffer[0]-1)*ledsPerPacket)].setRGB(buffer[4+(ledIdx*3)], buffer[4+(ledIdx*3)+1], buffer[4+(ledIdx*3)+2]);
					ledsSent++;
				}
			}
		}
		packetCount++;
	}
	else
	{
		packetCount = 0;
	}
	if(packetCount == buffer[2])
	{
		updateLeds = true;
	}
}

void playHardwareLighting()
{
	byte *color;
	uint16_t i, j;

	for(j = 0; j < 256*5; j++)
	{
		if(!hardwareLighting)
		{
			return;
		}
		for(i = 0; i < NUM_LEDS; i++)
		{
			if(!hardwareLighting)
			{
				return;
			}
			color = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
			leds[i].setRGB(*color, *(color+1), *(color+2));
		}
		FastLED.show();
		delay(20);
	}
}

byte * Wheel(byte WheelPos)
{
	static byte c[3];
 
	if(WheelPos < 85)
	{
		c[0]=WheelPos * 3;
		c[1]=255 - WheelPos * 3;
		c[2]=0;
	}
	else if(WheelPos < 170)
	{
		WheelPos -= 85;
		c[0]=255 - WheelPos * 3;
		c[1]=0;
		c[2]=WheelPos * 3;
	}
	else
	{
		WheelPos -= 170;
		c[0]=0;
		c[1]=WheelPos * 3;
		c[2]=255 - WheelPos * 3;
	}
	return c;
}