/*
Title :- CONTACTLESS BREATHING DETECTION USING ULTRASONIC WAVES.

Team :- 
asv8775 - Anand Vishwakarma
rg4357  - Raj Ghodasara
jy4286  - Jianhua Yang
sa7055 - Sujay Anantha

Sensor Used :- HCSR-04 Ultrasonic sensor. (40kHz operating frequency)

Description :- 
    Why Contactless?
    - Contact-based sensing will interfere with the sleep of baby. Also, due to the irritation, the baby might pull the sensor off,
      interfering with the readings, or disconnecting the sensor altogether.
    - It is also more prone to wear and tear due to movement of the baby.
    - On the other hand, a contactless sensor can be placed at a distance, out of the reach of the baby, thus not interfering with 
      the natural sleep of the baby. It can also be disguised in the form of a toy placed above the crib of the baby.

    What Sensor we used?
    - We used HCSR-04 sensor which is ultrasonic sensor consisting of transmitter and receiver with an operating frequency of 40kHz.
    - It features 4 pins:
      -> Trig - when this pin is high for 10us, it transmits 8 pulses of ultrasonic soundwaves.
      -> Echo - This pin is pulled high after all pulses of ultrasonic soundwaves are transmitted until receiver receives a 
                reflected soundwave.
      -> Vcc - 5V power suppy
      -> Gnd - ground connection
    
    STM32 pins used in this project:
      -> GPIOA pin 9 - as an output for Trig pin of HCSR-04.
      -> GPIOA pin 8 - as an input from Echo pin of HCSR-04 attached with an interrupt.
      -> 5V Vcc pin - connected to Vcc of HCSR-04.
      -> Gnd pin - connected to Gnd pin of HCSR-04 and ( - ) pin of buzzer.
      -> GPIOA pin 11 - as an output connected to ( + ) pin of buzzer as an alerting media.
      -> User Button - to toggle state of the system.
      -> LED! - as a alerting media on the board.

    How we detected breathing?
    - We are measuring time taken for the reflected wave to be captured by the receiver.
    - An interrupt is fired on the rising edge of PA8 to start the timer, and the timer is stored on the falling edge of PA8.
    - The difference gives the elapsed time for the ultrasonic wave. More, specifically we subtract the elapsedTime and 
      previousElapsedTime to get the timeDiff.
    - We need to measure the variation between readings to detect expansion or contraction of the stomach and chest area. If the
      variation is above a particular threshold, this indicates expansion or contraction of the stomach and chest area, and hence,
      breathing.
    - However, there is a critical decision to be made. The sensor gives new elapsed time much faster than the rate of breathing,
      or even the rate at which the body would move while breathing in or out. This would mean that comparing consecutive values
      of the elapsed time would not give a significant difference between breathing and not breathing. One solution to this would
      be to capture new values at a slower rate. However, this would result in a significant information being lost between 
      consecutive captures.
    - We solved this problem by introducing a cycle counter. The calculation of timeDiff is done in the same way. However, the values
      of elapsedTime are stored in previousElapsedTime every n cycles.
    - This means that we get new values of elapsedTime as frequently as before, and hence no information is lost. This also means 
      the new values of elapsedTime are compared with previousElapsedTime from n cycles before.
    - Through trial and error, we determined that the value of this cycleCount should be 10 and the threshold for the difference should
      be 30 microseconds.

Conclusion :- 

This real-time embedded system successfully detects breathing and more importantly, a lack of breathing. The contactless aspect has 
multiple benefits when compared to contact-based solutions.

*/



#include <mbed.h>
#include <stack>


#include "drivers/LCD_DISCO_F429ZI.h"
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

using namespace std::chrono;


//buffer for holding displayed text strings
char displayBuf[4][60];

// Declare and initialize global variables
LCD_DISCO_F429ZI display;
volatile int32_t elapsedTime;
volatile int32_t state; // detecting breathing if set to 1, otherwise standby mode
DigitalOut triggerPin(PA_9); // GPIOA pin 9 as output pin to send trigger to HCSR-04
InterruptIn intEcho(PA_8, PullDown); // GPIOA pin 8 as input to capture echo pulse width of HCSR-04
InterruptIn intButton(USER_BUTTON, PullDown); // USER Button of stm32 to change state
DigitalOut buzzerAlert(PA_11); // GPIOA pin 11 as output for alerting media (Buzzer)
DigitalOut alertLED(LED1); // LED1 as output for alerting through stm32
Timer echoTimer; // timer for measuring echo pulse
Timer breathTimer; // timer for breathing detection



// interrupt for echo pin on rising edge
// reset echo timer to start measuring echo time
void intEchoRisingEdge(){
  echoTimer.reset();
}

// interrupt for echo pin on falling edge
// capture current timer value to get current echo pulse width length in time
void intEchoFallingEdge(){
  elapsedTime = echoTimer.elapsed_time().count();
}

// interrupt for user button on falling edge
// toggle state and reset breathing timer
void intButtonFallingEdge(){
  state = !state;
  breathTimer.reset();
  display.SelectLayer(FOREGROUND);
  display.Clear(LCD_COLOR_BLACK);
  snprintf(displayBuf[2],60,"                    ");
  snprintf(displayBuf[3],60,"                    ");
  display.DisplayStringAt(0, LINE(10), (uint8_t *)displayBuf[2], LEFT_MODE);
  display.DisplayStringAt(0, LINE(11), (uint8_t *)displayBuf[3], LEFT_MODE);
}

//sets the background layer 
//to be visible, transparent, and
//resets its colors to all black
void setupBackgroundLayer(){
  display.SelectLayer(BACKGROUND);
  display.Clear(LCD_COLOR_BLACK);
  display.SetBackColor(LCD_COLOR_BLACK);
  display.SetTextColor(LCD_COLOR_GREEN);
  display.SetLayerVisible(BACKGROUND,ENABLE);
  display.SetTransparency(BACKGROUND,0x7Fu);
}

//resets the foreground layer to
//all black
void setup_foreground_layer(){
    display.SelectLayer(FOREGROUND);
    display.Clear(LCD_COLOR_BLACK);
    display.SetBackColor(LCD_COLOR_BLACK);
    display.SetTextColor(LCD_COLOR_WHITE);
}


//set text background color on LCD
void setTextBackColor(uint32_t color) {
  display.SelectLayer(FOREGROUND);
  display.SetBackColor(color);
}

int main(){
  
  // reset initial background and text color for LCD.
  setupBackgroundLayer();
  setup_foreground_layer();

  // assign interrupt logic to pins
  intEcho.rise(&intEchoRisingEdge);
  intEcho.fall(&intEchoFallingEdge);
  intButton.fall(&intButtonFallingEdge);

  // set alert LED to 0
  alertLED=0;

  // start echo timer for echo width calculation
  echoTimer.start();

  // initialize variables for breathing logic
  int32_t previousElapsedTime=0;
  int32_t timeDiff;
  int32_t cycleCount = 0;
  breathTimer.start();
  state = 0;


  while(1){

    // check if user pressed start or stop
    // Output message on LCD if state is 0 i.e standby
    if (state == 0) {
      setTextBackColor(LCD_COLOR_BLACK);
      snprintf(displayBuf[2],60,"Press user button to");
      snprintf(displayBuf[3],60,"start detection");
      display.DisplayStringAt(0, LINE(10), (uint8_t *)displayBuf[2], LEFT_MODE);
      display.DisplayStringAt(0, LINE(11), (uint8_t *)displayBuf[3], LEFT_MODE);
      continue;
    }

    // giving trigger to HCSR-04 to transmit ultrasonic pulse
    triggerPin = 1;
    wait_us(10);
    triggerPin = 0;

    // capturing time difference of current from previous captured time
    timeDiff = elapsedTime - previousElapsedTime;

    // storing current elapsed time every 10 cycle
    // Time difference between consecutive cycle is not differentiable as sensor output
    // and input capture rate is too high compared to human breathing rate.
    // hence we capture elapsed time every 10 cycle.
    if (cycleCount > 10) {
      previousElapsedTime = elapsedTime;
      cycleCount = 0;
    }
    cycleCount++;
    
    // reset when breathing is detected
    // when breathing takes place we get more than (+-)30 time difference from the sensor
    if (timeDiff > 30 || timeDiff < -30) {

      breathTimer.reset(); // reset breath timer
      alertLED = 0; // reset LED1 on stm32 to 0
      buzzerAlert = 0; // reset buzzer output to 0

      // clear alert message on LCD if present
      setTextBackColor(LCD_COLOR_BLACK);
      snprintf(displayBuf[2],60,"          "); 
      display.DisplayStringAt(70, LINE(10), (uint8_t *)displayBuf[2], LEFT_MODE);
    }

    // check if not breathing for 10s
    // give alert on LCD and through external media (Buzzer,LED)
    if (breathTimer.elapsed_time().count() >= 10000000) {
      alertLED = 1; // set LED1 on stm32 to 1
      buzzerAlert = 1; // set buzzer output to 1

      // output alert on LCD
      setTextBackColor(LCD_COLOR_RED);
      snprintf(displayBuf[2],60,"Alert!!!");
      display.DisplayStringAt(70, LINE(10), (uint8_t *)displayBuf[2], LEFT_MODE);
    }


    // output time from the last detected breath on LCD
    setTextBackColor(LCD_COLOR_BLACK);
    snprintf(displayBuf[0],60,"Stopped Breathing for");
    snprintf(displayBuf[1],60,"%llu seconds    ", duration_cast<seconds>(breathTimer.elapsed_time()).count());
    display.DisplayStringAt(0, LINE(17), (uint8_t *)displayBuf[0], LEFT_MODE);
    display.DisplayStringAt(0, LINE(18), (uint8_t *)displayBuf[1], LEFT_MODE);

    // uncomment to monitor raw echo time from sensor.
    printf("Cycle: %lu\tCurrent Time: %lu\tPrevious Time: %lu\tTime Difference: %ld\n",cycleCount,elapsedTime,previousElapsedTime,timeDiff);
  }
}