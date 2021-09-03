// Test code to control MD_OnePin PRI node
// 
// Allows controlled testing of PRI MD_OnePin comms link 
// primitives through a command line interface.
// Requires the matching SEC sketch to be running on the 
// SEC node
// 
// Dependencies:
// MD_cmdProcessor is available from https://github.com/MajicDesigns/MD_cmdProcessor 
//                 or the IDE Library Manager.
//

#include <MD_OnePin.h>
#include <MD_cmdProcessor.h>

const uint8_t COMMS_PIN = 8;  // pin used for communications

bool iterate = false;         // set true when main loop has something to do
bool RWalternate = false;     // alternate read and write in main loop
bool enableOutput = true;     // enable Serial output
bool enableBenchmark = false; // enable benchmark counters

uint16_t timeDelay = 1000;    // delay time when iterating

enum { S_IDLE, S_READ, S_WRITE, S_RW } loopState = S_IDLE;  // main loop execution state
MD_OnePin::packet_t writeData = 0;      // write data register
MD_OnePin OP(COMMS_PIN);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

// --- Comand Processor Data
// handler function prototypes
void handlerHelp(char* param);

// handler functions
void handlerX(char* param)  { loopState = S_IDLE;  RWalternate = false; }
void handlerRW(char* param) { loopState = S_READ;  RWalternate = true; }
void handlerB(char* param)  { enableBenchmark = !enableBenchmark; }
void handlerR(char* param) { loopState = S_READ; RWalternate = false; }

void handlerR1(char * param)
{
  MD_OnePin::packet_t data;

  Serial.print(F("\nRead 0x"));
  data = OP.read();
  Serial.print(data, HEX);
  if (!OP.isPresent()) Serial.print(F(" failed"));
}

void handlerW(char* param) 
{
  if (strlen(param) == 0)
  {
    loopState = S_WRITE;
    RWalternate = false;
  }
  else
  {
    MD_OnePin::packet_t data = strtoul(param, nullptr, 0);
    Serial.print(F("\nWrite 0x"));
    Serial.print(data, HEX);
    Serial.print(F(" ("));
    Serial.print(data);
    Serial.print(F(") "));

    if (!OP.write(data)) Serial.print(F("failed"));
  }
}

void handlerO(char* param) 
{ 
  enableOutput = !enableOutput; 
  Serial.print(F("\nOutput: "));
  Serial.print(enableOutput ? "On" : "Off");
}

void handlerP(char* param)
{
  timeDelay = strtoul(param, nullptr, 0);

  Serial.print(F("\nPeriod (ms):"));
  Serial.print(timeDelay);
}

void handlerD(char* param)
{
  writeData = strtoul(param, nullptr, 0);

  Serial.print(F("\nW Reg: 0x"));
  Serial.print(writeData, HEX);
  Serial.print(F(" ("));
  Serial.print(writeData);
  Serial.print(F(")"));
}


const MD_cmdProcessor::cmdItem_t PROGMEM cmdTable[] =
{
  { "?",  handlerHelp, "",    "Help", 0 },
  { "h",  handlerHelp, "",    "Help", 0 },

  { "r",  handlerR,  "",   "Read Once from from SEC", 1 },
  { "r1", handlerR1, "",   "Iterate Read from SEC", 1 },
  { "w",  handlerW,  "[n]","Iterate Write [or simple Write n] to SEC", 1 },
  { "rw", handlerRW, "",   "Alternate Reads and Writes", 1 },

  { "b",  handlerB,  "",  "Toggle Benchmarking output", 2 },
  { "d",  handlerD,  "n", "Set write Data register value (uint32_t)", 2 },
  { "o",  handlerO,  "",  "Toggle RW Output to serial monitor", 2},
  { "p",  handlerP,  "n", "Set main loop execution Period (ms)", 2 },
  { "x",  handlerX,  "",  "Stop main loop eXecution", 2 },
};

// Global command table
MD_cmdProcessor CP(Serial, cmdTable, ARRAY_SIZE(cmdTable));

void handlerHelp(char* param)
{
  Serial.print(F("\n----"));
  CP.help();
  Serial.print(F("\n"));
}

void setup(void)
{
  Serial.begin(57600);
  Serial.print("\n[MD_OnePin PRI Test]");
  Serial.print(F("\nEnter command. Ensure line ending set to newline.\n"));

  CP.begin();
  CP.help();

  OP.begin();
}

void loop(void)
{
  static uint32_t timeBench = 0;   // benchmarking timer in ms
  static uint16_t opsCount = 0;    // RW operations counted
  static uint16_t failCount = 0;   // failed RW operations counted
  static uint32_t bitCount = 0;    // number of bits transferred
  static uint32_t timeStart = 0;   // in ms

  CP.run();

  if (millis() - timeStart >= timeDelay)
  {
    if (RWalternate) loopState = (loopState == S_READ ? S_WRITE : S_READ);

    switch (loopState)
    {
    case S_IDLE:
    case S_RW:
      break;    // do nothing

    case S_READ:
      {
        MD_OnePin::packet_t data;

        data = OP.read(); 
        if (!OP.isPresent()) failCount++;
        else bitCount += BPP_SEC;
        opsCount++;
        if (enableOutput)
        {
          Serial.print("\n");
          Serial.print(data, HEX);
          if (!OP.isPresent()) Serial.print(" READ fail");
        }
      }
      break;

    case S_WRITE:
      if (enableOutput)
      {
        Serial.print("\n");
        Serial.print(writeData, HEX);
        Serial.write(' ');
      }
      if (!OP.write(writeData))
      {
        failCount++;
        if (enableOutput) Serial.print(" WRITE fail");
      }
      else 
        bitCount += BPP_PRI;

      opsCount++;
      writeData++;
      break;
    }

    timeStart = millis(); // reset for next run
  }

  // if benchmarking, write the number of operations each second
  if (enableBenchmark && loopState != S_IDLE)
  {
    if (millis() - timeBench >= 1000)
    {
      Serial.write('\n');
      Serial.print(opsCount);
      Serial.print(F(" ops/s ["));
      Serial.print(failCount);
      Serial.print(F(" fail] @ "));
      Serial.print(bitCount);
      Serial.print(F(" bps"));
      timeBench = millis();
      opsCount = failCount = bitCount = 0;
    }
  }
}
