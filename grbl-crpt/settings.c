/*
  settings.c - eeprom configuration handling
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c) 2026 Alan Mansour, DALSA

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

#include "grbl-crpt.h"

settings_t settings;

typedef struct {
  float steps_per_mm[N_AXIS];
  float max_rate[N_AXIS];
  float acceleration[N_AXIS];
  float max_travel[N_AXIS];
  uint8_t pulse_microseconds;
  uint8_t step_invert_mask;
  uint8_t dir_invert_mask;
  uint8_t stepper_idle_lock_time;
  uint8_t status_report_mask;
  float junction_deviation;
  float arc_tolerance;
  float rpm_max;
  float rpm_min;
  uint8_t flags;
  uint8_t homing_dir_mask;
  uint8_t homing_origin_max_mask;
  float homing_feed_rate;
  float homing_seek_rate;
  uint16_t homing_debounce_delay;
  float homing_pulloff[N_AXIS];
} settings_v11_t;

typedef struct {
  float steps_per_mm[N_AXIS];
  float max_rate[N_AXIS];
  float acceleration[N_AXIS];
  float max_travel[N_AXIS];
  uint8_t pulse_microseconds;
  uint8_t step_invert_mask;
  uint8_t dir_invert_mask;
  uint8_t stepper_idle_lock_time;
  uint8_t status_report_mask;
  float junction_deviation;
  float arc_tolerance;
  float rpm_max;
  float rpm_min;
  uint8_t flags;
  uint8_t homing_dir_mask;
  float homing_feed_rate;
  float homing_seek_rate;
  uint16_t homing_debounce_delay;
  float homing_pulloff;
} settings_v10_t;

const __flash settings_t defaults = {\
    .pulse_microseconds = DEFAULT_STEP_PULSE_MICROSECONDS,
    .stepper_idle_lock_time = DEFAULT_STEPPER_IDLE_LOCK_TIME,
    .step_invert_mask = DEFAULT_STEPPING_INVERT_MASK,
    .dir_invert_mask = DEFAULT_DIRECTION_INVERT_MASK,
    .status_report_mask = DEFAULT_STATUS_REPORT_MASK,
    .junction_deviation = DEFAULT_JUNCTION_DEVIATION,
    .arc_tolerance = DEFAULT_ARC_TOLERANCE,
    .homing_dir_mask = DEFAULT_HOMING_DIR_MASK,
    .homing_feed_rate = DEFAULT_HOMING_FEED_RATE,
    .homing_seek_rate = DEFAULT_HOMING_SEEK_RATE,
    .homing_debounce_delay = DEFAULT_HOMING_DEBOUNCE_DELAY,
    .homing_origin_max_mask = DEFAULT_HOMING_ORIGIN_MAX_MASK,
    .homing_pulloff[X_AXIS] = DEFAULT_X_HOMING_PULLOFF,
    .homing_pulloff[Y_AXIS] = DEFAULT_Y_HOMING_PULLOFF,
    .homing_pulloff[Z_AXIS] = DEFAULT_Z_HOMING_PULLOFF,
    .homing_pulloff[A_AXIS] = DEFAULT_A_HOMING_PULLOFF,
    .flags = (DEFAULT_REPORT_INCHES << BIT_REPORT_INCHES) | \
             (DEFAULT_INVERT_ST_ENABLE << BIT_INVERT_ST_ENABLE) | \
             (DEFAULT_HARD_LIMIT_ENABLE << BIT_HARD_LIMIT_ENABLE) | \
             (DEFAULT_HOMING_ENABLE << BIT_HOMING_ENABLE) | \
             (DEFAULT_SOFT_LIMIT_ENABLE << BIT_SOFT_LIMIT_ENABLE) | \
             (DEFAULT_INVERT_LIMIT_PINS << BIT_INVERT_LIMIT_PINS),
    .steps_per_mm[X_AXIS] = DEFAULT_X_STEPS_PER_MM,
    .steps_per_mm[Y_AXIS] = DEFAULT_Y_STEPS_PER_MM,
    .steps_per_mm[Z_AXIS] = DEFAULT_Z_STEPS_PER_MM,
      .steps_per_mm[A_AXIS] = DEFAULT_A_STEPS_PER_DEGREE,
    .max_rate[X_AXIS] = DEFAULT_X_MAX_RATE,
    .max_rate[Y_AXIS] = DEFAULT_Y_MAX_RATE,
    .max_rate[Z_AXIS] = DEFAULT_Z_MAX_RATE,
      .max_rate[A_AXIS] = DEFAULT_A_MAX_RATE,
    .acceleration[X_AXIS] = DEFAULT_X_ACCELERATION,
    .acceleration[Y_AXIS] = DEFAULT_Y_ACCELERATION,
    .acceleration[Z_AXIS] = DEFAULT_Z_ACCELERATION,
      .acceleration[A_AXIS] = DEFAULT_A_ACCELERATION,
    .max_travel[X_AXIS] = (-DEFAULT_X_MAX_TRAVEL),
    .max_travel[Y_AXIS] = (-DEFAULT_Y_MAX_TRAVEL),
    .max_travel[Z_AXIS] = (-DEFAULT_Z_MAX_TRAVEL),
    .max_travel[A_AXIS] = (-DEFAULT_A_MAX_TRAVEL)};


// Method to store startup lines into EEPROM
void settings_store_startup_line(uint8_t n, char *line)
{
  #ifdef FORCE_BUFFER_SYNC_DURING_EEPROM_WRITE
    protocol_buffer_synchronize(); // A startup line may contain a motion and be executing. 
  #endif
  uint32_t addr = n*(LINE_BUFFER_SIZE+1)+EEPROM_ADDR_STARTUP_BLOCK;
  memcpy_to_eeprom_with_checksum(addr,(char*)line, LINE_BUFFER_SIZE);
}


// Method to store build info into EEPROM
// NOTE: This function can only be called in IDLE state.
void settings_store_build_info(char *line)
{
  // Build info can only be stored when state is IDLE.
  memcpy_to_eeprom_with_checksum(EEPROM_ADDR_BUILD_INFO,(char*)line, LINE_BUFFER_SIZE);
}


// Method to store coord data parameters into EEPROM
void settings_write_coord_data(uint8_t coord_select, float *coord_data)
{
  #ifdef FORCE_BUFFER_SYNC_DURING_EEPROM_WRITE
    protocol_buffer_synchronize();
  #endif
  uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
  memcpy_to_eeprom_with_checksum(addr,(char*)coord_data, sizeof(float)*N_AXIS);
}


// Method to store GRBL-CRPT global settings struct and version number into EEPROM
// NOTE: This function can only be called in IDLE state.
void write_global_settings()
{
  eeprom_put_char(0, SETTINGS_VERSION);
  memcpy_to_eeprom_with_checksum(EEPROM_ADDR_GLOBAL, (char*)&settings, sizeof(settings_t));
}


// Method to restore EEPROM-saved GRBL-CRPT global settings back to defaults.
void settings_restore(uint8_t restore_flag) {
  if (restore_flag & SETTINGS_RESTORE_DEFAULTS) {    
    settings = defaults;
    write_global_settings();
  }

  if (restore_flag & SETTINGS_RESTORE_PARAMETERS) {
    uint8_t idx;
    float coord_data[N_AXIS];
    memset(&coord_data, 0, sizeof(coord_data));
    for (idx=0; idx <= SETTING_INDEX_NCOORD; idx++) { settings_write_coord_data(idx, coord_data); }
  }

  if (restore_flag & SETTINGS_RESTORE_STARTUP_LINES) {
    #if N_STARTUP_LINE > 0
      eeprom_put_char(EEPROM_ADDR_STARTUP_BLOCK, 0);
      eeprom_put_char(EEPROM_ADDR_STARTUP_BLOCK+1, 0); // Checksum
    #endif
    #if N_STARTUP_LINE > 1
      eeprom_put_char(EEPROM_ADDR_STARTUP_BLOCK+(LINE_BUFFER_SIZE+1), 0);
      eeprom_put_char(EEPROM_ADDR_STARTUP_BLOCK+(LINE_BUFFER_SIZE+2), 0); // Checksum
    #endif
  }

  if (restore_flag & SETTINGS_RESTORE_BUILD_INFO) {
    eeprom_put_char(EEPROM_ADDR_BUILD_INFO , 0);
    eeprom_put_char(EEPROM_ADDR_BUILD_INFO+1 , 0); // Checksum
  }
}


static uint8_t settings_migrate_v10(void)
{
  settings_v10_t old_settings;
  if (!memcpy_from_eeprom_with_checksum((char*)&old_settings, EEPROM_ADDR_GLOBAL, sizeof(old_settings))) {
    return(false);
  }

  settings = defaults;
  memcpy(settings.steps_per_mm, old_settings.steps_per_mm, sizeof(old_settings.steps_per_mm));
  memcpy(settings.max_rate, old_settings.max_rate, sizeof(old_settings.max_rate));
  memcpy(settings.acceleration, old_settings.acceleration, sizeof(old_settings.acceleration));
  memcpy(settings.max_travel, old_settings.max_travel, sizeof(old_settings.max_travel));
  settings.pulse_microseconds = old_settings.pulse_microseconds;
  settings.step_invert_mask = old_settings.step_invert_mask;
  settings.dir_invert_mask = old_settings.dir_invert_mask;
  settings.stepper_idle_lock_time = old_settings.stepper_idle_lock_time;
  settings.status_report_mask = old_settings.status_report_mask;
  settings.junction_deviation = old_settings.junction_deviation;
  settings.arc_tolerance = old_settings.arc_tolerance;
  settings.flags = old_settings.flags;
  settings.homing_dir_mask = old_settings.homing_dir_mask;
  settings.homing_feed_rate = old_settings.homing_feed_rate;
  settings.homing_seek_rate = old_settings.homing_seek_rate;
  settings.homing_debounce_delay = old_settings.homing_debounce_delay;
  settings.homing_pulloff[X_AXIS] = old_settings.homing_pulloff;
  settings.homing_pulloff[Y_AXIS] = old_settings.homing_pulloff;
  settings.homing_pulloff[Z_AXIS] = old_settings.homing_pulloff;
  write_global_settings();
  return(true);
}


static uint8_t settings_migrate_v11(void)
{
  settings_v11_t old_settings;
  if (!memcpy_from_eeprom_with_checksum((char*)&old_settings, EEPROM_ADDR_GLOBAL, sizeof(old_settings))) {
    return(false);
  }

  settings = defaults;
  memcpy(settings.steps_per_mm, old_settings.steps_per_mm, sizeof(old_settings.steps_per_mm));
  memcpy(settings.max_rate, old_settings.max_rate, sizeof(old_settings.max_rate));
  memcpy(settings.acceleration, old_settings.acceleration, sizeof(old_settings.acceleration));
  memcpy(settings.max_travel, old_settings.max_travel, sizeof(old_settings.max_travel));
  settings.pulse_microseconds = old_settings.pulse_microseconds;
  settings.step_invert_mask = old_settings.step_invert_mask;
  settings.dir_invert_mask = old_settings.dir_invert_mask;
  settings.stepper_idle_lock_time = old_settings.stepper_idle_lock_time;
  settings.status_report_mask = old_settings.status_report_mask;
  settings.junction_deviation = old_settings.junction_deviation;
  settings.arc_tolerance = old_settings.arc_tolerance;
  settings.flags = old_settings.flags & ~(1<<1);
  settings.homing_dir_mask = old_settings.homing_dir_mask;
  settings.homing_origin_max_mask = old_settings.homing_origin_max_mask;
  settings.homing_feed_rate = old_settings.homing_feed_rate;
  settings.homing_seek_rate = old_settings.homing_seek_rate;
  settings.homing_debounce_delay = old_settings.homing_debounce_delay;
  memcpy(settings.homing_pulloff, old_settings.homing_pulloff, sizeof(old_settings.homing_pulloff));
  write_global_settings();
  return(true);
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t settings_read_startup_line(uint8_t n, char *line)
{
  uint32_t addr = n*(LINE_BUFFER_SIZE+1)+EEPROM_ADDR_STARTUP_BLOCK;
  if (!(memcpy_from_eeprom_with_checksum((char*)line, addr, LINE_BUFFER_SIZE))) {
    // Reset line with default value
    line[0] = 0; // Empty line
    settings_store_startup_line(n, line);
    return(false);
  }
  return(true);
}


// Reads startup line from EEPROM. Updated pointed line string data.
uint8_t settings_read_build_info(char *line)
{
  if (!(memcpy_from_eeprom_with_checksum((char*)line, EEPROM_ADDR_BUILD_INFO, LINE_BUFFER_SIZE))) {
    // Reset line with default value
    line[0] = 0; // Empty line
    settings_store_build_info(line);
    return(false);
  }
  return(true);
}


// Read selected coordinate data from EEPROM. Updates pointed coord_data value.
uint8_t settings_read_coord_data(uint8_t coord_select, float *coord_data)
{
  uint32_t addr = coord_select*(sizeof(float)*N_AXIS+1) + EEPROM_ADDR_PARAMETERS;
  if (!(memcpy_from_eeprom_with_checksum((char*)coord_data, addr, sizeof(float)*N_AXIS))) {
    // Reset with default zero vector
    clear_vector_float(coord_data);
    settings_write_coord_data(coord_select,coord_data);
    return(false);
  }
  return(true);
}


// Reads GRBL-CRPT global settings struct from EEPROM.
uint8_t read_global_settings() {
  // Check version-byte of eeprom
  uint8_t version = eeprom_get_char(0);
  if (version == SETTINGS_VERSION) {
    // Read settings-record and check checksum
    if (!(memcpy_from_eeprom_with_checksum((char*)&settings, EEPROM_ADDR_GLOBAL, sizeof(settings_t)))) {
      return(false);
    }
  } else {
    return(false);
  }
  return(true);
}


// A helper method to set settings from command line
uint8_t settings_store_global_setting(uint8_t parameter, float value) {
  if (value < 0.0) { return(STATUS_NEGATIVE_VALUE); }
  if (parameter >= AXIS_SETTINGS_START_VAL) {
    // Store axis configuration. Axis numbering sequence set by AXIS_SETTING defines.
    // NOTE: Ensure the setting index corresponds to the report.c settings printout.
    parameter -= AXIS_SETTINGS_START_VAL;
    uint8_t set_idx = 0;
    while (set_idx < AXIS_N_SETTINGS) {
      if (parameter < N_AXIS) {
        // Valid axis setting found.
        switch (set_idx) {
          case 0:
            #ifdef MAX_STEP_RATE_HZ
              if (value*settings.max_rate[parameter] > (MAX_STEP_RATE_HZ*60.0)) { return(STATUS_MAX_STEP_RATE_EXCEEDED); }
            #endif
            settings.steps_per_mm[parameter] = value;
            break;
          case 1:
            #ifdef MAX_STEP_RATE_HZ
              if (value*settings.steps_per_mm[parameter] > (MAX_STEP_RATE_HZ*60.0)) {  return(STATUS_MAX_STEP_RATE_EXCEEDED); }
            #endif
            settings.max_rate[parameter] = value;
            break;
          case 2: settings.acceleration[parameter] = value*60*60; break; // Convert to mm/min^2 for GRBL-CRPT internal use.
          case 3: settings.max_travel[parameter] = -value; break;  // Store as negative for GRBL-CRPT internal use.
          case 4: settings.homing_pulloff[parameter] = value; break;
        }
        break; // Exit while-loop after setting has been configured and proceed to the EEPROM write call.
      } else {
        set_idx++;
        // If axis index greater than N_AXIS or setting index greater than number of axis settings, error out.
        if ((parameter < AXIS_SETTINGS_INCREMENT) || (set_idx == AXIS_N_SETTINGS)) { return(STATUS_INVALID_STATEMENT); }
        parameter -= AXIS_SETTINGS_INCREMENT;
      }
    }
  } else {
    // Store non-axis GRBL-CRPT settings
    uint8_t int_value = trunc(value);
    switch(parameter) {
      case 0:
        if (int_value < 3) { return(STATUS_SETTING_STEP_PULSE_MIN); }
        settings.pulse_microseconds = int_value; break;
      case 1: settings.stepper_idle_lock_time = int_value; break;
      case 2:
        settings.step_invert_mask = int_value;
        st_generate_step_dir_invert_masks(); // Regenerate step and direction port invert masks.
        break;
      case 3:
        settings.dir_invert_mask = int_value;
        st_generate_step_dir_invert_masks(); // Regenerate step and direction port invert masks.
        break;
      case 4: // Reset to ensure change. Immediate re-init may cause problems.
        if (int_value) { settings.flags |= BITFLAG_INVERT_ST_ENABLE; }
        else { settings.flags &= ~BITFLAG_INVERT_ST_ENABLE; }
        break;
      case 5: // Reset to ensure change. Immediate re-init may cause problems.
        if (int_value) { settings.flags |= BITFLAG_INVERT_LIMIT_PINS; }
        else { settings.flags &= ~BITFLAG_INVERT_LIMIT_PINS; }
        break;
      case 10: settings.status_report_mask = int_value; break;
      case 11: settings.junction_deviation = value; break;
      case 12: settings.arc_tolerance = value; break;
      case 13:
        if (int_value) { settings.flags |= BITFLAG_REPORT_INCHES; }
        else { settings.flags &= ~BITFLAG_REPORT_INCHES; }
        system_flag_wco_change(); // Make sure WCO is immediately updated.
        break;
      case 20:
        if (int_value) {
          if (bit_isfalse(settings.flags, BITFLAG_HOMING_ENABLE)) { return(STATUS_SOFT_LIMIT_ERROR); }
          settings.flags |= BITFLAG_SOFT_LIMIT_ENABLE;
        } else { settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE; }
        break;
      case 21:
        if (int_value) { settings.flags |= BITFLAG_HARD_LIMIT_ENABLE; }
        else { settings.flags &= ~BITFLAG_HARD_LIMIT_ENABLE; }
        limits_init(); // Re-init to immediately change. NOTE: Nice to have but could be problematic later.
        break;
      case 22:
        if (int_value) { settings.flags |= BITFLAG_HOMING_ENABLE; }
        else {
          settings.flags &= ~BITFLAG_HOMING_ENABLE;
          settings.flags &= ~BITFLAG_SOFT_LIMIT_ENABLE; // Force disable soft-limits.
        }
        break;
      case 23: settings.homing_dir_mask = int_value; break;
      case 24: settings.homing_feed_rate = value; break;
      case 25: settings.homing_seek_rate = value; break;
      case 26: settings.homing_debounce_delay = int_value; break;
      case 27:
        for (uint8_t idx = X_AXIS; idx <= Z_AXIS; idx++) { settings.homing_pulloff[idx] = value; }
        break;
      case 28:
        if (int_value >= (1<<N_AXIS)) { return(STATUS_INVALID_STATEMENT); }
        settings.homing_origin_max_mask = int_value;
        break;
      default:
        return(STATUS_INVALID_STATEMENT);
    }
  }
  write_global_settings();
  return(STATUS_OK);
}


// Initialize the config subsystem
void settings_init() {
  if(!read_global_settings()) {
    uint8_t version = eeprom_get_char(0);
    if ((version == SETTINGS_VERSION_PREVIOUS) && settings_migrate_v11()) {
      report_crpt_settings();
    } else if ((version == SETTINGS_VERSION_PREVIOUS_2) && settings_migrate_v10()) {
      report_crpt_settings();
    } else {
      report_status_message(STATUS_SETTING_READ_FAIL);
      settings_restore(SETTINGS_RESTORE_ALL);
      report_crpt_settings();
    }
  }
}


// Returns step pin mask according to GRBL-CRPT internal axis indexing.
uint8_t get_step_pin_mask(uint8_t axis_idx)
{
  if ( axis_idx == X_AXIS ) { return((1<<X_STEP_BIT)); }
  if ( axis_idx == Y_AXIS ) { return((1<<Y_STEP_BIT)); }
  if ( axis_idx == Z_AXIS ) { return((1<<Z_STEP_BIT)); }
  return((1<<A_STEP_BIT));
}


// Returns direction pin mask according to GRBL-CRPT internal axis indexing.
uint8_t get_direction_pin_mask(uint8_t axis_idx)
{
  if ( axis_idx == X_AXIS ) { return((1<<X_DIRECTION_BIT)); }
  if ( axis_idx == Y_AXIS ) { return((1<<Y_DIRECTION_BIT)); }
  if ( axis_idx == Z_AXIS ) { return((1<<Z_DIRECTION_BIT)); }
  return((1<<A_DIRECTION_BIT));
}


// Returns limit pin mask according to GRBL-CRPT internal axis indexing.
uint8_t get_limit_pin_mask(uint8_t axis_idx)
{
  if ( axis_idx == X_AXIS ) { return((1<<X_LIMIT_BIT)); }
  if ( axis_idx == Y_AXIS ) { return((1<<Y_LIMIT_BIT)); }
  if ( axis_idx == Z_AXIS ) { return((1<<Z_LIMIT_BIT)); }
  return((1<<A_LIMIT_BIT));
}
