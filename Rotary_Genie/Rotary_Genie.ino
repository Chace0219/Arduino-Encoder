#include <genieArduino.h>
#include "ClickEncoder.h"
#include "TimerOne.h"
#include "FiniteStateMachine.h"

#define COUNTSCREEN 1
#define SELECTSCREEN 2
#define LOGOSCREEN 3

#define LOGOSCREENTIME 3000

#define RESETLINE 2
#define ENC_SW A0
#define ENC_A A1
#define ENC_B A2

#define PUMP1 3 /// pump 1 is D6
#define PUMP2 5
#define PUMP3 6

#define PUMPACTIVE HIGH
#define PUMPDEACTIVE LOW

#define INFOTEXT 0
#define SECONDTEXT 1

#define NONE 0
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
    chosenProtocol.fillWashTime = 30 * 1000;
    chosenProtocol.fillAntiTime = 15 * 1000;
    chosenProtocol.drainTime = 60 * 1000;
    chosenProtocol.flushTime = 10 * 1000;
    chosenProtocol.washTime = 300 * 1000;
    chosenProtocol.incubationTime = 3600 * 1000;

    /*
    chosenProtocol.fillWashTime = 30 * 1000;
    chosenProtocol.fillAntiTime = 30 * 1000;
    chosenProtocol.drainTime = 30 * 1000;
    chosenProtocol.flushTime = 30 * 1000;
    chosenProtocol.washTime = 30 * 1000;
    chosenProtocol.incubationTime = 30 * 1000;
    */


    chosenProtocol.numIterations = 3;
    chosenProtocol.wash1Cnt = 0;
    chosenProtocol.wash2Cnt = 0;
  }
  break;

  case 2:
  {
    chosenProtocol.fillWashTime = 30 * 1000;
    chosenProtocol.fillAntiTime = 15 * 1000;
    chosenProtocol.drainTime = 60 * 1000;
    chosenProtocol.flushTime = 10 * 1000;
    chosenProtocol.washTime = 300 * 1000;
    chosenProtocol.incubationTime = 3600 * 1000;
    chosenProtocol.numIterations = 3;
    chosenProtocol.wash1Cnt = 0;
    chosenProtocol.wash2Cnt = 0;
  }
  break;

  case 3:
  {
    chosenProtocol.fillWashTime = 60 * 1000;
    chosenProtocol.fillAntiTime = 30 * 1000;
    chosenProtocol.drainTime = 90 * 1000;
    chosenProtocol.flushTime = 10 * 1000;
    chosenProtocol.washTime = 600 * 1000;
    chosenProtocol.incubationTime = 3600 * 1000;
    chosenProtocol.numIterations = 3;
    chosenProtocol.wash1Cnt = 0;
    chosenProtocol.wash2Cnt = 0;
  }
  break;

  case 4:
  {
    chosenProtocol.fillWashTime = 60 * 1000;
    chosenProtocol.fillAntiTime = 30 * 1000;
    chosenProtocol.drainTime = 90 * 1000;
    chosenProtocol.flushTime = 10 * 1000;
    chosenProtocol.washTime = 600 * 1000;
    chosenProtocol.incubationTime = 3600 * 1000;
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

#define PAUSE 0
#define SKIP 1
#define CANCEL 2

Genie genie;
uint8_t nSelProtocol = 0;
uint8_t nSelButton = PAUSE;
uint8_t pausedStatus = false;
static uint32_t nLastCheckedTime = millis();
static uint32_t nPrevCountTime = 0;

static uint32_t nPrevGauge = 0;
static uint32_t nPrevSecond = 0;


State idle(NULL);
State logoScreen(NULL);
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

FiniteStateMachine screenMachine(idle);


State scrollIdle(NULL);
State scrollDelay(NULL);

FiniteStateMachine scrollMachine(scrollIdle);

void setup() {

  encoder = new ClickEncoder(ENC_A, ENC_B, ENC_SW, 4, LOW); // only here, it is for PULLUP for SW
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  lastEncoder = -1;

  // 
  Serial.begin(200000);
  genie.Begin(Serial);
  // Serial.begin(115200);  // UART Debug for terminal

  // reset lcd
  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output (4D Arduino Adaptor V2 - Display Reset)
  digitalWrite(RESETLINE, LOW);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, HIGH);  // unReset the Display via D4
  delay(3000);

  // init pumps
  pinMode(PUMP1, OUTPUT);
  digitalWrite(PUMP1, PUMPDEACTIVE);
  pinMode(PUMP2, OUTPUT);
  digitalWrite(PUMP2, PUMPDEACTIVE);
  pinMode(PUMP3, OUTPUT);
  digitalWrite(PUMP3, PUMPDEACTIVE);

  // switch to logo screen
  screenMachine.transitionTo(logoScreen);
  genie.WriteObject(GENIE_OBJ_FORM, LOGOSCREEN, 0);

}

void buttonDisplay()
{
  // button display
  switch (nSelButton)
  {
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
}

void loop()
{
  static uint32_t lastDisplayTime = millis();

  scrollMachine.update();

  if (scrollMachine.isInState(scrollDelay) && scrollMachine.timeInCurrentState() > 600)
  { // once scroll triggered, for 600, another scroll will be ignored in order to unpredictiable scroll
    scrollMachine.transitionTo(scrollIdle);
  }

  // current encoder value
  currEncoder += encoder->getValue();
  if (currEncoder != lastEncoder)
  {
    if (scrollMachine.isInState(scrollIdle))
    {
      if (currEncoder > lastEncoder)
      {
        if (nSelProtocol < PROTOCOL4 && screenMachine.isInState(selectMenu))
        {
          chooseProtocol(nSelProtocol);
          nSelProtocol++;
          genie.WriteObject(GENIE_OBJ_USERIMAGES, 4, nSelProtocol);
        }
        else if (nSelButton < CANCEL && screenMachine.isInState(selectMenu) == false)
        {
          nSelButton++;
          buttonDisplay();
        }
      }
      else
      {
        if (nSelProtocol > PROTOCOL1 && screenMachine.isInState(selectMenu))
        {
          chooseProtocol(nSelProtocol);
          nSelProtocol--;
          genie.WriteObject(GENIE_OBJ_USERIMAGES, 4, nSelProtocol);
        }
        else if (nSelButton > PAUSE && screenMachine.isInState(selectMenu) == false)
        {
          nSelButton--;
          buttonDisplay();
        }
      }
    }
    lastEncoder = currEncoder;
  }

  if (screenMachine.isInState(selectMenu))
  {
    digitalWrite(PUMP1, PUMPDEACTIVE);
    digitalWrite(PUMP2, PUMPDEACTIVE);
    digitalWrite(PUMP3, PUMPDEACTIVE);
  }
  else if (screenMachine.isInState(flushFill))
  {
    if (!pausedStatus)
    { // flush fill, pump1 only on
      digitalWrite(PUMP1, PUMPACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }

  }
  else if (screenMachine.isInState(flushAction))
  { // no pump, pause flush time
    digitalWrite(PUMP1, PUMPDEACTIVE);
    digitalWrite(PUMP2, PUMPDEACTIVE);
    digitalWrite(PUMP3, PUMPDEACTIVE);
  }
  else if (screenMachine.isInState(flushDrain))
  {
    if (!pausedStatus)
    { // flush drain, pump3, drain time
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(wash1Fill))
  {
    if (!pausedStatus)
    { // wash1 fill, pump1
      digitalWrite(PUMP1, PUMPACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(wash1Action))
  { // wash1 wash, 
    digitalWrite(PUMP1, PUMPDEACTIVE);
    digitalWrite(PUMP2, PUMPDEACTIVE);
    digitalWrite(PUMP3, PUMPDEACTIVE);
  }
  else if (screenMachine.isInState(wash1Drain))
  {
    if (!pausedStatus)
    { // wahs1 drain, pump3
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(incubationFill))
  {
    if (!pausedStatus)
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(incubationAction))
  {
    digitalWrite(PUMP1, PUMPDEACTIVE);
    digitalWrite(PUMP2, PUMPDEACTIVE);
    digitalWrite(PUMP3, PUMPDEACTIVE);
  }
  else if (screenMachine.isInState(incubationDrain))
  {
    if (!pausedStatus)
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(wash2Fill))
  {
    if (!pausedStatus)
    {
      digitalWrite(PUMP1, PUMPACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }
  else if (screenMachine.isInState(wash2Action))
  {
    digitalWrite(PUMP1, PUMPDEACTIVE);
    digitalWrite(PUMP2, PUMPDEACTIVE);
    digitalWrite(PUMP3, PUMPDEACTIVE);
  }
  else if (screenMachine.isInState(wash2Drain))
  {
    if (!pausedStatus)
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPACTIVE);
    }
    else
    {
      digitalWrite(PUMP1, PUMPDEACTIVE);
      digitalWrite(PUMP2, PUMPDEACTIVE);
      digitalWrite(PUMP3, PUMPDEACTIVE);
    }
  }

  if (screenMachine.isInState(logoScreen))
  {
    if (screenMachine.timeInCurrentState() >= LOGOSCREENTIME)
    {
      screenMachine.transitionTo(selectMenu);
      genie.WriteObject(GENIE_OBJ_FORM, SELECTSCREEN, 0);
    }
  }
  else
  {
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
          nSelButton = PAUSE;
          pausedStatus = false;

          // reset timers
          nLastCheckedTime = millis();
          nPrevCountTime = 0;

          // transit 
          nPrevGauge = 0;
          nPrevSecond = 0;
          genie.WriteObject(GENIE_OBJ_FORM, COUNTSCREEN, 0x0);
          genie.WriteStr(0, F("         FILLING        "));
          buttonDisplay();

          screenMachine.transitionTo(flushFill);
        }
      }
      else
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
          }
          else
          {
            // resume counting
            nLastCheckedTime = millis();
            pausedStatus = false;
          }
          buttonDisplay();
        }
        break;

        case SKIP:
        {
          nLastCheckedTime = millis();
          nPrevCountTime = 0;
          transitNextStep();
        }
        break;

        case CANCEL:
        {
          nSelProtocol = NONE;
          screenMachine.transitionTo(selectMenu);
          genie.WriteObject(GENIE_OBJ_FORM, SELECTSCREEN, 0x0);
        }
        break;
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
  nSelButton = PAUSE;
  pausedStatus = false;
  nPrevCountTime = 0;
  nLastCheckedTime = millis();
  buttonDisplay();

  if (screenMachine.isInState(flushFill))
  {
    genie.WriteStr(0, F("        FLUSHING        "));
    screenMachine.transitionTo(flushAction);
  }
  else if (screenMachine.isInState(flushAction))
  {
    genie.WriteStr(0, F("        DRAINING        "));
    screenMachine.transitionTo(flushDrain);
  }
  else if (screenMachine.isInState(flushDrain))
  {
    genie.WriteStr(0, F("         FILLING        "));
    screenMachine.transitionTo(wash1Fill);
  }
  else if (screenMachine.isInState(wash1Fill))
  {
    genie.WriteStr(0, F("         WASHING        "));
    screenMachine.transitionTo(wash1Action);
  }
  else if (screenMachine.isInState(wash1Action))
  {
    genie.WriteStr(0, F("        DRAINING        "));
    screenMachine.transitionTo(wash1Drain);
  }
  else if (screenMachine.isInState(wash1Drain))
  {
    if (chosenProtocol.wash1Cnt >= chosenProtocol.numIterations)
    {
      genie.WriteStr(0, F("         FILLING        "));
      screenMachine.transitionTo(incubationFill);
    }
    else
    {
      genie.WriteStr(0, F("         FILLING        "));
      screenMachine.transitionTo(wash1Fill);
      chosenProtocol.wash1Cnt++;
    }
  }
  else if (screenMachine.isInState(incubationFill))
  {

    genie.WriteStr(0, F("       INCUBATION      "));
    screenMachine.transitionTo(incubationAction);
  }
  else if (screenMachine.isInState(incubationAction))
  {
    genie.WriteStr(0, F("        DRAINING       "));
    screenMachine.transitionTo(incubationDrain);
  }
  else if (screenMachine.isInState(incubationDrain))
  {
    genie.WriteStr(0, F("         FILLING        "));
    screenMachine.transitionTo(wash2Fill);
  }
  else if (screenMachine.isInState(wash2Fill))
  {
    genie.WriteStr(0, F("         WASHING        "));
    screenMachine.transitionTo(wash2Action);
  }
  else if (screenMachine.isInState(wash2Action))
  {
    genie.WriteStr(0, F("        DRAINING       "));
    screenMachine.transitionTo(wash2Drain);
  }
  else if (screenMachine.isInState(wash2Drain))
  {
    if (chosenProtocol.wash2Cnt >= chosenProtocol.numIterations)
    {
      nSelProtocol = NONE;
      screenMachine.transitionTo(selectMenu);
      genie.WriteObject(GENIE_OBJ_FORM, SELECTSCREEN, 0x0);
      genie.WriteObject(GENIE_OBJ_USERIMAGES, 4, nSelProtocol);
    }
    else
    {
      genie.WriteStr(0, F("         FILLING        "));
      screenMachine.transitionTo(wash2Fill);
      chosenProtocol.wash2Cnt++;
    }
  }
  nPrevGauge = 0;
  nPrevSecond = 0;
}

void displayCounting(uint32_t interval, uint32_t elapsed)
{
  if (nPrevGauge != (interval - elapsed) / 1000)
  {
    nPrevGauge = (interval - elapsed) / 1000;
    genie.WriteStr(1, String((interval - elapsed) / 1000) + "\n");
  }

  if (nPrevSecond != map(interval - elapsed, 0, interval, 0, 100))
  {
    nPrevSecond = map(interval - elapsed, 0, interval, 0, 100);
    genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 0x00, map(interval - elapsed, 0, interval, 0, 100));
  }
}

