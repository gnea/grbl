/*
  spindle_control.c - spindle control methods
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"


#ifdef SPINDLE_MINIMUM_PWM
  #define SPINDLE_PWM_MIN_VALUE SPINDLE_MINIMUM_PWM
#else
  #define SPINDLE_PWM_MIN_VALUE 0
#endif
#define SPINDLE_PWM_RANGE (SPINDLE_PWM_MAX_VALUE-SPINDLE_PWM_MIN_VALUE)

#ifdef VARIABLE_SPINDLE
  static float pwm_gradient; // Precalulated value to speed up rpm to PWM conversions.
#endif


void spindle_init()
{
  #ifdef VARIABLE_SPINDLE

    // Configure variable spindle PWM and enable pin, if requried. On the Uno, PWM and enable are
    // combined unless configured otherwise.
    SPINDLE_PWM_DDR |= (1<<SPINDLE_PWM_BIT); // Configure as PWM output pin.
    SPINDLE_TCCRA_REGISTER = SPINDLE_TCCRA_INIT_MASK; // Configure PWM output compare timer
    SPINDLE_TCCRB_REGISTER = SPINDLE_TCCRB_INIT_MASK;
    #ifdef USE_SPINDLE_DIR_AS_ENABLE_PIN
      SPINDLE_ENABLE_DDR |= (1<<SPINDLE_ENABLE_BIT); // Configure as output pin.
    #else
      SPINDLE_DIRECTION_DDR |= (1<<SPINDLE_DIRECTION_BIT); // Configure as output pin.
    #endif

    pwm_gradient = SPINDLE_PWM_RANGE/(settings.rpm_max-settings.rpm_min);

  #else

    // Configure no variable spindle and only enable pin.
    SPINDLE_ENABLE_DDR |= (1<<SPINDLE_ENABLE_BIT); // Configure as output pin.
    SPINDLE_DIRECTION_DDR |= (1<<SPINDLE_DIRECTION_BIT); // Configure as output pin.

  #endif

  spindle_stop();
}


// Stop and start spindle routines. Called by all spindle routines and stepper ISR.
inline void spindle_stop()
{
  // On the Uno, spindle enable and PWM are shared. Other CPUs have seperate enable pin.
  #ifdef VARIABLE_SPINDLE
    SPINDLE_TCCRA_REGISTER &= ~(1<<SPINDLE_COMB_BIT); // Disable PWM. Output voltage is zero.
    #ifdef USE_SPINDLE_DIR_AS_ENABLE_PIN
      #ifdef INVERT_SPINDLE_ENABLE_PIN
        SPINDLE_ENABLE_PORT |= (1<<SPINDLE_ENABLE_BIT);  // Set pin to high
      #else
        SPINDLE_ENABLE_PORT &= ~(1<<SPINDLE_ENABLE_BIT); // Set pin to low
      #endif
    #endif
  #else
    #ifdef INVERT_SPINDLE_ENABLE_PIN
      SPINDLE_ENABLE_PORT |= (1<<SPINDLE_ENABLE_BIT);  // Set pin to high
    #else
      SPINDLE_ENABLE_PORT &= ~(1<<SPINDLE_ENABLE_BIT); // Set pin to low
    #endif
  #endif
}


#ifdef VARIABLE_SPINDLE
  inline void spindle_set_speed(uint8_t pwm_value)
  {
    if (pwm_value == SPINDLE_PWM_OFF_VALUE) {
      spindle_stop();
    } else {
      SPINDLE_OCR_REGISTER = pwm_value; // Set PWM output level.
      SPINDLE_TCCRA_REGISTER |= (1<<SPINDLE_COMB_BIT); // Ensure PWM output is enabled.

      #if defined(USE_SPINDLE_DIR_AS_ENABLE_PIN)
        #ifdef INVERT_SPINDLE_ENABLE_PIN
          SPINDLE_ENABLE_PORT &= ~(1<<SPINDLE_ENABLE_BIT);
        #else
          SPINDLE_ENABLE_PORT |= (1<<SPINDLE_ENABLE_BIT);
        #endif
      #endif

    }
  }


  uint8_t spindle_compute_pwm_value(float rpm) // 328p PWM register is 8-bit.
  {
    // Calculate PWM register value based on rpm max/min settings and programmed rpm.
    if ((settings.rpm_min >= settings.rpm_max) || (rpm > settings.rpm_max)) {
      // No PWM range possible. Set simple on/off spindle control pin state.
      return(SPINDLE_PWM_MAX_VALUE);
    } else if (rpm < settings.rpm_min) {
      if (rpm == 0.0) { return(SPINDLE_PWM_OFF_VALUE); }
      else { return(SPINDLE_PWM_MIN_VALUE); }
    } else {
      return(floor( (rpm-settings.rpm_min)*pwm_gradient + (SPINDLE_PWM_MIN_VALUE+0.5)));
    }
  }
#endif


// Immediately sets spindle running state with direction and spindle rpm via PWM, if enabled.
// Called by spindle_run() after sync and parking motion/spindle stop override during restore.
void spindle_set_state(uint8_t state, uint8_t pwm_value)
{
  if (sys.abort) { return; } // Block during abort.

  // Halt or set spindle direction and rpm.
  if (state == SPINDLE_DISABLE) {

    spindle_stop();

  } else {

    #ifndef USE_SPINDLE_DIR_AS_ENABLE_PIN
      if (state == SPINDLE_ENABLE_CW) {
        SPINDLE_DIRECTION_PORT &= ~(1<<SPINDLE_DIRECTION_BIT);
      } else {
        SPINDLE_DIRECTION_PORT |= (1<<SPINDLE_DIRECTION_BIT);
      }
    #endif

    #ifdef VARIABLE_SPINDLE

      spindle_set_speed(pwm_value);

    #else

      // NOTE: Without variable spindle, the enable bit should just turn on or off, regardless
      // if the spindle speed value is zero, as its ignored anyhow.
      #ifdef INVERT_SPINDLE_ENABLE_PIN
        SPINDLE_ENABLE_PORT &= ~(1<<SPINDLE_ENABLE_BIT);
      #else
        SPINDLE_ENABLE_PORT |= (1<<SPINDLE_ENABLE_BIT);
      #endif

    #endif

  }
}


// Called by g-code parser when setting spindle state and requires a buffer sync.
void spindle_run(uint8_t state, float rpm)
{
  if (sys.state == STATE_CHECK_MODE) { return; }
  protocol_buffer_synchronize(); // Empty planner buffer to ensure spindle is set when programmed.
  #ifdef VARIABLE_SPINDLE
    spindle_set_state(state, spindle_compute_pwm_value(rpm));
  #else
    spindle_set_state(state,0); // Send null pwm value. Not used.
  #endif
}
