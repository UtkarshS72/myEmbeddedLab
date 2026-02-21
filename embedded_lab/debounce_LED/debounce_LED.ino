#include <Arduino.h>
#include <stdint.h>

#define GPIO_OUT_REG     0x3FF44004
#define GPIO_ENABLE_REG  0x3FF44020
#define GPIO_IN_REG      0x3FF4403C

static inline uint32_t cpu_cycles() {
  uint32_t ccount;
  asm volatile ("rsr %0, ccount" : "=a"(ccount));
  return ccount;
}

void setup() {
  Serial.begin(115200);

  volatile uint32_t *gpio_out = (uint32_t*) GPIO_OUT_REG;
  volatile uint32_t *gpio_en  = (uint32_t*) GPIO_ENABLE_REG;
  volatile uint32_t *gpio_in  = (uint32_t*) GPIO_IN_REG;

  // LED on GPIO2
  *gpio_en |= (1u << 2);

  // Button on GPIO5 with pull-up 
  pinMode(5, INPUT_PULLUP);
  *gpio_en &= ~(1u << 5); // ensure input

  int last_raw = ((*gpio_in) >> 5) & 1; // last value measured at 5th bit
  int stable_state = 1; // it is a pull_up
  int prev_stable_state = stable_state; 
  uint32_t last_change_time = 0;
  int led_state = 0; // led is off initially
  uint32_t window  = 2400000; // ~10ms at 240MHz (approx), window is approx 10ms

  while (1) {
    int raw = ((*gpio_in) >> 5) & 1; // current value at 5th bit (changes constantly as loop runs)

    if (last_raw != raw) { // edge detected
      last_change_time = cpu_cycles(); // when the first button press was detected
      

      last_raw = raw; // save the value we changed to at the edge

      
    }

    if((last_raw == raw) && ((cpu_cycles() - last_change_time) >= window) && (raw != stable_state)){ // here the last_raw == raw is not really needed, I'm leaving it here to prevent future confusion
      stable_state = raw;
      if((prev_stable_state == 1) && (stable_state == 0)){
        led_state ^= 1;
        *gpio_out = (*gpio_out&~(1u << 2))|(led_state << 2);
      }
      prev_stable_state = stable_state;
    }
    
  }
}

void loop() {}
