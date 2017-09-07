/*

    Power control.

    THEORY OF OPERATION
    ===================

    We put the MCU into "powerdown" mode to save batteries when the unit is not in use.
    
    The sleep with timeout works by enabling the watchdog timer to generate an interrupt after the specified delay.


*/


#include "blinks.h"
#include "hardware.h"

#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "utils.h"
#include "power.h"

// Need to redo the enum here becuase multipule enums break Arduino IDE so we made extern in power.h


// Don't do anything, just the interrupt itself will wake us up
// TODO: If we ever need more code space, this could be replaced by an IRET in the VECTOR table.

EMPTY_INTERRUPT( WDT_vect );
      
// Goto low power sleep - get woken up by button or IR LED 
// Be sure to turn off all pixels before sleeping since
// the PWM timers will not be running so pixels will not look right.
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

// TODO: We should probably ALWAYS sleep with timers on between frames to save CPU power. 

void powerdown(bool wakeOnbutton,  uint8_t wakeOnIR_bitmask  ) {
    
    #ifdef DEBUG_CHECKS
    
        if (wakeOnIR_bitmask & ~IR_BITS) {
            
            DEBUG_ERROR( DEBUG_WAKEBITS ); 
            
        }            
    
    #endif
    
    if (!wakeOnbutton) {
        CBI( BUTTON_MASK , BUTTON_PCINT );        // Clear button bit in pin change mask so it does not generate an interrupt. 
    }
        
    IR_MASK &= wakeOnIR_bitmask;                 // Clear any pin change mask bits for IR face we don't care about        
             
    sleep_cpu();        // Good night
        
    IR_MASK = IR_PCINT;                           // Restore IR interrupts
    SBI( BUTTON_MASK , BUTTON_PCINT );            // Restore button interrupt
    
}

// Sleep with a predefined timeout. 
// This is very power efficient since chip is stopped except for WDT
// If wakeOnButton is true, then we will wake if the button changes (up or down)
// Each bit in wakeOnIR_bitmask represents the IR sensor on one face. If the bit is 1
// then we will wake when there is a change on that face

void powerdownWithTimeout( bool wakeOnbutton, uint8_t wakeOnIR_bitmask , sleepTimeoutType timeout ) {
    
    wdt_reset();
    WDTCSR =   timeout;              // Enable WDT Interrupt  (WDIE and timeout bits all included in the timeout values)
    
    powerdown(wakeOnbutton,wakeOnIR_bitmask);
    
    WDTCSR = 0;                      // Turn off the WDT interrupt (no special sequence needed here)
                                     // (assigning all bits to zero is 1 instruction and we don't care about the other bits getting clobbered
    
}

void sleep_init(void) {

    // Could save a byte here by combining these two to a single assign
    
    // I know datasheet repeatedly states you should only set Sleep Enable just before sleeping to avoid
    // "accidentally" sleeping, but the only way to sleep is to execute the "sleep" instruction
    // so if you are somehow executing that instruction at a time when you don't mean to sleep, then you
    // have bigger problems to worry about.
    
    // SIZE: These could be combined to save a few bytes
    set_sleep_mode( SLEEP_MODE_PWR_DOWN );
    sleep_enable();

    
}    
