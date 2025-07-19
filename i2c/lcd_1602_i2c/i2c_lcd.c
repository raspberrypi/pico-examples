/**
* Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "i2c_lcd.h"

#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#define INSTRUCTION_FUNCTION_SET (1 << 5)
#define FUNCTION_SET_DATALENGTH_8BIT (1 << 4) // 4-bit if not set
#define FUNCTION_SET_USE_2_LINES (1 << 3) // 1 line if not set
#define FUNCTION_SET_FONT_5x10 (1 << 2) // 5x7 if not set

#define INSTRUCTION_DISPLAY_CTRL (1 << 3)
#define DISPLAY_CTRL_DISPLAY_ON (1 << 2)
#define DISPLAY_CTRL_CURSOR_ON (1 << 1)
#define DISPLAY_CTRL_CURSOR_BLINK (1 << 0)

#define INSTRUCTION_CLEAR_DISPLAY (1 << 0)

#define INSTRUCTION_ENTRY_MODE_SET (1 << 2)
#define ENTRY_MODE_INCREMENT (1 << 1) // decrement if not set
#define ENTRY_MODE_SHIFT (1 << 0) // don't shift if not set

#define INSTRUCTION_GO_HOME (1 << 1)

#define INSTRUCTION_SET_RAM_ADDR (1 << 7)

#define N_ROWS 2
#define N_COLS 16
#define LINEBUF_LEN (N_ROWS * N_COLS + 1)

static const uint8_t ROW_OFFSETS[] = { 0x00, 0x40, 0x14, 0x54 }; // in display memory

typedef union
{
    uint8_t dat;
    struct
    {
        bool RS        : 1;     // bit 0
        bool RW        : 1;     // bit 1
        bool EN        : 1;     // bit 2
        bool BACKLIGHT : 1;     // bit 3
        bool D4        : 1;     // bit 4
        bool D5        : 1;     // bit 5
        bool D6        : 1;     // bit 6
        bool D7        : 1;     // bit 7
    };
} PortExpanderData_t;

struct i2c_lcd
{
    PortExpanderData_t portExpanderDat;
    bool cursor;
    bool cursorBlink;
    bool displayEnabled;
    i2c_inst_t* i2cHandle;
    uint8_t address;
};

static void writeExpanderData(i2c_lcd_handle inst, bool clockDisplayController)
{
    if (clockDisplayController)
    {
        // Assert enable signal
        inst->portExpanderDat.EN = 1;

        // Send to hardware
        i2c_write_blocking(inst->i2cHandle, inst->address, &inst->portExpanderDat.dat, 1, false);
        sleep_us(1);

        // De-assert enable
        inst->portExpanderDat.EN = 0;

        // Send to hardware
        i2c_write_blocking(inst->i2cHandle, inst->address, &inst->portExpanderDat.dat, 1, false);
        sleep_us(50);
    }
    else
    {
        i2c_write_blocking(inst->i2cHandle, inst->address, &inst->portExpanderDat.dat, 1, false);
    }
}

static void write4bits(i2c_lcd_handle inst, uint8_t bits)
{
    // Load in the bits
    inst->portExpanderDat.D4 = (bits & (1 << 0)) != 0;
    inst->portExpanderDat.D5 = (bits & (1 << 1)) != 0;
    inst->portExpanderDat.D6 = (bits & (1 << 2)) != 0;
    inst->portExpanderDat.D7 = (bits & (1 << 3)) != 0;

    writeExpanderData(inst, true);
}

static void writeByte(i2c_lcd_handle inst, uint8_t data, bool dstControlReg)
{
    inst->portExpanderDat.RS = !dstControlReg;

    // Write more significant nibble first
    write4bits(inst, data >> 4);

    // Then write less significant nibble
    write4bits(inst, data);
}

static void executeInstruction(i2c_lcd_handle inst, uint8_t instruction, uint8_t flags)
{
    writeByte(inst, instruction | flags, true);

    switch (instruction)
    {
        case INSTRUCTION_FUNCTION_SET:
        case INSTRUCTION_DISPLAY_CTRL:
        case INSTRUCTION_ENTRY_MODE_SET:
        case INSTRUCTION_SET_RAM_ADDR:
            sleep_us(53);
            break;

        case INSTRUCTION_CLEAR_DISPLAY:
            sleep_us(3000);
            break;

        case INSTRUCTION_GO_HOME:
            sleep_us(2000);
            break;
    }
}

// https://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
i2c_lcd_handle i2c_lcd_init(i2c_inst_t* handle, uint8_t address)
{
    i2c_lcd_handle inst = calloc(sizeof(struct i2c_lcd), 1);
    inst->i2cHandle = handle;
    inst->address = address;

    // Special case of function set
    write4bits(inst, 0b0011);
    sleep_us(4100);
    write4bits(inst, 0b0011);
    sleep_us(100);
    write4bits(inst, 0b0011);
    sleep_us(100);

    // Function set interface to 4 bit mode
    write4bits(inst, 0b0010);
    sleep_us(100);

    executeInstruction(inst, INSTRUCTION_FUNCTION_SET, FUNCTION_SET_USE_2_LINES);
    executeInstruction(inst, INSTRUCTION_ENTRY_MODE_SET, ENTRY_MODE_INCREMENT);

    i2c_lcd_clear(inst);
    i2c_lcd_setDisplayVisible(inst, true);
    i2c_lcd_setCursorLocation(inst, 0,0);

    return inst;
}

void i2c_lcd_writeChar(i2c_lcd_handle inst, char c)
{
    writeByte(inst, c, false);
}

void i2c_lcd_writeString(i2c_lcd_handle inst, char* string)
{
    for (int i = 0; i < strlen(string); i++)
    {
        i2c_lcd_writeChar(inst, string[i]);
    }
}

void i2c_lcd_writeStringf(i2c_lcd_handle inst, const char* __restrict format, ...)
{
    va_list args;
    va_start(args, format);

    char linebuf[LINEBUF_LEN];
    vsnprintf(linebuf, LINEBUF_LEN, format, args);
    i2c_lcd_writeString(inst, linebuf);

    va_end(args);
}

void i2c_lcd_setCursorLocation(i2c_lcd_handle inst, uint8_t x, uint8_t y)
{
    // Bounds check
    if (x < N_COLS && y <= N_ROWS)
    {
        executeInstruction(inst, INSTRUCTION_SET_RAM_ADDR, x + ROW_OFFSETS[y]);
    }
}

void i2c_lcd_setCursorLine(i2c_lcd_handle inst, uint8_t line)
{
    // Bounds check
    if (line <= N_ROWS)
    {
        executeInstruction(inst, INSTRUCTION_SET_RAM_ADDR, ROW_OFFSETS[line]);
    }
}

void i2c_lcd_writeLines(i2c_lcd_handle inst, char* line1, char* line2)
{
    char linebuf1[LINEBUF_LEN];
    char linebuf2[LINEBUF_LEN];

    snprintf(linebuf1, LINEBUF_LEN, "%-16s", line1);
    snprintf(linebuf2, LINEBUF_LEN, "%-16s", line2);

    i2c_lcd_setCursorLocation(inst, 0,0);
    i2c_lcd_writeString(inst, linebuf1);
    i2c_lcd_setCursorLocation(inst, 0,1);
    i2c_lcd_writeString(inst, linebuf2);
}

void i2c_lcd_clear(i2c_lcd_handle inst)
{
    executeInstruction(inst, INSTRUCTION_CLEAR_DISPLAY, 0);
}

void i2c_lcd_setBacklightEnabled(i2c_lcd_handle inst, bool en)
{
    inst->portExpanderDat.BACKLIGHT = en;
    writeExpanderData(inst, false);
}

static void updateDisplayConfiguration(i2c_lcd_handle inst)
{
    uint8_t flags = 0;
    if (inst->cursor) flags         |= DISPLAY_CTRL_CURSOR_ON;
    if (inst->cursorBlink) flags    |= DISPLAY_CTRL_CURSOR_BLINK;
    if (inst->displayEnabled) flags |= DISPLAY_CTRL_DISPLAY_ON;

    executeInstruction(inst, INSTRUCTION_DISPLAY_CTRL, flags);
}

void i2c_lcd_setDisplayVisible(i2c_lcd_handle inst, bool en)
{
    inst->displayEnabled = en;
    updateDisplayConfiguration(inst);
}

void i2c_lcd_setCursorEnabled(i2c_lcd_handle inst, bool cusror, bool blink)
{
    inst->cursor = cusror;
    inst->cursorBlink = blink;
    updateDisplayConfiguration(inst);
}
