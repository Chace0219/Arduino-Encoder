#include <genieArduino.h>
#include "ClickEncoder.h"
#include "TimerOne.h"
#include "FiniteStateMachine.h"

#define DOWNCOUNTTIME 30000

#define RESETLINE 2
#define ENC_SW A0
#define ENC_A A1
#define ENC_B A2


#define SELECTFORM 0
#define DOWNCOUNTFORM 1

#define INFOTEXT 0
#define SECONDTEXT 1



ClickEncoder *encoder;
int16_t lastEncoder, currEncoder;

void timerIsr() {
	encoder->service();
}

Genie genie;

void selectMenuEnter();
void selectedEnter();

uint8_t nSelectedRoutine = 0;
State downCount(NULL);
State selectMenu(selectMenuEnter, NULL, NULL);
State selected(selectedEnter, NULL, NULL);
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

}

void loop() {
	static uint32_t lastDisplayTime = millis();

	// current encoder value
	currEncoder += encoder->getValue();

	if (currEncoder != lastEncoder)
	{
		if (currEncoder > lastEncoder)
		{
			if (nSelectedRoutine < 6 && (screenMachine.isInState(selectMenu) || screenMachine.isInState(selected)))
			{
				nSelectedRoutine++;
				// screenMachine.transitionTo(selected);
				genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelectedRoutine);
			}
		}
		else
		{
			if (nSelectedRoutine > 1 && (screenMachine.isInState(selectMenu) || screenMachine.isInState(selected)))
			{
				nSelectedRoutine--;
				screenMachine.transitionTo(selected);
				genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelectedRoutine);
			}
		}
		lastEncoder = currEncoder;
	}

	// display update part as fit to status of program
	if (millis() - lastDisplayTime > 50)
	{ // every 50ms
		lastDisplayTime = millis();
		if (screenMachine.isInState(selected) || screenMachine.isInState(selectMenu))
		{
			genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelectedRoutine);
		}
		else if (screenMachine.isInState(downCount))
		{
			uint8_t nReserveTime = (DOWNCOUNTTIME - screenMachine.timeInCurrentState()) / 1000;
			String titleStr = "ROUNTINE #" + String(nSelectedRoutine);
			genie.WriteStr(0, titleStr);
			String secondStr = String(nReserveTime) + " sec";
			genie.WriteStr(1, secondStr);
			genie.WriteObject(GENIE_OBJ_COOL_GAUGE, 0, nReserveTime);
		}
	}

	// if 30 sec in countdown,
	// it will return to main menu automatically
	if (screenMachine.isInState(downCount))
	{
		if (screenMachine.timeInCurrentState() > DOWNCOUNTTIME)
		{ // transit to select menu form
			genie.WriteObject(GENIE_OBJ_FORM, 0, 0x0);
			screenMachine.transitionTo(selectMenu);
		}
	}

	ClickEncoder::Button b = encoder->getButton();
	if (b != ClickEncoder::Open) {
		switch (b)
		{
		case ClickEncoder::Clicked:
		{
			// transit to down count form
			genie.WriteObject(GENIE_OBJ_FORM, 1, 0x0);
			screenMachine.transitionTo(downCount);
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

void selectMenuEnter()
{
	// genie.WriteObject(GENIE_OBJ_FORM, SELECTFORM, 1);
	// genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelectedRoutine);
}

void selectedEnter()
{
	// genie.WriteObject(GENIE_OBJ_USERIMAGES, SELECTFORM, nSelectedRoutine);
}