#include <Herkulex.h>

int n=1; //motor ID - verify your ID !!!!
bool is0601 = false;

void setup()  
{
  delay(2000);  //a delay to have time for serial monitor opening
  Serial.begin(115200);    // Open serial communications
  Serial.println("Begin");
  Herkulex.begin(57600,10,11); //open serial with rx=10 and tx=11 
  Herkulex.reboot(n); //reboot first motor
  delay(500); 
  Herkulex.initialize(); //initialize motors
  delay(200);  
}

void loop(){
  Serial.println("Move Angle: -100 degrees");
  Herkulex.moveOneAngle(n, -100, 1000, LED_BLUE, false); //move motor with 300 speed  
  delay(1200);
  Serial.print("Get servo Angle:");
  Serial.println(Herkulex.getAngle(n, is0601));
  Serial.println("Move Angle: 100 degrees");
  Herkulex.moveOneAngle(n, 100, 1000, LED_BLUE, false); //move motor with 300 speed  
  delay(1200);
  Serial.print("Get servo Angle:");
  Serial.println(Herkulex.getAngle(n, is0601));
  
}


