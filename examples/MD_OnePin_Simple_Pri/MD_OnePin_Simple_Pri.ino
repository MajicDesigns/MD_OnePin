// Bare Bones MD_OnePin PRI node
//
// Create the primary node and writes a continuously incrementing number to SEC.
//

#include <MD_OnePin.h>

const uint8_t COMMS_PIN = 8;

MD_OnePin OP(COMMS_PIN);

void setup(void)
{
  OP.begin();
}

void loop(void)
{
  static uint8_t count = 0;
  
  Serial.write('\n');
  Serial.write(count);
  if (!OP.write(count++))
    Serial.print(" WRITE fail");
  delay(100);
}
