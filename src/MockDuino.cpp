#include "MockDuino.h"
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FireUtils.h"
#include "Thread.h"

using namespace std;
using namespace mockduino;

MockSerial mockSerial;
MockDuino arduino;
vector<uint8_t> serialbytes;
int16_t eeprom_data[EEPROM_END];


void MockSerial::clear() {
    serialbytes.clear();
    serialout.clear();
    serialline.clear();
}

void MockSerial::push(uint8_t value) {
    serialbytes.push_back(value);
}

void MockSerial::push(int16_t value) {
    //uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    serialbytes.push_back((uint8_t)(value & 0xff));
}

void MockSerial::push(int32_t value) {
    //uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back((uint8_t)((value >> 24) & 0xff));
    serialbytes.push_back((uint8_t)((value >> 16) & 0xff));
    serialbytes.push_back((uint8_t)((value >> 8) & 0xff));
    serialbytes.push_back((uint8_t)(value & 0xff));
}

void MockSerial::push(float value) {
    uint8_t *pvalue = (uint8_t *) &value;
    serialbytes.push_back(pvalue[0]);
    serialbytes.push_back(pvalue[1]);
    serialbytes.push_back(pvalue[2]);
    serialbytes.push_back(pvalue[3]);
}

void MockSerial::push(string value) {
    push(value.c_str());
}

void MockSerial::push(const char * value) {
    for (const char *s = value; *s; s++) {
        serialbytes.push_back(*s);
    }
}

string MockSerial::output() {
    string result = serialout;
    serialout = "";
    return result;
}

int MockSerial::available() {
    return serialbytes.size();
}

void MockSerial::begin(long speed) {
}

uint8_t MockSerial::read() {
    if (serialbytes.size() < 1) {
        return 0;
    }
    uint8_t c = serialbytes[0];
    serialbytes.erase(serialbytes.begin());
    return c;
}

size_t MockSerial::write(uint8_t value) {
    serialout.append(1, (char) value);
	if (value == '\r') {
		serialline.append(1, '\\');
		serialline.append(1, 'r');
		// skip
	} else if (value == '\n') {
        cout << "mockSerial	: \"" << serialline << "\"" << endl;
        serialline = "";
    } else {
        serialline.append(1, (char)value);
    }
    return 1;
}

void MockSerial::print(const char value) {
	char buf[2];
	buf[0] = value;
	buf[1] = 0;
    serialout.append(buf);
    serialline.append(buf);
}

void MockSerial::print(const char *value) {
    serialout.append(value);
    serialline.append(value);
}

void MockSerial::print(int value, int format) {
    stringstream buf;
    switch (format) {
    case HEX:
        buf << std::hex << value;
        buf << std::dec;
        break;
    default:
    case DEC:
        buf << value;
        break;
    }
    string bufVal = buf.str();
    serialline.append(bufVal);
    serialout.append(bufVal);
}

Print& mockduino::get_Print() {
	return mockSerial;
}

int16_t mockduino::serial_read() {
	return mockSerial.read();
}
int16_t mockduino::serial_available() {
	return mockSerial.available();
}
void mockduino::serial_begin(int32_t baud) {
	mockSerial.begin(baud);
}
void mockduino::serial_print(const char *value) {
	mockSerial.print(value);
}
void mockduino::serial_print(const char value) {
	mockSerial.print(value);
}
void mockduino::serial_print(int16_t value, int16_t format) {
	mockSerial.print(value, format);
}

///////////////////// MockDuino ///////////////////

int __heap_start, *__brkval;

MockDuino::MockDuino() {
    clear();
}

int16_t& MockDuino::MEM(int addr) {
    ASSERT(0 <= addr && addr < MOCKDUINO_MEM);
    return mem[addr];
}

void MockDuino::clear() {
    //int novalue = 0xfe;
    mockSerial.output();	// discard
    for (int16_t i = 0; i < MOCKDUINO_PINS; i++) {
        pin[i] = NOVALUE;
        _pinMode[i] = NOVALUE;
    }
    for (int16_t i = 0; i < MOCKDUINO_MEM; i++) {
        mem[i] = NOVALUE;
    }
	for (int16_t i=0; i<EEPROM_END; i++) {
		eeprom_data[i] = NOVALUE;
	}
    memset(pinPulses, 0, sizeof(pinPulses));
    usDelay = 0;
    ADCSRA = 0;	// ADC control and status register A (disabled)
    TCNT1 = 0; 	// Timer/Counter1
    CLKPR = 0;	// Clock prescale register
	sei(); // enable interrupts
}

int32_t MockDuino::pulses(int16_t pin) {
    ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
    return pinPulses[pin];
}

void MockDuino::dump() {
    for (int i = 0; i < MOCKDUINO_MEM; i += 16) {
        int dead = true;
        for (int j = 0; j < 16; j++) {
            if (mem[i + j] != NOVALUE) {
                dead = false;
                break;
            }
        }
        if (!dead) {
            cout << "MEM" << setfill('0') << setw(3) << i << "\t: ";
            for (int j = 0; j < 16; j++) {
                cout << setfill('0') << setw(4) << std::hex << mem[i + j] << " ";
                cout << std::dec;
                if (j % 4 == 3) {
                    cout << "| ";
                }
            }
            cout << endl;
        }
    }
}

void MockDuino::timer64us(int increment) {
    if (TIMER_ENABLED) {
        TCNT1 += increment;
    }
}

void MockDuino::delay500ns() {
}

void mockduino::delayMicroseconds(uint16_t usDelay) {
    arduino.usDelay += usDelay;
}

void mockduino::analogWrite(int16_t pin, int16_t value) {
    ASSERT(A0 <= pin && pin < MOCKDUINO_PINS);
	ASSERT(0 <= value && value <= 255);
	arduino.pin[pin] = value;
}

int16_t mockduino::analogRead(int16_t pin) {
    ASSERT(A0 <= pin && pin < MOCKDUINO_PINS);
    ASSERT(arduino.pin[pin] != NOVALUE);
	return arduino.pin[pin];
}

void mockduino::digitalWrite(int16_t pin, int16_t value) {
    ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
    ASSERTEQUAL(OUTPUT, arduino.getPinMode(pin));
    if (arduino.pin[pin] != value) {
        if (value == 0) {
			//TESTCOUT2("digitalWrite LOW pin:", pin, " value:", value);
            arduino.pinPulses[pin]++;
		} else {
			//TESTCOUT2("digitalWrite HIGH pin:", pin, " value:", value);
        }
        arduino.pin[pin] = value ? HIGH : LOW;
	} else {
		//TESTCOUT2("digitalWrite (ignored) pin:", pin, " value:", value);
    }
}

int16_t mockduino::digitalRead(int16_t pin) {
    ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
	if (arduino._pinMode[pin] == NOVALUE) {
		cerr << "digitalRead(" << pin << ") pinMode:NOVALUE" << endl;
	}
    ASSERT(arduino._pinMode[pin] != NOVALUE);
	if (arduino.pin[pin] == NOVALUE) {
		cerr << "digitalRead(" << pin << ") NOVALUE" << endl;
	}
    ASSERT(arduino.pin[pin] != NOVALUE);
    return arduino.pin[pin];
}

void mockduino::pinMode(int16_t pin, int16_t inout) {
    ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
    arduino._pinMode[pin] = inout;
	if (inout == INPUT_PULLUP) {
		if (arduino.pin[pin] == NOVALUE) {
			arduino.pin[pin] = HIGH;
			TESTCOUT2("pinMode INPUT_PULLUP pin:", pin, " value:", arduino.pin[pin]);
		} else {
			//TESTCOUT3("pinMode pin:", pin, " mode:", inout, " value:", arduino.pin[pin]);
		}
	}
}

int16_t MockDuino::getPinMode(int16_t pin) {
    ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
    return arduino._pinMode[pin];
}

int16_t MockDuino::getPin(int16_t pin) {
    ASSERT(pin != NOPIN);
    return arduino.pin[pin];
}

void MockDuino::setPin(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
		//TESTCOUT2("setPin pin:", pin, " value:", value);
		ASSERT(0 <= pin && pin < MOCKDUINO_PINS);
        arduino.pin[pin] = value;
    }
}

void MockDuino::setPinMode(int16_t pin, int16_t value) {
    if (pin != NOPIN) {
        arduino._pinMode[pin] = value;
    }
}

void mockduino::delay(int ms) {
    arduino.timer64us(MS_TICKS(ms));
}

/////////////// avr/eeprom.h /////////////////

uint8_t mockduino::eeprom_read_byte(uint8_t *addr) {
    if ((size_t) addr < 0 || EEPROM_END <= (size_t) addr) {
        return 255;
    }
    return eeprom_data[(size_t) addr];
}

void mockduino::eeprom_write_byte(uint8_t *addr, uint8_t value) {
    if (0 <= (size_t) addr && (size_t) addr < EEPROM_END) {
        eeprom_data[(size_t) addr] = value;
    }
}

string eeprom_read_string(uint8_t *addr) {
	string result;
	for (size_t i=0; i+(size_t)addr<EEPROM_END; i++) {
		uint8_t b = mockduino::eeprom_read_byte(i+addr);
		if (!b) { break; }
		result += (char) b;
	}
	return result;
}	
