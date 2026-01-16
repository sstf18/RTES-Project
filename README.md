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
