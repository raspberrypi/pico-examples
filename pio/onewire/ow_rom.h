// Generic ROM commands for 1-Wire devices
// https://www.analog.com/en/technical-articles/guide-to-1wire-communication.html
//
#define OW_READ_ROM         0x33
#define OW_MATCH_ROM        0x55
#define OW_SKIP_ROM         0xCC
#define OW_ALARM_SEARCH     0xEC
#define OW_SEARCH_ROM       0xF0