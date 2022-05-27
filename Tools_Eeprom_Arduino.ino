#include <EEPROM.h>

/*******************************************************************************
 * Tools_Eeprom-Arduino.ino
 * inspiration from https://github.com/yoppeh/eeprom-programmer
 * Arduino code for the internal eeprom. Note that this code is 
 * the ATmega328P and probably Arduino Mega). 
 *
 * Serial port settings:
 *  57600, 8n1, no flow control
 *
 * If you want to use other than 57600, change the SERIAL_BAUD constant below.
 *  
 * Terminal settings:
 *  turn off local echo
 *  Newline receive: LF
 *  Newline transmit: LF
 */


/* These are used in the x-modem protocol */
#define CHAR_SOH                0x01
#define CHAR_EOT                0x04
#define CHAR_ACK                0x06
#define CHAR_NAK                0x15
#define CHAR_ETB                0x17
#define CHAR_CAN                0x18
#define CHAR_ESC                0x1b
#define CHAR_C                  0x43

#define XMODEM_RETRIES          10
#define XMODEM_DELAY            2000
#define XMODEM_PACKET_MAX       128

/* Serial commands accepted by the programmer */
#define CMD_ERASE               'e'
#define CMD_FILL                'f'
#define CMD_HELP                'h'
#define CMD_READ                'r'
#define CMD_SIZE                's'
#define CMD_VERSION             'v'
#define CMD_XMODEM              'x'

/* Software version
 */
#define VERSION_MAJ             0
#define VERSION_MIN             0
#define VERSION_BLD             1

/* Change this to match whatever baud rate you want to use for the serial 
 * connection. 
 */
#define SERIAL_BAUD             57600

/* Set this to the size, in bytes of the EEPROM. In the case of the atmega328,
 * it's 1k. 
 */
#define DEFAULT_EEPROM_SIZE     1024
#define PAGE_SIZE               64

unsigned int eeprom_size = DEFAULT_EEPROM_SIZE;

/* Writes a single input byte at a specified address.
 */
int write_byte(int address, byte data) 
{
    EEPROM.update(address, data);
    //EEPROM.write(address, data);
    // byte was successfully written
    return 1;
}


/* Write a page of a given length (64 bytes max) at a given address.
 */
int write_page(int address, byte *data, byte len){
    while (len--){
        EEPROM.update(address++, *data++);
        //EEPROM.write(address++, *data++);
    }
    return 1;
}


/* Write an xmodem packet 1 EEPROM page at a time. 
 */
void write_packet(word address, byte *data, word len)
{
    word i;
    
    for (i = 0; i < len; i += PAGE_SIZE, data += PAGE_SIZE) {
        write_page(address + i, data, PAGE_SIZE);
    }
}


/* Very that the received xmodem packet matches what was
 * written to the EEPROM.
 */
int verify_packet(word address, byte *data, word len)
{
    word i;

    for (i = 0; i < len; i++, address++, data++) {
        if (read_byte(address) != *data) {
            return 0;
        }
    }
    return 1;
}


/* Erase the EEPROM by writing all 1s to it. We do it this way, since the
 * hardware isn't there to do a hardware erase. That would require putting
 * a 12 volt line in the circuit, and... why bother...
 */
int erase_eeprom(void)
{
    byte fill[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    word i;
    for (i = 0; i < eeprom_size; i += PAGE_SIZE) {
        if (i % 64 == 0) {
            Serial.print(".");
        }
        if (!write_page(i, fill, PAGE_SIZE)) {
            return 0;
        }
    }
    return 1;
}


/* Returns the byte at the given address in the EEPROM.
 */
byte read_byte(word address){
    byte data;
    
    data = EEPROM.read(address);
    return data;
}


/* Writes a pattern to the EEPROM. The pattern is address = address & 0xff. So,
 * essentially, every address is set to the value of its lower 8 bits. I guess
 * that's the long way to say, it repeats from 0 to 255 until the end of memory.
 */
int write_pattern(void)
{
    byte fill[PAGE_SIZE];
    word i, j;
    for (i = 0; i < eeprom_size; i += PAGE_SIZE) {
        for (j = 0; j < PAGE_SIZE; j++) {
            fill[j] = i + j;
        }
        if (i % 1024 == 0) {
            Serial.print(".");
        }
        if (!write_page(i, fill, PAGE_SIZE)) {
            return 0;
        }
    }
    return 1;
}


/* Reads the entire EEPROM. Prints the data out in the format: 
 * aaaa  xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx   ................
 * Where aaaa is the address in hex, xx is 16 bytes of data in hex and 
 * ............... is the ascii representation for each data byte, if it is 
 * printable.
 */
void read_eeprom(void)
{
    char s[6];
    word i, j;
    byte d;
    for (i = 0; i < eeprom_size; i += 16) {
        sprintf(s, "%04x ", i);
        Serial.print(s);
        for (j = 0; j < 16; j++) {
            sprintf(s, " %02x", read_byte(i + j));
            Serial.print(s);
        }
        Serial.print("  ");
        for (j = 0; j < 16; j++) {
            d = read_byte(i + j);
            sprintf(s, "%c", isprint(d) ? d : '.');
            Serial.print(s);
        }
        Serial.println("");
    }
}


/* Display terminal prompt
 */
void show_prompt(void)
{
    Serial.print("> ");
}


/* Display serial command help.
 */
void cmd_help(void)
{
    Serial.println("commands:");
    Serial.print(CMD_ERASE); Serial.println(F(" - erase the eeprom, fill with 0xFF"));
    Serial.print(CMD_FILL); Serial.println(F(" - fill eeprom with incrementing bytes"));
    Serial.print(CMD_HELP); Serial.println(F(" - help (this) text"));
    Serial.print(CMD_READ); Serial.println(F(" - read and dump eeprom contents"));
    Serial.print(CMD_SIZE); Serial.print(F(" - set the size of the eeprom (currently ")); Serial.print(eeprom_size ); Serial.print(F("bytes, "));
    if (eeprom_size == 1024) {
        Serial.print(F("ATMEGA328"));
    } else if (eeprom_size == 512) {
        Serial.print(F("ATMEGA8 or ATMEGA168"));
    } else if (eeprom_size == 4096) {
        Serial.print(F("ATMEGA1280 or ATMEGA2560"));
    } else {
        Serial.print(F("Unknown chip"));
    }
    Serial.println(")");
    Serial.print(CMD_VERSION); Serial.println(F(" - print software version info"));
    Serial.print(CMD_XMODEM); Serial.println(F(" - xmodem transfer binary file to eeprom"));
}

 
/* Initial board setup.
 */
void setup() 
{
    Serial.begin(SERIAL_BAUD);
    cmd_help();
    show_prompt();
}


/* Flush the serial input buffer
 */
void flush_serial_input(void)
{
    while (Serial.available()) {
        Serial.read();
    }
}


/* Sends the xmodem abort sequence out the serial port.
 */
void abort_xmodem(char *s)
{
    int i;

    for (i = 0; i < 8; i++) {
        Serial.write(CHAR_CAN);
        Serial.flush();
        delay(1000);
    }
    Serial.print(F("transfer aborted: "));
    Serial.println(s);
    flush_serial_input();
}


/* Read the next available byte from the serial port.
 */
int read_serial(void)
{
    while (!Serial.available());
    return Serial.read();
}


/* Erase the EEPROM.
 */
void cmd_erase(void)
{
    Serial.print(F("erasing eeprom "));
    if (!erase_eeprom()) {
        Serial.println(F(" error"));
        return;
    } else {
        Serial.println(F(" ok"));
    }
}


/* Fill the EEPROM with a sequence.
 */
void cmd_fill(void)
{    
    Serial.print(F("filling eeprom "));
    if (!write_pattern()) {
        Serial.println(F(" error"));
        return;
    } else {
        Serial.println(F(" ok"));
    }
}

/* Read the entire EEPROM.
 */
void cmd_read(void)
{
    Serial.println(F("reading eeprom:"));
    read_eeprom();
    Serial.println(F("all addresses read"));
}


/* Display size command help 
*/
void cmd_size_help(void)
{
    Serial.println(F("select new eeprom size:"));
    Serial.println(F("1 - 1O24 (ATMEGA328)"));
    Serial.println(F("2 - 512 (ATMEGA8 or ATMEGA168)"));
    Serial.println(F("3 - 4096 (ATMEGA1280 or ATMEGA2560)"));
    Serial.println(F("any other key to cancel"));
}


/* Set the EEPROM size.
 */
 void cmd_size(void)
 {
    int ch;
    cmd_size_help();
    show_prompt();
    ch = read_serial();
    if (ch == '1') {
        eeprom_size = 1024;
    } else if (ch == '2') {
        eeprom_size = 512;
    } else if (ch == '3') {
        eeprom_size = 4096;    
    } else {
        Serial.println("");
        return;
    }
    Serial.print(F("eeprom size set to ")); Serial.print(eeprom_size ); Serial.print("bytes (");
    if (eeprom_size == 1024) {
        Serial.print(F("ATMEGA328"));
    } else if (eeprom_size == 512) {
        Serial.print(F("ATMEGA8 or ATMEGA168"));
    } else if (eeprom_size == 4096) {
        Serial.print(F("ATMEGA1280 or ATMEGA2560"));
    }
    Serial.println(")");
    Serial.println("");
    cmd_help();
 }

 
/* Print software info.
 */
void cmd_version(void)
{
    char version[12];
    Serial.println(F("software version info:"));
    sprintf(version, "%02i\.%02i.%02i", VERSION_MAJ, VERSION_MIN, VERSION_BLD);
    Serial.print(F("version: "));
    Serial.println(version);
}


/* Read the next available byte from the serial port.
 */
int xmodem_read_serial(void)
{
    unsigned long start = millis();
    while (!Serial.available()) {
        if (millis() - start >= XMODEM_DELAY) {
            return -1;
        }
    }
    return Serial.read();
}


/* Print packet data
 */
 void print_packet_data(byte *packet) {
    char s[3];
    for (int i = 1; i <= XMODEM_PACKET_MAX; i++) {
        sprintf(s, "%02x ", packet[i - 1]);
        Serial.print(s);
        if (i % 16 == 0) {
            Serial.println("");
        }
    }
 }


/* Begin xmodem transfer to the EEPROM.
 */
void cmd_xmodem(void)
{
    unsigned long int start;
    word addr = 0;
    int retry;
    int ch;
    byte pkt_len;
    byte prev_pkt_num = 0;
    byte pkt_num = 1;
    byte remote_pkt_num;
    byte remote_not_pkt_num;
    byte remote_chk;
    byte chk;
    byte response_char = CHAR_NAK;
    byte error_count = 0;
    static byte packet[XMODEM_PACKET_MAX];
    char s[12];
    char *last_error;
    
    Serial.println(F("Starting xmodem"));
    Serial.println(F("Start transfer now or press 'ESC' key to abort"));
    
    while (1) {
        // packet start
        retry = XMODEM_RETRIES;
        while (retry--) {
            if (response_char) {
                Serial.write(response_char);
                Serial.flush();
                ch = xmodem_read_serial();
                switch (ch) {
                    case CHAR_SOH:
                        goto begin;
                    case CHAR_ESC:
                        Serial.println(F("Aborted by user"));
                        return;
                    case CHAR_CAN:
                        ch = xmodem_read_serial();
                        if (ch == CHAR_CAN) {
                            flush_serial_input();
                        }
                        Serial.write(CHAR_ACK);
                        Serial.flush();
                        Serial.println(F("Remote cancelled transfer"));
                        return;
                    case CHAR_EOT:
                        Serial.write(CHAR_NAK);
                        Serial.flush();
                        delay(1000);
                        ch = xmodem_read_serial();
                        if (ch == CHAR_EOT) {
                            Serial.write(CHAR_ACK);
                            Serial.flush();
                            delay(1000);
                            flush_serial_input();
                            Serial.println(F("transfer complete"));
                            return;
                        }
                        ch = 0;
                        break;
                }
            }
        }
begin:
        if (ch != CHAR_SOH) {
            response_char = CHAR_NAK;
            flush_serial_input();
            continue;
        }
        response_char = 0;
        // packet number
        ch = xmodem_read_serial();
        if (ch == -1) {
            last_error = "timeout waiting for remote_pkt_num";
            error_count++;
            goto skip;
        }
        remote_pkt_num = ch;
        // complement packet number
        ch = xmodem_read_serial();
        if (ch == -1) {
            last_error = "timeout waiting for remote_not_pkt_num";
            error_count++;
            goto skip;
        }
        remote_not_pkt_num = ch;
        // packet
        pkt_len = 0;
        chk = 0;
        while (pkt_len < XMODEM_PACKET_MAX) {
            start = millis();
            while (!Serial.available()) {
                if (millis() - start >= XMODEM_DELAY) {
                    last_error = "timeout waiting for packet data";
                    error_count++;
                    goto skip;
                }
            }
            ch = Serial.read();
            if (ch == -1) {
                last_error = "timeout reading packet data";
                error_count++;
                goto skip;
            }
            packet[pkt_len++] = ch;
            chk += (byte)ch;
        }
        ch = xmodem_read_serial();
        if (ch == -1) {
            last_error = "timeout waiting for checksum";
            error_count++;
            goto skip;
        }
        remote_chk = ch;
        // make sure data < EEPROM size
        if (addr == eeprom_size) {
            abort_xmodem("image is too large for eeprom");
            return;
        }
        // check complement error
        if (remote_not_pkt_num + pkt_num != 255) {
            Serial.write(CHAR_NAK);
            Serial.flush();
            last_error = "check complement error";
            error_count++;
            goto skip;
        }
        // check duplicate packet
        if (remote_pkt_num == prev_pkt_num) {
            Serial.write(CHAR_ACK);
            Serial.flush();
            last_error = "check duplicate packet";
            error_count = 0;
            goto skip;
        // check out of sequence
        } else if (remote_pkt_num != pkt_num) {
            last_error = "out of sequence";
            goto skip;
        }
        // checksum test
        if (remote_chk != chk) {
            Serial.write(CHAR_NAK);
            Serial.flush();
            last_error = "checksum error";
            error_count++;
            goto skip;
        } 
        // no errors, send an ACK and write the pages to the EEPROM 
        Serial.write(CHAR_ACK);
        Serial.flush();
        write_packet(addr, packet, XMODEM_PACKET_MAX);
        if (!verify_packet(addr, packet, XMODEM_PACKET_MAX)) {
            abort_xmodem("verify failed writing packet, aborting");
            print_packet_data(packet);
            return;
        }
        addr += XMODEM_PACKET_MAX;
        prev_pkt_num = pkt_num;
        pkt_num = remote_pkt_num + 1;
        error_count = 0;
skip:   
        if (error_count == XMODEM_RETRIES) {
            abort_xmodem(last_error);
            Serial.print(F("prev_pkt_num = ")); Serial.println(prev_pkt_num);
            Serial.print(F("pkt_num = ")); Serial.println(pkt_num);
            sprintf(s, " (%02x)", remote_pkt_num);
            Serial.print(F("remote_pkt_num = ")); Serial.print(remote_pkt_num); Serial.println(s);
            sprintf(s, " (%02x)", remote_not_pkt_num);
            Serial.print(F("remote_not_pkt_num = ")); Serial.print(remote_not_pkt_num); Serial.println(s);
            sprintf(s, " (%02x)", remote_chk);
            Serial.print(F("remote_chk = ")); Serial.print(remote_chk); Serial.println(s);
            Serial.print(F("chk = ")); Serial.println(chk);
            print_packet_data(packet);
            return;
        }
    }
}


/* Unknown command.
 */
void cmd_unknown(char cmd)
{
    Serial.print(F("unknown command: '")); 
    Serial.print(cmd); 
    Serial.println(F("'"));
    Serial.println(F("use 'h' for help"));
}


/* Process the specified command.
 */
void process_cmd(char cmd)
{
    cmd = tolower(cmd);
    Serial.println(cmd);
    
    switch (cmd) {
        case CMD_ERASE:
            cmd_erase();
            break;
        case CMD_FILL:
            cmd_fill();
            break;
        case CMD_HELP:
            cmd_help();
            break;
        case CMD_READ:
            cmd_read();
            break;
        case CMD_SIZE:
            cmd_size();
            break;
        case CMD_VERSION:
            cmd_version();
            break;
        case CMD_XMODEM:
            cmd_xmodem();
            break;
        default:
            cmd_unknown(cmd);
            break;
    }
}


/* Process commands from the serial port, output results.
 */
void loop() 
{
    process_cmd(read_serial());
    show_prompt();            
}
