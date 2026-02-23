#include <stdint.h>

#define GPIO_OUT_REG     0x3FF44004
#define GPIO_ENABLE_REG  0x3FF44020
#define GPIO_IN_REG      0x3FF4403C
#define IO_MUX_GPIO5_REG 0x3FF4906C

static inline uint32_t cpu_cycles() {
  uint32_t ccount;
  asm volatile ("rsr %0, ccount" : "=a"(ccount));
  return ccount;
}

volatile uint32_t *gpio_out = (uint32_t*) GPIO_OUT_REG;
volatile uint32_t *gpio_en  = (uint32_t*) GPIO_ENABLE_REG;
volatile uint32_t *gpio_in  = (uint32_t*) GPIO_IN_REG;
volatile uint32_t *gpio5_config = (uint32_t*) IO_MUX_GPIO5_REG;

volatile int last_raw;
volatile uint32_t last_change_time;

int stable_state = 1; // it is a pull_up
int prev_stable_state = stable_state; 
int led_state = 0; // led is off initially

void IRAM_ATTR button_isr(){
  last_raw = ((*gpio_in) >> 5) & 1; // update value at most recent transition
  last_change_time = cpu_cycles();
}

void setup() {

  *gpio5_config &= ~(0x7u << 12);  // clear bits 12..14
  *gpio5_config |= (1u << 8); // FUN_WPU pin 5 internal pull up enabled
  *gpio5_config |= (1u << 9); // FUN_IE pin 5 can sense inputs, this needs to be explicit
  *gpio5_config &= ~(1u << 7); // FUN_WPD pin 5 internal pull down disabled
  *gpio_en &= ~(1u << 5); // ensures pin 5 will not drive output
  *gpio_en |= (1u << 2); // ensure pin 2 (led) is an output

  noInterrupts();
  last_change_time = cpu_cycles();
  last_raw = ((*gpio_in) >> 5) & 1;
  interrupts();

  attachInterrupt(digitalPinToInterrupt(5), button_isr, CHANGE);
}

void loop() {
  noInterrupts();
  int last_raw_local = last_raw;
  uint32_t last_change_time_local = last_change_time;
  interrupts();
  if((cpu_cycles() - last_change_time_local >= 2400000) && (last_raw_local != stable_state)){
      stable_state = last_raw_local;
      if(stable_state == 0){
        led_state ^=1;
        *gpio_out = (*gpio_out&~(1u << 2))|((uint32_t)led_state << 2);
      }
      prev_stable_state = stable_state;
    }  
}
