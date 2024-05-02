// zerror.h

#ifndef ZERROR_H
#define ZERROR_H

#include "ssd1306.h" // Ensure this includes the definition of ssd1306_t

extern ssd1306_t disp;

// Updated function declaration to match the revised function that accepts three strings
extern void displayError(const char* line1, const char* line2, const char* line3);

#endif // ZERROR_H
