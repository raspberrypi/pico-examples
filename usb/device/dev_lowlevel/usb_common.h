/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _USB_COMMON_H
#define _USB_COMMON_H

#include "pico/types.h"
#include "hardware/structs/usb.h"

// bmRequestType bit definitions
#define USB_REQ_TYPE_STANDARD 0x00u
#define USB_REQ_TYPE_TYPE_MASK 0x60u
#define USB_REQ_TYPE_TYPE_CLASS 0x20u
#define USB_REQ_TYPE_TYPE_VENDOR 0x40u

#define USB_REQ_TYPE_RECIPIENT_MASK 0x1fu
#define USB_REQ_TYPE_RECIPIENT_DEVICE 0x00u
#define USB_REQ_TYPE_RECIPIENT_INTERFACE 0x01u
#define USB_REQ_TYPE_RECIPIENT_ENDPOINT 0x02u

#define USB_DIR_OUT 0x00u
#define USB_DIR_IN 0x80u

#define USB_TRANSFER_TYPE_CONTROL 0x0
#define USB_TRANSFER_TYPE_ISOCHRONOUS 0x1
#define USB_TRANSFER_TYPE_BULK 0x2
#define USB_TRANSFER_TYPE_INTERRUPT 0x3
#define USB_TRANSFER_TYPE_BITS 0x3

// Descriptor types
#define USB_DT_DEVICE 0x01
#define USB_DT_CONFIG 0x02
#define USB_DT_STRING 0x03
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05

#define USB_REQUEST_GET_STATUS 0x0
#define USB_REQUEST_CLEAR_FEATURE 0x01
#define USB_REQUEST_SET_FEATURE 0x03
#define USB_REQUEST_SET_ADDRESS 0x05
#define USB_REQUEST_GET_DESCRIPTOR 0x06
#define USB_REQUEST_SET_DESCRIPTOR 0x07
#define USB_REQUEST_GET_CONFIGURATION 0x08
#define USB_REQUEST_SET_CONFIGURATION 0x09
#define USB_REQUEST_GET_INTERFACE 0x0a
#define USB_REQUEST_SET_INTERFACE 0x0b
#define USB_REQUEST_SYNC_FRAME 0x0c

#define USB_REQUEST_MSC_GET_MAX_LUN 0xfe
#define USB_REQUEST_MSC_RESET 0xff

#define USB_FEAT_ENDPOINT_HALT            0x00
#define USB_FEAT_DEVICE_REMOTE_WAKEUP   0x01
#define USB_FEAT_TEST_MODE                0x02

#define USB_DESCRIPTOR_TYPE_ENDPOINT 0x05

struct usb_setup_packet {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __packed;

struct usb_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
};

struct usb_device_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __packed;

struct usb_configuration_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __packed;

struct usb_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __packed;

struct usb_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __packed;

struct usb_endpoint_descriptor_long {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
    uint8_t bRefresh;
    uint8_t bSyncAddr;
} __attribute__((packed));

#endif