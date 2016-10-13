/*
  coolant_control.c - coolant control methods
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

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


void coolant_init()
{
  COOLANT_FLOOD_DDR |= (1 << COOLANT_FLOOD_BIT); // Configure as output pin
  #ifdef ENABLE_M7
    COOLANT_MIST_DDR |= (1 << COOLANT_MIST_BIT);
  #endif
  coolant_set_state(COOLANT_DISABLE);
}


void coolant_set_state(uint8_t mode)
{
  if (mode & COOLANT_FLOOD_ENABLE) {
    #ifdef INVERT_COOLANT_FLOOD_PIN
      COOLANT_FLOOD_PORT &= ~(1 << COOLANT_FLOOD_BIT);
    #else
      COOLANT_FLOOD_PORT |= (1 << COOLANT_FLOOD_BIT);
    #endif
  } else {
    #ifdef INVERT_COOLANT_FLOOD_PIN
      COOLANT_FLOOD_PORT |= (1 << COOLANT_FLOOD_BIT);
    #else
      COOLANT_FLOOD_PORT &= ~(1 << COOLANT_FLOOD_BIT);
    #endif
  }
  #ifdef ENABLE_M7
    if (mode & COOLANT_MIST_ENABLE) {
      #ifdef INVERT_COOLANT_MIST_PIN
        COOLANT_MIST_PORT &= ~(1 << COOLANT_MIST_BIT);
      #else
        COOLANT_MIST_PORT |= (1 << COOLANT_MIST_BIT);
      #endif
    } else {
      #ifdef INVERT_COOLANT_MIST_PIN
        COOLANT_MIST_PORT |= (1 << COOLANT_MIST_BIT);
      #else
        COOLANT_MIST_PORT &= ~(1 << COOLANT_MIST_BIT);
      #endif
    }
  #endif
}


void coolant_run(uint8_t mode)
{
  if (sys.state == STATE_CHECK_MODE) { return; }
  protocol_buffer_synchronize(); // Ensure coolant turns on when specified in program.
  if (sys.abort) { return; } // Block during abort.
  coolant_set_state(mode);
}
