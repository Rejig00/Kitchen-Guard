/* Inside the attached function, delay() won’t work and the value 
 * returned by millis() will not increment. Serial data received 
 * while in the function may be lost. You should declare as volatile 
 * any variables that you modify within the attached function. 
 * See the section on ISRs below for more information
 */

#define interruptPin  17
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;


void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  //delay(1);       //delay wont work as millis() don't increase
  portEXIT_CRITICAL_ISR(&mux);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Monitoring interrupts: ");
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(interruptCounter>0){
 
      portENTER_CRITICAL(&mux);
      interruptCounter--;
      portEXIT_CRITICAL(&mux);
 
      numberOfInterrupts++;
      Serial.print("An interrupt has occurred. Total: ");
      Serial.println(numberOfInterrupts);
  }
}



