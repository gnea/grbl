/*
  limits.c - code pertaining to limit-switches and performing the homing cycle
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


// Homing axis search distance multiplier. Computed by this value times the cycle travel.
#ifndef HOMING_AXIS_SEARCH_SCALAR
  #define HOMING_AXIS_SEARCH_SCALAR  1.5 // Must be > 1 to ensure limit switch will be engaged.
#endif
#ifndef HOMING_AXIS_LOCATE_SCALAR
  #define HOMING_AXIS_LOCATE_SCALAR  5.0 // Must be > 1 to ensure limit switch is cleared.
#endif

#ifdef ENABLE_DUAL_AXIS
  // Flags for dual axis async limit trigger check.
  #define DUAL_AXIS_CHECK_DISABLE     0  // Must be zero
  #define DUAL_AXIS_CHECK_ENABLE      bit(0)
  #define DUAL_AXIS_CHECK_TRIGGER_1   bit(1)
  #define DUAL_AXIS_CHECK_TRIGGER_2   bit(2)
#endif

void limits_init()
{
  LIMIT_DDR &= ~(LIMIT_MASK); // Set as input pins

  #ifdef DISABLE_LIMIT_PIN_PULL_UP
    LIMIT_PORT &= ~(LIMIT_MASK); // Normal low operation. Requires external pull-down.
  #else
    LIMIT_PORT |= (LIMIT_MASK);  // Enable internal pull-up resistors. Normal high operation.
  #endif

  if (bit_istrue(settings.flags,BITFLAG_HARD_LIMIT_ENABLE)) {
    LIMIT_PCMSK |= LIMIT_MASK; // Enable specific pins of the Pin Change Interrupt
    PCICR |= (1 << LIMIT_INT); // Enable Pin Change Interrupt
  } else {
    limits_disable();
  }

  #ifdef ENABLE_SOFTWARE_DEBOUNCE
    MCUSR &= ~(1<<WDRF);
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR = (1<<WDP0); // Set time-out at ~32msec.
  #endif
}


// Disables hard limits.
void limits_disable()
{
  LIMIT_PCMSK &= ~LIMIT_MASK;  // Disable specific pins of the Pin Change Interrupt
  PCICR &= ~(1 << LIMIT_INT);  // Disable Pin Change Interrupt
}


// Returns limit state as a bit-wise uint8 variable. Each bit indicates an axis limit, where
// triggered is 1 and not triggered is 0. Invert mask is applied. Axes are defined by their
// number in bit position, i.e. Z_AXIS is (1<<2) or bit 2, and Y_AXIS is (1<<1) or bit 1.
uint8_t limits_get_state()
{
  uint8_t limit_state = 0;
  uint8_t pin = (LIMIT_PIN & LIMIT_MASK);
  #ifdef INVERT_LIMIT_PIN_MASK
    pin ^= INVERT_LIMIT_PIN_MASK;
  #endif
  if (bit_isfalse(settings.flags,BITFLAG_INVERT_LIMIT_PINS)) { pin ^= LIMIT_MASK; }
  if (pin) {
    uint8_t idx;
    for (idx=0; idx<N_AXIS; idx++) {
      if (pin & get_limit_pin_mask(idx)) { limit_state |= (1 << idx); }
    }
    #ifdef ENABLE_DUAL_AXIS
      if (pin & (1<<DUAL_LIMIT_BIT)) { limit_state |= (1 << N_AXIS); }
    #endif
  }
  return(limit_state);
}


// This is the Limit Pin Change Interrupt, which handles the hard limit feature. A bouncing
// limit switch can cause a lot of problems, like false readings and multiple interrupt calls.
// If a switch is triggered at all, something bad has happened and treat it as such, regardless
// if a limit switch is being disengaged. It's impossible to reliably tell the state of a
// bouncing pin because the Arduino microcontroller does not retain any state information when
// detecting a pin change. If we poll the pins in the ISR, you can miss the correct reading if the 
// switch is bouncing.
// NOTE: Do not attach an e-stop to the limit pins, because this interrupt is disabled during
// homing cycles and will not respond correctly. Upon user request or need, there may be a
// special pinout for an e-stop, but it is generally recommended to just directly connect
// your e-stop switch to the Arduino reset pin, since it is the most correct way to do this.
#ifndef ENABLE_SOFTWARE_DEBOUNCE
  ISR(LIMIT_INT_vect) // DEFAULT: Limit pin change interrupt process.
  {
    // Ignore limit switches if already in an alarm state or in-process of executing an alarm.
    // When in the alarm state, Grbl should have been reset or will force a reset, so any pending
    // moves in the planner and serial buffers are all cleared and newly sent blocks will be
    // locked out until a homing cycle or a kill lock command. Allows the user to disable the hard
    // limit setting if their limits are constantly triggering after a reset and move their axes.
    if (sys.state != STATE_ALARM) {
      if (!(sys_rt_exec_alarm)) {
        #ifdef HARD_LIMIT_FORCE_STATE_CHECK
          // Check limit pin state.
          if (limits_get_state()) {
            mc_reset(); // Initiate system kill.
            system_set_exec_alarm(EXEC_ALARM_HARD_LIMIT); // Indicate hard limit critical event
          }
        #else
          mc_reset(); // Initiate system kill.
          system_set_exec_alarm(EXEC_ALARM_HARD_LIMIT); // Indicate hard limit critical event
        #endif
      }
    }
  }
#else // OPTIONAL: Software debounce limit pin routine.
  // Upon limit pin change, enable watchdog timer to create a short delay. 
  ISR(LIMIT_INT_vect) { if (!(WDTCSR & (1<<WDIE))) { WDTCSR |= (1<<WDIE); } }
  ISR(WDT_vect) // Watchdog timer ISR
  {
    WDTCSR &= ~(1<<WDIE); // Disable watchdog timer. 
    if (sys.state != STATE_ALARM) {  // Ignore if already in alarm state. 
      if (!(sys_rt_exec_alarm)) {
        // Check limit pin state. 
        if (limits_get_state()) {
          mc_reset(); // Initiate system kill.
          system_set_exec_alarm(EXEC_ALARM_HARD_LIMIT); // Indicate hard limit critical event
        }
      }  
    }
  }
#endif

// Homes the specified cycle axes, sets the machine position, and performs a pull-off motion after
// completing. Homing is a special motion case, which involves rapid uncontrolled stops to locate
// the trigger point of the limit switches. The rapid stops are handled by a system level axis lock
// mask, which prevents the stepper algorithm from executing step pulses. Homing motions typically
// circumvent the processes for executing motions in normal operation.
// NOTE: Only the abort realtime command can interrupt this process.
// TODO: Move limit pin-specific calls to a general function for portability.
void limits_go_home(uint8_t cycle_mask)
{
  if (sys.abort) { return; } // Block if system reset has been issued.

  // Initialize plan data struct for homing motion. Spindle and coolant are disabled.
  plan_line_data_t plan_data;
  plan_line_data_t *pl_data = &plan_data;
  memset(pl_data,0,sizeof(plan_line_data_t));
  pl_data->condition = (PL_COND_FLAG_SYSTEM_MOTION|PL_COND_FLAG_NO_FEED_OVERRIDE);
  #ifdef USE_LINE_NUMBERS
    pl_data->line_number = HOMING_CYCLE_LINE_NUMBER;
  #endif

  // Initialize variables used for homing computations.
  uint8_t n_cycle = (2*N_HOMING_LOCATE_CYCLE+1);
  uint8_t step_pin[N_AXIS];
  #ifdef ENABLE_DUAL_AXIS
    uint8_t step_pin_dual;
    uint8_t dual_axis_async_check;
    int32_t dual_trigger_position;
    #if (DUAL_AXIS_SELECT == X_AXIS)
      float fail_distance = (-DUAL_AXIS_HOMING_FAIL_AXIS_LENGTH_PERCENT/100.0)*settings.max_travel[Y_AXIS];
    #else
      float fail_distance = (-DUAL_AXIS_HOMING_FAIL_AXIS_LENGTH_PERCENT/100.0)*settings.max_travel[X_AXIS];
    #endif
    fail_distance = min(fail_distance, DUAL_AXIS_HOMING_FAIL_DISTANCE_MAX);
    fail_distance = max(fail_distance, DUAL_AXIS_HOMING_FAIL_DISTANCE_MIN);
    int32_t dual_fail_distance = trunc(fail_distance*settings.steps_per_mm[DUAL_AXIS_SELECT]);
    // int32_t dual_fail_distance = trunc((DUAL_AXIS_HOMING_TRIGGER_FAIL_DISTANCE)*settings.steps_per_mm[DUAL_AXIS_SELECT]);
  #endif
  float target[N_AXIS];
  float max_travel = 0.0;
  uint8_t idx;
  for (idx=0; idx<N_AXIS; idx++) {
    // Initialize step pin masks
    step_pin[idx] = get_step_pin_mask(idx);
    #ifdef COREXY
      if ((idx==A_MOTOR)||(idx==B_MOTOR)) { step_pin[idx] = (get_step_pin_mask(X_AXIS)|get_step_pin_mask(Y_AXIS)); }
    #endif

    if (bit_istrue(cycle_mask,bit(idx))) {
      // Set target based on max_travel setting. Ensure homing switches engaged with search scalar.
      // NOTE: settings.max_travel[] is stored as a negative value.
      max_travel = max(max_travel,(-HOMING_AXIS_SEARCH_SCALAR)*settings.max_travel[idx]);
    }
  }
  #ifdef ENABLE_DUAL_AXIS
    step_pin_dual = (1<<DUAL_STEP_BIT);
  #endif

  // Set search mode with approach at seek rate to quickly engage the specified cycle_mask limit switches.
  bool approach = true;
  float homing_rate = settings.homing_seek_rate;

  uint8_t limit_state, axislock, n_active_axis;
  do {

    system_convert_array_steps_to_mpos(target,sys_position);

    // Initialize and declare variables needed for homing routine.
    axislock = 0;
    #ifdef ENABLE_DUAL_AXIS
      sys.homing_axis_lock_dual = 0;
      dual_trigger_position = 0;
      dual_axis_async_check = DUAL_AXIS_CHECK_DISABLE;
    #endif
    n_active_axis = 0;
    for (idx=0; idx<N_AXIS; idx++) {
      // Set target location for active axes and setup computation for homing rate.
      if (bit_istrue(cycle_mask,bit(idx))) {
        n_active_axis++;
        #ifdef COREXY
          if (idx == X_AXIS) {
            int32_t axis_position = system_convert_corexy_to_y_axis_steps(sys_position);
            sys_position[A_MOTOR] = axis_position;
            sys_position[B_MOTOR] = -axis_position;
          } else if (idx == Y_AXIS) {
            int32_t axis_position = system_convert_corexy_to_x_axis_steps(sys_position);
            sys_position[A_MOTOR] = sys_position[B_MOTOR] = axis_position;
          } else {
            sys_position[Z_AXIS] = 0;
          }
        #else
          sys_position[idx] = 0;
        #endif
        // Set target direction based on cycle mask and homing cycle approach state.
        // NOTE: This happens to compile smaller than any other implementation tried.
        if (bit_istrue(settings.homing_dir_mask,bit(idx))) {
          if (approach) { target[idx] = -max_travel; }
          else { target[idx] = max_travel; }
        } else {
          if (approach) { target[idx] = max_travel; }
          else { target[idx] = -max_travel; }
        }
        // Apply axislock to the step port pins active in this cycle.
        axislock |= step_pin[idx];
        #ifdef ENABLE_DUAL_AXIS
          if (idx == DUAL_AXIS_SELECT) { sys.homing_axis_lock_dual = step_pin_dual; }
        #endif
      }

    }
    homing_rate *= sqrt(n_active_axis); // [sqrt(N_AXIS)] Adjust so individual axes all move at homing rate.
    sys.homing_axis_lock = axislock;

    // Perform homing cycle. Planner buffer should be empty, as required to initiate the homing cycle.
    pl_data->feed_rate = homing_rate; // Set current homing rate.
    plan_buffer_line(target, pl_data); // Bypass mc_line(). Directly plan homing motion.

    sys.step_control = STEP_CONTROL_EXECUTE_SYS_MOTION; // Set to execute homing motion and clear existing flags.
    st_prep_buffer(); // Prep and fill segment buffer from newly planned block.
    st_wake_up(); // Initiate motion
    do {
      if (approach) {
        // Check limit state. Lock out cycle axes when they change.
        limit_state = limits_get_state();
        for (idx=0; idx<N_AXIS; idx++) {
          if (axislock & step_pin[idx]) {
            if (limit_state & (1 << idx)) {
              #ifdef COREXY
                if (idx==Z_AXIS) { axislock &= ~(step_pin[Z_AXIS]); }
                else { axislock &= ~(step_pin[A_MOTOR]|step_pin[B_MOTOR]); }
              #else
                axislock &= ~(step_pin[idx]);
                #ifdef ENABLE_DUAL_AXIS
                  if (idx == DUAL_AXIS_SELECT) { dual_axis_async_check |= DUAL_AXIS_CHECK_TRIGGER_1; }
                #endif
              #endif
            }
          }
        }
        sys.homing_axis_lock = axislock;
        #ifdef ENABLE_DUAL_AXIS
          if (sys.homing_axis_lock_dual) { // NOTE: Only true when homing dual axis.
            if (limit_state & (1 << N_AXIS)) { 
              sys.homing_axis_lock_dual = 0;
              dual_axis_async_check |= DUAL_AXIS_CHECK_TRIGGER_2;
            }
          }
          
          // When first dual axis limit triggers, record position and begin checking distance until other limit triggers. Bail upon failure.
          if (dual_axis_async_check) {
            if (dual_axis_async_check & DUAL_AXIS_CHECK_ENABLE) {
              if (( dual_axis_async_check &  (DUAL_AXIS_CHECK_TRIGGER_1 | DUAL_AXIS_CHECK_TRIGGER_2)) == (DUAL_AXIS_CHECK_TRIGGER_1 | DUAL_AXIS_CHECK_TRIGGER_2)) {
                dual_axis_async_check = DUAL_AXIS_CHECK_DISABLE;
              } else {
                if (abs(dual_trigger_position - sys_position[DUAL_AXIS_SELECT]) > dual_fail_distance) {
                  system_set_exec_alarm(EXEC_ALARM_HOMING_FAIL_DUAL_APPROACH);
                  mc_reset();
                  protocol_execute_realtime();
                  return;
                }
              }
            } else {
              dual_axis_async_check |= DUAL_AXIS_CHECK_ENABLE;
              dual_trigger_position = sys_position[DUAL_AXIS_SELECT];
            }
          }
        #endif
      }

      st_prep_buffer(); // Check and prep segment buffer. NOTE: Should take no longer than 200us.

      // Exit routines: No time to run protocol_execute_realtime() in this loop.
      if (sys_rt_exec_state & (EXEC_SAFETY_DOOR | EXEC_RESET | EXEC_CYCLE_STOP)) {
        uint8_t rt_exec = sys_rt_exec_state;
        // Homing failure condition: Reset issued during cycle.
        if (rt_exec & EXEC_RESET) { system_set_exec_alarm(EXEC_ALARM_HOMING_FAIL_RESET); }
        // Homing failure condition: Safety door was opened.
        if (rt_exec & EXEC_SAFETY_DOOR) { system_set_exec_alarm(EXEC_ALARM_HOMING_FAIL_DOOR); }
        // Homing failure condition: Limit switch still engaged after pull-off motion
        if (!approach && (limits_get_state() & cycle_mask)) { system_set_exec_alarm(EXEC_ALARM_HOMING_FAIL_PULLOFF); }
        // Homing failure condition: Limit switch not found during approach.
        if (approach && (rt_exec & EXEC_CYCLE_STOP)) { system_set_exec_alarm(EXEC_ALARM_HOMING_FAIL_APPROACH); }
        if (sys_rt_exec_alarm) {
          mc_reset(); // Stop motors, if they are running.
          protocol_execute_realtime();
          return;
        } else {
          // Pull-off motion complete. Disable CYCLE_STOP from executing.
          system_clear_exec_state_flag(EXEC_CYCLE_STOP);
          break;
        }
      }

    #ifdef ENABLE_DUAL_AXIS
      } while ((STEP_MASK & axislock) || (sys.homing_axis_lock_dual));
    #else
      } while (STEP_MASK & axislock);
    #endif

    st_reset(); // Immediately force kill steppers and reset step segment buffer.
    delay_ms(settings.homing_debounce_delay); // Delay to allow transient dynamics to dissipate.

    // Reverse direction and reset homing rate for locate cycle(s).
    approach = !approach;

    // After first cycle, homing enters locating phase. Shorten search to pull-off distance.
    if (approach) {
      max_travel = settings.homing_pulloff*HOMING_AXIS_LOCATE_SCALAR;
      homing_rate = settings.homing_feed_rate;
    } else {
      max_travel = settings.homing_pulloff;
      homing_rate = settings.homing_seek_rate;
    }

  } while (n_cycle-- > 0);

  // The active cycle axes should now be homed and machine limits have been located. By
  // default, Grbl defines machine space as all negative, as do most CNCs. Since limit switches
  // can be on either side of an axes, check and set axes machine zero appropriately. Also,
  // set up pull-off maneuver from axes limit switches that have been homed. This provides
  // some initial clearance off the switches and should also help prevent them from falsely
  // triggering when hard limits are enabled or when more than one axes shares a limit pin.
  int32_t set_axis_position;
  // Set machine positions for homed limit switches. Don't update non-homed axes.
  for (idx=0; idx<N_AXIS; idx++) {
    // NOTE: settings.max_travel[] is stored as a negative value.
    if (cycle_mask & bit(idx)) {
      #ifdef HOMING_FORCE_SET_ORIGIN
        set_axis_position = 0;
      #else
        if ( bit_istrue(settings.homing_dir_mask,bit(idx)) ) {
          set_axis_position = lround((settings.max_travel[idx]+settings.homing_pulloff)*settings.steps_per_mm[idx]);
        } else {
          set_axis_position = lround(-settings.homing_pulloff*settings.steps_per_mm[idx]);
        }
      #endif

      #ifdef COREXY
        if (idx==X_AXIS) {
          int32_t off_axis_position = system_convert_corexy_to_y_axis_steps(sys_position);
          sys_position[A_MOTOR] = set_axis_position + off_axis_position;
          sys_position[B_MOTOR] = set_axis_position - off_axis_position;
        } else if (idx==Y_AXIS) {
          int32_t off_axis_position = system_convert_corexy_to_x_axis_steps(sys_position);
          sys_position[A_MOTOR] = off_axis_position + set_axis_position;
          sys_position[B_MOTOR] = off_axis_position - set_axis_position;
        } else {
          sys_position[idx] = set_axis_position;
        }
      #else
        sys_position[idx] = set_axis_position;
      #endif

    }
  }
  sys.step_control = STEP_CONTROL_NORMAL_OP; // Return step control to normal operation.
}


// Performs a soft limit check. Called from mc_line() only. Assumes the machine has been homed,
// the workspace volume is in all negative space, and the system is in normal operation.
// NOTE: Used by jogging to limit travel within soft-limit volume.
void limits_soft_check(float *target)
{
  if (system_check_travel_limits(target)) {
    sys.soft_limit = true;
    // Force feed hold if cycle is active. All buffered blocks are guaranteed to be within
    // workspace volume so just come to a controlled stop so position is not lost. When complete
    // enter alarm mode.
    if (sys.state == STATE_CYCLE) {
      system_set_exec_state_flag(EXEC_FEED_HOLD);
      do {
        protocol_execute_realtime();
        if (sys.abort) { return; }
      } while ( sys.state != STATE_IDLE );
    }
    mc_reset(); // Issue system reset and ensure spindle and coolant are shutdown.
    system_set_exec_alarm(EXEC_ALARM_SOFT_LIMIT); // Indicate soft limit critical event
    protocol_execute_realtime(); // Execute to enter critical event loop and system abort
    return;
  }
}
