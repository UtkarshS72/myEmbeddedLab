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



/*
INTERRUPT-DRIVEN DEBOUNCE 

The program has 2 independent execution contexts:

1) main loop --> normal program flow
2) ISR       --> asynchronous hardware event (button edge)

The ISR can interrupt the CPU at ANY instruction boundary.
It is not part of the normal program order.

Because both contexts access the same variables, special handling
is required to prevent race conditions and inconsistent reads.
*/

/*
IRAM_ATTR:

On ESP32 most code executes from external flash through a cache.
During certain hardware events the flash cache may be temporarily disabled.

If an interrupt occurs while the cache is disabled and the ISR lives in flash,
the CPU cannot fetch instructions --> crash/reset.

IRAM_ATTR forces the ISR code into internal instruction RAM (IRAM),
so the interrupt can always execute safely.
*/

/*
volatile shared variables:

Variables modified inside an ISR MUST be declared volatile.

Reason:
The compiler normally assumes memory does not change unless the program
writes to it. It may cache values in registers and reuse them.

An ISR changes memory outside the normal program flow.
Without volatile, the compiler may reuse a stale register value and
the main program will never observe the change.

volatile tells the compiler:
"this memory may change at any time — always reread it from memory".
*/

/*
Atomic snapshot (critical section):

The ISR can update shared variables at any time.
If the main loop reads them while an interrupt occurs, it may read
half old and half new values --> race condition.

Therefore we briefly disable interrupts while copying shared variables
into local variables.

We do NOT keep interrupts disabled during processing.
We only disable them long enough to take a consistent snapshot.
*/

/*
Why copy to local variables?

After interrupts are re-enabled, the ISR may run again immediately.

The program must operate on a stable moment in time, not a moving target.

Local copies represent:
"the exact state of the system at one instant".

All debounce decisions are made using the snapshot, not the live variables.
*/

/*
ISR design rule:

The ISR does NOT implement debounce logic.
It does NOT toggle the LED.
It does NOT perform long calculations.

The ISR only records:
    - the raw pin level
    - the time the change occurred

Processing is deferred to the main loop.

Reason:
ISRs must be short, deterministic, and non-blocking.
*/
