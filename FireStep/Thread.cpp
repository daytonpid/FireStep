#include "Arduino.h"
#include "Thread.h"

using namespace firestep;

namespace firestep {
	ThreadClock 	threadClock;
	ThreadRunner 	threadRunner;
	struct Thread *	pThreadList;
	int 			nThreads;
	int32_t 		nLoops;
	int32_t 		nTardies;

};


void Thread::setup() {
    bool active = false;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (pThread == this) {
            active = true;
            break;
        }
    }

    if (!active) {
        pNext = pThreadList;
        pThreadList = this;
        if (id == 0) {
            id = 'a' + nThreads;
        }
		nThreads++;
        if (nThreads >= MAX_ThreadS) {
            Error("SC", MAX_ThreadS);
        }
    }
}

void PulseThread::setup(Ticks period, Ticks pulseWidth) {
    Thread::setup();
    if (pulseWidth == 0) {
        m_HighPeriod = period / 2;
    } else {
        m_HighPeriod = pulseWidth;
    }
    m_LowPeriod = period - m_HighPeriod;
}

void PulseThread::loop() {
    isHigh = !isHigh;
    if (isHigh) {
        nextLoop.ticks = threadClock.ticks+m_HighPeriod;
    } else {
        nextLoop.ticks = threadClock.ticks+m_LowPeriod;
    }
}


unsigned long totalLoops;

void MonitorThread::setup(int pinLED) {
    id = 'Z';
    // set monitor interval to not coincide with timer overflow
    PulseThread::setup(MS_TICKS(1000), MS_TICKS(250));
    this->pinLED = pinLED;
    verbose = false;
	if (pinLED != NOPIN) {
		pinMode(pinLED, OUTPUT);
	}
    blinkLED = true;

    for (byte i = 4; i-- > 0;) {
        LED(i);
        delay(500);
        LED(0);
        delay(500);
    }
}

void MonitorThread::LED(byte value) {
	if (pinLED != NOPIN) {
		digitalWrite(pinLED, value ? HIGH : LOW);
	}
}

void MonitorThread::Error(const char *msg, int value) {
    LED(3);
    for (int i = 0; i < 20; i++) {
        Serial.print('>');
    }
    Serial.print(msg);
    Serial.println(value);
}

void MonitorThread::loop() {
    PulseThread::loop();
#define MONITOR
#ifdef MONITOR
    ThreadEnable(false);
    if (blinkLED) {
        if (isHigh) {
            LED(blinkLED);
        } else {
            LED(LED_NONE);
        }
    }
    if (nTardies > 50) {
        Error("T", nTardies);
        for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
            Serial.print(pThread->id);
            Serial.print(":");
            Serial.print(pThread->tardies, DEC);
            pThread->tardies = 0;
            Serial.print(" ");
        }
        Serial.println('!');
    } else if (nTardies > 20) {
        LED(LED_YELLOW);
        verbose = true;
    }
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        if (threadClock.generation > pThread->nextLoop.generation + 1 && 
			pThread->nextLoop.generation > 0) {
			//cout << "ticks:" << threadClock.ticks 
				//<< " nextLoop:" << pThread->nextLoop.ticks 
				//<< " pThread:" << pThread->id << endl;
            Error("O@G", pThread->nextLoop.generation);
        }
    }
    if (isHigh) {
        totalLoops += nLoops;
        if (verbose) {
            Serial.print(".");
            //DEBUG_DEC("F", Free());
            DEBUG_DEC("S", millis() / 1000);
            DEBUG_DEC("G", threadClock.generation);
            DEBUG_DEC("H", nLoops);
            if (threadClock.generation > 0) {
                DEBUG_DEC("H/G", totalLoops / threadClock.generation);
            }
            DEBUG_DEC("T", nTardies);
            DEBUG_EOL();
        }
        nLoops = 0;
        nTardies = 0;
    }
    ThreadEnable(true);
#endif
}

MonitorThread firestep::monitor;

void firestep::Error(const char *msg, int value) {
    monitor.Error(msg, value);
}

ThreadRunner::ThreadRunner() {
	clear();
}

void ThreadRunner::clear() {
	pThreadList = NULL;
	threadClock.ticks = 0;
	nThreads = 0;
	nLoops = 0;
	nTardies = 0;
	generation = threadClock.generation;
	lastAge = 0;
	age = 0;
	nHB = 0;
	testTardies = 0;
	fast = 255;
	TCNT1 = 0;
}

void ThreadRunner::setup(int pinLED) {
    monitor.setup(pinLED);
    //DEBUG_DEC("CLKPR", CLKPR);
    //DEBUG_DEC("nThreads", nThreads);
    //DEBUG_EOL();

    TCCR1A = 0; // Timer mode
    TIMSK1 = 0 << TOIE1;	// disable interrupts
	lastAge = 0;
    ThreadEnable(true);
}

/**
 * The generation count has exceeded the maximum.
 * Give the machine a rest and power-cycle it.
 */ 
void ThreadRunner::resetGenerations() {
    threadClock.ticks = 0;
	lastAge = 0;
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        pThread->nextLoop.ticks = 0;
    }
}

void firestep::ThreadEnable(boolean enable) {
#ifdef DEBUG_ThreadENABLE
    DEBUG_DEC("C", ticks());
    for (ThreadPtr pThread = pThreadList; pThread; pThread = pThread->pNext) {
        Serial.print(pThread->id);
        Serial.print(":");
        Serial.print(pThread->nextLoop.ticks, DEC);
        Serial.print(" ");
    }
    DEBUG_EOL();
#endif
    if (enable) {
        //TCCR1B = 1<<CS12 | 0<<CS11 | 0<<CS10; // Timer prescaler div256
        //TCCR1B = 0 << CS12 | 0 << CS11 | 1 << CS10; // Timer prescaler div1 (16MHz)
        TCCR1B = 1 << CS12 | 0 << CS11 | 1 << CS10; // Timer prescaler div1024 (15625Hz)
    } else {
        TCCR1B = 0;	// Stop clock
    }
}

void PrintN(char c, byte n) {
    while (n--) {
        Serial.print(c);
    }
}

