#include <genieArduino.h>
#include "ClickEncoder.h"
#include "TimerOne.h"
#include "FiniteStateMachine.h"

#define DOWNCOUNTTIME 30000

#define RESETLINE 2
#define ENC_SW A0
#define ENC_A A1
#define ENC_B A2

#define PUMP1 5
#define PUMP2 6
#define PUMP3 7
#define PUMP4 8

#define PUMPACTIVE HIGH
#define PUMPDEACTIVE LOW


#define SELECTFORM 0
#define DOWNCOUNTFORM 1

#define INFOTEXT 0
#define SECONDTEXT 1

#define PROTOCOL1 1
#define PROTOCOL2 2
#define PROTOCOL3 3
#define PROTOCOL4 4

struct Protocol
{
	uint16_t fillWashTime;
	uint16_t fillAntiTime;
	uint16_t drainTime;
	uint16_t flushTime;
	uint16_t washTime;
	uint16_t incubationTime;
	uint16_t wash1Cnt;
	uint16_t wash2Cnt;
	uint16_t numIterations;
}chosenProtocol;

bool chooseProtocol(uint8_t index)
{
	if (index < 1 || index > 4)
		return false;
	switch (index)
	{
		case 1:
		{
			chosenProtocol.fillWashTime = 30;
			chosenProtocol.fillAntiTime = 15;
			chosenProtocol.drainTime = 60;
			chosenProtocol.flushTime = 10;
			chosenProtocol.washTime = 300;
			chosenProtocol.incubationTime = 3600;
			chosenProtocol.numIterations = 3;
			chosenProtocol.wash1Cnt = 0;
			chosenProtocol.wash2Cnt = 0;
		}
		break;

		case 2:
		{
			chosenProtocol.fillWashTime = 30;
			chosenProtocol.fillAntiTime = 15;
			chosenProtocol.drainTime = 60;
			chosenProtocol.flushTime = 10;
			chosenProtocol.washTime = 300;
			chosenProtocol.incubationTime = 3600;
			chosenProtocol.numIterations = 3;
			chosenProtocol.wash1Cnt = 0;
			chosenProtocol.wash2Cnt = 0;
		}
		break;

		case 3:
		{
			chosenProtocol.fillWashTime = 60;
			chosenProtocol.fillAntiTime = 30;
			chosenProtocol.drainTime = 90;
			chosenProtocol.flushTime = 10;
			chosenProtocol.washTime = 600;
			chosenProtocol.incubationTime = 3600;
			chosenProtocol.numIterations = 3;
			chosenProtocol.wash1Cnt = 0;
			chosenProtocol.wash2Cnt = 0;
		}
		break;

		case 4:
		{
			chosenProtocol.fillWashTime = 60;
			chosenProtocol.fillAntiTime = 30;
			chosenProtocol.drainTime = 90;
			chosenProtocol.flushTime = 10;
			chosenProtocol.washTime = 600;
			chosenProtocol.incubationTime = 3600;
			chosenProtocol.numIterations = 3;
			chosenProtocol.wash1Cnt = 0;
			chosenProtocol.wash2Cnt = 0;
		}
		break;
	}
	return true;
}

ClickEncoder *encoder;
int16_t lastEncoder, currEncoder;

void timerIsr() {
	encoder->service();
}

#define NONE 0
#define PAUSE 1
#define SKIP 2
#define CANCEL 3

Genie genie;
uint8_t nSelProtocol = 0;
uint8_t nSelButton = NONE;
uint8_t pausedStatus = false;
static uint32_t nLastCheckedTime = millis();
static uint32_t nPrevCountTime = 0;

State selectMenu(NULL);

State flushFill(NULL);
State flushAction(NULL);
State flushDrain(NULL);

State wash1Fill(NULL);
State wash1Action(NULL);
State wash1Drain(NULL);

State incubationFill(NULL);
State incubationAction(NULL);
State incubationDrain(NULL);

State wash2Fill(NULL);
State wash2Action(NULL);
State wash2Drain(NULL);

FiniteStateMachine screenMachine(selectMenu);

void setup() {

	encoder = new ClickEncoder(ENC_A, ENC_B, ENC_SW, 4);
	Timer1.initialize(1000);
	Timer1.attachInterrupt(timerIsr);
	lastEncoder = -1;

	Serial.begin(200000);  // UART Debug for terminal
	genie.Begin(Serial);
	// Serial.begin(115200);  // UART Debug for terminal

	// reset lcd
	pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output (4D Arduino Adaptor V2 - Display Reset)
	digitalWrite(RESETLINE, LOW);  // Reset the Display via D4
	delay(100);
	digitalWrite(RESETLINE, HIGH);  // unReset the Display via D4

	delay(3500);

	// init pumps
	pinMode(PUMP1, OUTPUT);
	digitalWrite(PUMP1, PUMPDEACTIVE);
	pinMode(PUMP2, OUTPUT);
	digitalWrite(PUMP2, PUMPDEACTIVE);
	pinMode(PUMP3, OUTPUT);
	digitalWrite(PUMP3, PUMPDEACTIVE);
	pinMode(PUMP4, OUTPUT);
	digitalWrite(PUMP4, PUMPDEACTIVE);

}

void loop() {
	static uint32_t lastDisplayTime = millis();

	// current encoder value
	currEncoder += encoder->getValue();

	if (currEncoder != lastEncoder)
	{
		if (currEncoder > lastEncoder)
		{
			if (nSelProtocol < PROTOCOL4 && screenMachine.isInState(selectMenu))
			{
				chooseProtocol(nSelProtocol);
				nSelProtocol++;
			}
			else if (nSelButton < CANCEL && screenMachine.isInState(selectMenu) == false)
			{
				nSelButton++;
			}
		}
		else
		{
			if (nSelProtocol > PROTOCOL1 && screenMachine.isInState(selectMenu))
			{
				chooseProtocol(nSelProtocol);
				nSelProtocol--;
			}
			else if (nSelButton > NONE && screenMachine.isInState(selectMenu) == false)
			{
				nSelButton--;
			}
		}
		lastEncoder = currEncoder;
	}

	// display update part as fit to status of program
	if (millis() - lastDisplayTime > 50)
	{ // every 50ms
		lastDisplayTime = millis();
		if (screenMachine.isInState(selectMenu))
		{
			genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelProtocol);
		}
		else
		{
			// Title String
			if (screenMachine.isInState(flushFill))
				genie.WriteStr(0, F("FILLING"));
			else if(screenMachine.isInState(flushAction))
				genie.WriteStr(0, F("FLUSHING"));
			else if (screenMachine.isInState(flushDrain))
				genie.WriteStr(0, F("DRAINING"));
			else if (screenMachine.isInState(wash1Fill))
				genie.WriteStr(0, F("FILLING"));
			else if (screenMachine.isInState(wash1Action))
				genie.WriteStr(0, F("WASHING"));
			else if (screenMachine.isInState(wash1Drain))
				genie.WriteStr(0, F("DRAINING"));
			else if (screenMachine.isInState(incubationFill))
				genie.WriteStr(0, F("FILLING"));
			else if (screenMachine.isInState(incubationAction))
				genie.WriteStr(0, F("INCUBATION"));
			else if (screenMachine.isInState(incubationDrain))
				genie.WriteStr(0, F("DRAINING"));
			else if (screenMachine.isInState(wash2Fill))
				genie.WriteStr(0, F("FILLING"));
			else if (screenMachine.isInState(wash2Action))
				genie.WriteStr(0, F("WASHING"));
			else if (screenMachine.isInState(wash2Drain))
				genie.WriteStr(0, F("DRAINING"));

			// button display
			switch (nSelButton)
			{
				case NONE:
				{
					if(pausedStatus)
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 2);
					else
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 0);

					genie.WriteObject(GENIE_OBJ_USERIMAGES, 2, 0);
					genie.WriteObject(GENIE_OBJ_USERIMAGES, 3, 0);
				}
				break;

				case PAUSE:
				{
					if (pausedStatus)
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 3);
					else
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 1);

					genie.WriteObject(GENIE_OBJ_USERIMAGES, 2, 0);
					genie.WriteObject(GENIE_OBJ_USERIMAGES, 3, 0);
				}
				break;

				case SKIP:
				{
					if (pausedStatus)
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 2);
					else
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 0);

					genie.WriteObject(GENIE_OBJ_USERIMAGES, 2, 1);
					genie.WriteObject(GENIE_OBJ_USERIMAGES, 3, 0);
				}
				break;

				case CANCEL:
				{
					if (pausedStatus)
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 2);
					else
						genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 0);

					genie.WriteObject(GENIE_OBJ_USERIMAGES, 2, 0);
					genie.WriteObject(GENIE_OBJ_USERIMAGES, 3, 1);
				}
				break;
			}

			// display gauge and second string
			if (screenMachine.isInState(flushFill))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.fillWashTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}

			}
			else if (screenMachine.isInState(flushAction))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.flushTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}

			}
			else if (screenMachine.isInState(flushDrain))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.drainTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(wash1Fill))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.fillWashTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}

			}
			else if (screenMachine.isInState(wash1Action))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.washTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}

			}
			else if (screenMachine.isInState(wash1Drain))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.drainTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(incubationFill))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.fillAntiTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(incubationAction))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.incubationTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(incubationDrain))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.drainTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(wash2Fill))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.fillWashTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}
			else if (screenMachine.isInState(wash2Action))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.washTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}

			}
			else if (screenMachine.isInState(wash2Drain))
			{
				if (!pausedStatus)
				{
					uint32_t nElapsedTime = nPrevCountTime + (millis() - nLastCheckedTime);
					uint32_t nTargetTime = chosenProtocol.drainTime;
					if (nElapsedTime > nTargetTime)
						transitNextStep();
					else
						displayCounting(nTargetTime, nElapsedTime);
				}
			}

			//  control pumps
			if (screenMachine.isInState(selectMenu))
			{
				digitalWrite(PUMP1, PUMPDEACTIVE);
				digitalWrite(PUMP2, PUMPDEACTIVE);
				digitalWrite(PUMP3, PUMPDEACTIVE);
				digitalWrite(PUMP4, PUMPDEACTIVE);
			}
			else if (screenMachine.isInState(flushFill))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}

			}
			else if (screenMachine.isInState(flushAction))
			{
				digitalWrite(PUMP1, PUMPDEACTIVE);
				digitalWrite(PUMP2, PUMPDEACTIVE);
				digitalWrite(PUMP3, PUMPDEACTIVE);
				digitalWrite(PUMP4, PUMPDEACTIVE);
			}
			else if (screenMachine.isInState(flushDrain))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(wash1Fill))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(wash1Action))
			{
				digitalWrite(PUMP1, PUMPDEACTIVE);
				digitalWrite(PUMP2, PUMPDEACTIVE);
				digitalWrite(PUMP3, PUMPDEACTIVE);
				digitalWrite(PUMP4, PUMPDEACTIVE);
			}
			else if (screenMachine.isInState(wash1Drain))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(incubationFill))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(incubationAction))
			{
				digitalWrite(PUMP1, PUMPDEACTIVE);
				digitalWrite(PUMP2, PUMPDEACTIVE);
				digitalWrite(PUMP3, PUMPDEACTIVE);
				digitalWrite(PUMP4, PUMPDEACTIVE);
			}
			else if (screenMachine.isInState(incubationDrain))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(wash2Fill))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
			else if (screenMachine.isInState(wash2Action))
			{
				digitalWrite(PUMP1, PUMPDEACTIVE);
				digitalWrite(PUMP2, PUMPDEACTIVE);
				digitalWrite(PUMP3, PUMPDEACTIVE);
				digitalWrite(PUMP4, PUMPDEACTIVE);
			}
			else if (screenMachine.isInState(wash2Drain))
			{
				if (!pausedStatus)
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
				else
				{
					digitalWrite(PUMP1, PUMPDEACTIVE);
					digitalWrite(PUMP2, PUMPDEACTIVE);
					digitalWrite(PUMP3, PUMPDEACTIVE);
					digitalWrite(PUMP4, PUMPDEACTIVE);
				}
			}
		}
	}

	ClickEncoder::Button b = encoder->getButton();
	if (b != ClickEncoder::Open) {
		switch (b)
		{
		case ClickEncoder::Clicked:
		{
			// when select menu screen, it will enter protocol mode
			if (screenMachine.isInState(selectMenu))
			{
				if (nSelProtocol > NONE)
				{
					nSelButton = NONE; // none button select
					pausedStatus = false;

					// transit 
					genie.WriteObject(GENIE_OBJ_FORM, DOWNCOUNTFORM, 0x0);
					screenMachine.transitionTo(flushFill);
				}
			}
			else
			{
				//
				if (nSelButton > NONE)
				{
					switch (nSelButton)
					{
						case PAUSE:
						{
							if (!pausedStatus)
							{
								nPrevCountTime = millis() - nLastCheckedTime;
								nLastCheckedTime = millis();
								pausedStatus = true;
								// paused counting
								genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 3);
							}
							else
							{
								// resume counting
								nLastCheckedTime = millis();
								pausedStatus = false;
								genie.WriteObject(GENIE_OBJ_USERIMAGES, 1, 1);
							}
						}
						break;

						case SKIP:
						{
							nSelButton = NONE;
							pausedStatus = false;
							nLastCheckedTime = millis();
							nPrevCountTime = 0;
							transitNextStep();
						}
						break;

						case CANCEL:
						{
							nSelProtocol = NONE;
							screenMachine.transitionTo(selectMenu);
							//
							genie.WriteObject(GENIE_OBJ_FORM, SELECTFORM, 0x0);
						}
						break;
					}
				}
			}
		}
		break;
		default:
		{

		}
		break;
		}
	}
	screenMachine.update();
}


void transitNextStep()
{
	if (screenMachine.isInState(flushFill))
		screenMachine.transitionTo(flushAction);
	else if (screenMachine.isInState(flushAction))
		screenMachine.transitionTo(flushDrain);
	else if (screenMachine.isInState(flushDrain))
		screenMachine.transitionTo(wash1Fill);
	else if (screenMachine.isInState(wash1Fill))
		screenMachine.transitionTo(wash1Action);
	else if (screenMachine.isInState(wash1Action))
		screenMachine.transitionTo(wash1Drain);
	else if (screenMachine.isInState(wash1Drain))
	{
		if (chosenProtocol.wash1Cnt >= chosenProtocol.numIterations)
			screenMachine.transitionTo(incubationFill);
		else
		{
			screenMachine.transitionTo(wash1Fill);
			chosenProtocol.wash1Cnt++;
		}
	}
	else if (screenMachine.isInState(incubationFill))
		screenMachine.transitionTo(incubationAction);
	else if (screenMachine.isInState(incubationAction))
		screenMachine.transitionTo(incubationDrain);
	else if (screenMachine.isInState(incubationDrain))
		screenMachine.transitionTo(wash2Fill);
	else if (screenMachine.isInState(wash2Fill))
		screenMachine.transitionTo(wash2Action);
	else if (screenMachine.isInState(wash2Action))
		screenMachine.transitionTo(wash2Drain);
	else if (screenMachine.isInState(wash2Drain))
	{
		if (chosenProtocol.wash2Cnt >= chosenProtocol.numIterations)
		{
			nSelProtocol = NONE;
			screenMachine.transitionTo(selectMenu);
			genie.WriteObject(GENIE_OBJ_FORM, SELECTFORM, 0x0);
		}
		else
		{
			screenMachine.transitionTo(wash2Fill);
			chosenProtocol.wash2Cnt++;
		}
	}
}

void displayCounting(uint32_t interval, uint32_t elapsed)
{
	genie.WriteStr(1, String((interval - elapsed) / 1000));
	genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 0x00, map(interval - elapsed, 0, interval, 0, 100));
}