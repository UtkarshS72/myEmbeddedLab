#include <Arduino.h>
#include <stdint.h>

#define GPIO_ENABLE_REG  0x3FF44020
#define GPIO_IN_REG      0x3FF4403C
#define IO_MUX_GPIO5_REG 0x3FF4906C

static inline void tiny_delay(volatile uint32_t n) {
  while (n--) { }
}

void setup() {
  Serial.begin(115200);

  volatile uint32_t *gpio_en  = (uint32_t*)GPIO_ENABLE_REG;
  volatile uint32_t *gpio_in  = (uint32_t*)GPIO_IN_REG;
  volatile uint32_t *gpio5_config     = (uint32_t*)IO_MUX_GPIO5_REG;

  // Configure GPIO5 as GPIO function
  *gpio5_config &= ~(0x7u << 12);      // MCU_SEL = 0 (GPIO function)
  *gpio5_config |=  (1u << 9);         // FUN_IE = 1 (enable input buffer)
  *gpio5_config |=  (1u << 8);         // FUN_WPU = 1 (pull-up)
  *gpio5_config &= ~(1u << 7);         // FUN_WPD = 0 (no pull-down)

  // Ensure GPIO5 is input (disable output driver)
  *gpio_en &= ~(1u << 5);

  // Raw polling demo: print current level + edge events
  int last = ((*gpio_in) >> 5) & 1;

  while (1) {
    int raw = ((*gpio_in) >> 5) & 1;

    // Print only on change (edge), so Serial output is readable
    if (raw != last) {
      Serial.print("GPIO5 raw edge: ");
      Serial.print(last);
      Serial.print(" -> ");
      Serial.println(raw);
      last = raw;
    }

    // Small delay for UART 
    tiny_delay(50000);
  }
}

void loop() {}
