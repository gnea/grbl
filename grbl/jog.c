/*
  jog.h - Jogging methods
  Part of Grbl

  Copyright (c) 2016 Sungeun K. Jeon for Gnea Research LLC

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


// Sets up valid jog motion received from g-code parser, checks for soft-limits, and executes the jog.
uint8_t jog_execute(plan_line_data_t *pl_data, parser_block_t *gc_block)
{
  // Initialize planner data struct for jogging motions.
  // NOTE: Spindle and coolant are allowed to fully function with overrides during a jog.
  pl_data->feed_rate = gc_block->values.f;
  pl_data->condition |= PL_COND_FLAG_NO_FEED_OVERRIDE;
  #ifdef USE_LINE_NUMBERS
    pl_data->line_number = gc_block->values.n;
  #endif

  if (bit_istrue(settings.flags,BITFLAG_SOFT_LIMIT_ENABLE)) {
    if (system_check_travel_limits(gc_block->values.xyz)) { return(STATUS_TRAVEL_EXCEEDED); }
  }

  // ---- ALOE PATCH: hard-limit direction guard for jog ----
  // Block jog motion toward a pressed hard-limit switch BEFORE mc_line().
  // Returns an error status, so gcode.c does NOT memcpy the parser position
  // (gc_state.position) -- otherwise the parser drifts ahead of sys_position
  // and the reverse-direction jog appears "stuck" for several clicks.
  {
    uint8_t limit_state = limits_get_state();
    sys.limit_trip_mask &= limit_state; // Forget latched direction for any released switch.
    if (limit_state) {
      float current_mpos[N_AXIS];
      system_convert_array_steps_to_mpos(current_mpos, sys_position);
      uint8_t idx;
      for (idx=0; idx<N_AXIS; idx++) {
        if (limit_state & (1<<idx)) {
          // ALOE PATCH: Block the direction latched at the moment this axis tripped (see
          // the limit ISR and mc_line()). Falls back to the $23 homing side when the switch
          // tripped without motion (e.g. pressed by hand at standstill).
          uint8_t block_positive;
          if (sys.limit_trip_mask & bit(idx)) {
            block_positive = !bit_istrue(sys.limit_trip_dir,bit(idx)); // Latched bit set = was moving negative.
          } else if (bit_istrue(settings.homing_dir_mask,bit(idx))) {
            block_positive = false; // Homing moves negative -> homing switch is on the minus side.
          } else {
            block_positive = true;  // Homing moves positive -> homing switch is on the plus side.
          }
          if (block_positive) {
            // Switch on plus side: block positive jog.
            if (gc_block->values.xyz[idx] > current_mpos[idx]) { return(STATUS_TRAVEL_EXCEEDED); }
          } else {
            // Switch on minus side: block negative jog.
            if (gc_block->values.xyz[idx] < current_mpos[idx]) { return(STATUS_TRAVEL_EXCEEDED); }
          }
        }
      }
    }
  }
  // ---- END PATCH ----

  // Valid jog command. Plan, set state, and execute.
  mc_line(gc_block->values.xyz,pl_data);
  if (sys.state == STATE_IDLE) {
    if (plan_get_current_block() != NULL) { // Check if there is a block to execute.
      sys.state = STATE_JOG;
      st_prep_buffer();
      st_wake_up();  // NOTE: Manual start. No state machine required.
    }
  }

  return(STATUS_OK);
}
