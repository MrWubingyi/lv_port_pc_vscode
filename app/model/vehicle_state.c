#include "vehicle_state.h"

#include <cjson/cJSON.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool parse_optional_int(const cJSON *root, const char *name,
                               int minimum, int maximum, int *value,
                               bool *optional_invalid) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsNumber(item)) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' type error (expected number)\n", name);
    return false;
  }
  if (item->valuedouble < minimum || item->valuedouble > maximum) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' out of range [%d, %d] (got %.2f)\n",
            name, minimum, maximum, item->valuedouble);
    return false;
  }
  *value = item->valueint;
  return true;
}

static bool parse_optional_bool(const cJSON *root, const char *name,
                                bool *value, bool *optional_invalid) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsBool(item)) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' type error (expected boolean)\n", name);
    return false;
  }
  *value = cJSON_IsTrue(item);
  return true;
}

static bool parse_optional_double(const cJSON *root, const char *name,
                                  double minimum, double maximum,
                                  double *value, bool *optional_invalid) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsNumber(item)) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' type error (expected number)\n", name);
    return false;
  }
  if (!isfinite(item->valuedouble)) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' is not finite (NaN/Inf)\n", name);
    return false;
  }
  if (item->valuedouble < minimum || item->valuedouble > maximum) {
    if (optional_invalid) *optional_invalid = true;
    fprintf(stderr, "[VehicleState] Optional field '%s' out of range [%.1f, %.1f] (got %.2f)\n",
            name, minimum, maximum, item->valuedouble);
    return false;
  }
  *value = item->valuedouble;
  return true;
}

bool vehicle_state_parse_json(const char *json_text, vehicle_state_t *state) {
  if (json_text == NULL || state == NULL) {
    return false;
  }

  cJSON *root = cJSON_Parse(json_text);
  if (root == NULL) {
    fprintf(stderr, "[VehicleState] Invalid JSON syntax\n");
    return false;
  }

  const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
  const cJSON *sequence = cJSON_GetObjectItemCaseSensitive(root, "seq");
  const cJSON *timestamp =
      cJSON_GetObjectItemCaseSensitive(root, "timestampMs");
  const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speedKph");
  const cJSON *rpm = cJSON_GetObjectItemCaseSensitive(root, "rpm");
  const cJSON *gear = cJSON_GetObjectItemCaseSensitive(root, "gear");
  const cJSON *soc = cJSON_GetObjectItemCaseSensitive(root, "soc");

  bool version_valid = (version != NULL && cJSON_IsNumber(version) && version->valueint >= 0);
  if (!version_valid) {
    fprintf(stderr, "[VehicleState] Required field 'version' missing, wrong type, or < 0\n");
  }

  bool seq_valid = (sequence != NULL && cJSON_IsNumber(sequence) && sequence->valuedouble >= 0);
  if (!seq_valid) {
    fprintf(stderr, "[VehicleState] Required field 'seq' missing, wrong type, or < 0\n");
  }

  bool time_valid = (timestamp != NULL && cJSON_IsNumber(timestamp) && timestamp->valuedouble > 0);
  if (!time_valid) {
    fprintf(stderr, "[VehicleState] Required field 'timestampMs' missing, wrong type, or <= 0\n");
  }

  int speed_val = 0;
  bool speed_valid = false;
  if (speed != NULL && cJSON_IsNumber(speed)) {
    speed_val = speed->valueint;
    if (speed_val >= 0 && speed_val <= 200) {
      speed_valid = true;
    } else {
      fprintf(stderr, "[VehicleState] Required field 'speedKph' out of range [0, 200] (got %d)\n", speed_val);
    }
  } else {
    fprintf(stderr, "[VehicleState] Required field 'speedKph' missing or not a number\n");
  }

  int rpm_val = 0;
  bool rpm_valid = false;
  if (rpm != NULL && cJSON_IsNumber(rpm)) {
    rpm_val = rpm->valueint;
    if (rpm_val >= 0 && rpm_val <= 8000) {
      rpm_valid = true;
    } else {
      fprintf(stderr, "[VehicleState] Required field 'rpm' out of range [0, 8000] (got %d)\n", rpm_val);
    }
  } else {
    fprintf(stderr, "[VehicleState] Required field 'rpm' missing or not a number\n");
  }

  int soc_val = 0;
  bool soc_valid = false;
  if (soc != NULL && cJSON_IsNumber(soc)) {
    soc_val = soc->valueint;
    if (soc_val >= 0 && soc_val <= 100) {
      soc_valid = true;
    } else {
      fprintf(stderr, "[VehicleState] Required field 'soc' out of range [0, 100] (got %d)\n", soc_val);
    }
  } else {
    fprintf(stderr, "[VehicleState] Required field 'soc' missing or not a number\n");
  }

  int gear_val = VEHICLE_GEAR_P;
  bool gear_valid = false;
  if (gear != NULL) {
    if (cJSON_IsNumber(gear)) {
      gear_val = gear->valueint;
      if (gear_val >= VEHICLE_GEAR_P && gear_val <= VEHICLE_GEAR_D) {
        gear_valid = true;
      }
    } else if (cJSON_IsString(gear) && gear->valuestring != NULL &&
               strlen(gear->valuestring) == 1) {
      switch (gear->valuestring[0]) {
        case 'P': gear_val = VEHICLE_GEAR_P; gear_valid = true; break;
        case 'R': gear_val = VEHICLE_GEAR_R; gear_valid = true; break;
        case 'N': gear_val = VEHICLE_GEAR_N; gear_valid = true; break;
        case 'D': gear_val = VEHICLE_GEAR_D; gear_valid = true; break;
        default: break;
      }
    }
  }
  if (!gear_valid) {
    fprintf(stderr, "[VehicleState] Required field 'gear' missing, wrong type, or invalid value\n");
  }

  bool required_fields_valid = version_valid && seq_valid && time_valid &&
                               speed_valid && rpm_valid && soc_valid && gear_valid;

  int version_out = version_valid ? version->valueint : 0;
  uint64_t seq_out = seq_valid ? (uint64_t)sequence->valuedouble : 0;
  uint64_t time_out = time_valid ? (uint64_t)timestamp->valuedouble : 0;

  vehicle_state_t parsed = {
      .version = version_out,
      .sequence = seq_out,
      .timestamp_ms = time_out,
      .speed_kph = speed_val,
      .rpm = rpm_val,
      .gear = gear_val,
      .soc = soc_val,
      .turn_signal = VEHICLE_TURN_SIGNAL_NONE,
      .door_lock = true,
      .parking_brake = false,
      .warning = VEHICLE_WARNING_NONE,
      .validity = speed_valid ? VEHICLE_VALIDITY_VALID : VEHICLE_VALIDITY_INVALID_SPEED,
      .headlights_state = 0,
      .high_beam_lights_state = 0,
      .engine_coolant_temp = 90.0,
      .ev_battery_level = (double)soc_val,
      .data_status = required_fields_valid ? VEHICLE_DATA_STATUS_NORMAL : VEHICLE_DATA_STATUS_INVALID,
      .range_km = soc_val * 12,
      .outside_temp_c = -5,
      .load_tenths = 64,
      .load_max_tenths = 220,
      .trip_tenths = 0
  };

  /* Cumulative tracking of invalid optional fields */
  bool optional_invalid = false;

  parse_optional_int(root, "turnSignal", VEHICLE_TURN_SIGNAL_NONE,
                     VEHICLE_TURN_SIGNAL_HAZARD, &parsed.turn_signal, &optional_invalid);
  parse_optional_bool(root, "doorLock", &parsed.door_lock, &optional_invalid);
  parse_optional_bool(root, "parkingBrake", &parsed.parking_brake, &optional_invalid);
  parse_optional_int(root, "warning", VEHICLE_WARNING_NONE,
                     VEHICLE_WARNING_CRITICAL, &parsed.warning, &optional_invalid);
  parse_optional_int(root, "validity", VEHICLE_VALIDITY_VALID,
                     VEHICLE_VALIDITY_STALE, &parsed.validity, &optional_invalid);
  if (!speed_valid) {
    parsed.validity = VEHICLE_VALIDITY_INVALID_SPEED;
  }

  parse_optional_int(root, "headlightsState", 0, 100,
                     &parsed.headlights_state, &optional_invalid);
  parse_optional_int(root, "highBeamLightsState", 0, 100,
                     &parsed.high_beam_lights_state, &optional_invalid);
  parse_optional_double(root, "engineCoolantTemp", -50.0, 200.0,
                        &parsed.engine_coolant_temp, &optional_invalid);
  parse_optional_double(root, "evBatteryLevel", 0.0, 100.0,
                        &parsed.ev_battery_level, &optional_invalid);

  int data_status_val = VEHICLE_DATA_STATUS_NORMAL;
  if (!parse_optional_int(root, "dataStatus", VEHICLE_DATA_STATUS_NORMAL,
                          VEHICLE_DATA_STATUS_TRANSPORT_DISCONNECTED, &data_status_val, &optional_invalid)) {
    parsed.data_status = VEHICLE_DATA_STATUS_INVALID;
  } else {
    parsed.data_status = data_status_val;
  }

  parse_optional_int(root, "rangeKm", 0, 5000,
                     &parsed.range_km, &optional_invalid);
  parse_optional_int(root, "outsideTempC", -100, 100,
                     &parsed.outside_temp_c, &optional_invalid);
  parse_optional_int(root, "loadTenths", 0, 5000,
                     &parsed.load_tenths, &optional_invalid);
  parse_optional_int(root, "loadMaxTenths", 1, 5000,
                     &parsed.load_max_tenths, &optional_invalid);
  parse_optional_int(root, "tripTenths", 0, 1000000000,
                     &parsed.trip_tenths, &optional_invalid);
  parse_optional_bool(root, "engineWarning", &parsed.engine_warning, &optional_invalid);

  int steering_buttons = 0;
  parse_optional_int(root, "steeringButtons", 0, 0x7fffffff,
                     &steering_buttons, &optional_invalid);
  parsed.steering_buttons = (uint32_t)steering_buttons;

  parse_optional_bool(root, "beltWarning", &parsed.seatbelt_warning, &optional_invalid);
  parse_optional_bool(root, "seatbeltWarning", &parsed.seatbelt_warning, &optional_invalid);
  parse_optional_bool(root, "handbrake", &parsed.handbrake, &optional_invalid);
  parse_optional_bool(root, "brakingWarning", &parsed.braking_warning, &optional_invalid);
  parse_optional_bool(root, "coolantWarning", &parsed.coolant_warning, &optional_invalid);
  parse_optional_bool(root, "highBeam", &parsed.high_beam, &optional_invalid);
  parse_optional_bool(root, "lowBeam", &parsed.low_beam, &optional_invalid);

  /* If any required or optional field is illegal/invalid, mark overall state as INVALID */
  bool is_frame_illegal = (!required_fields_valid || optional_invalid || (parsed.data_status == VEHICLE_DATA_STATUS_INVALID));
  if (is_frame_illegal) {
    parsed.data_status = VEHICLE_DATA_STATUS_INVALID;
  }

  /* Sync handbrake if parkingBrake was provided */
  if (cJSON_GetObjectItemCaseSensitive(root, "parkingBrake") != NULL) {
    parsed.handbrake = parsed.parking_brake;
  }

  /* High beam state sync */
  if (cJSON_GetObjectItemCaseSensitive(root, "highBeamLightsState") != NULL) {
    parsed.high_beam = (parsed.high_beam_lights_state > 0);
  }

  /* Coolant temp warning trigger: > 105.0 C or explicit coolantWarning */
  if (parsed.engine_coolant_temp > 105.0) {
    parsed.coolant_warning = true;
  }

  /* Battery warning trigger: soc <= 15% or evBatteryLevel <= 15.0% */
  if (parsed.soc <= 15 || parsed.ev_battery_level <= 15.0) {
    parsed.battery_warning = true;
  }

  /* General or Critical Warning status sync */
  if (parsed.warning == VEHICLE_WARNING_CRITICAL) {
    parsed.engine_warning = true;
    parsed.braking_warning = true;
  } else if (parsed.warning == VEHICLE_WARNING_GENERAL) {
    parsed.engine_warning = true;
  }

  *state = parsed;
  cJSON_Delete(root);
  return true;
}

void vehicle_state_print(const vehicle_state_t *state) {
  if (state == NULL) {
    return;
  }

  static const char gear_chars[] = {'P', 'R', 'N', 'D'};
  char gear_char = (state->gear >= 0 && state->gear <= 3) ? gear_chars[state->gear] : '?';

  // printf("Parsed: seq=%" PRIu64 " time=%" PRIu64 " speed=%d km/h"
  //        " rpm=%d"
  //        " gear=%c(%d)"
  //        " soc=%d%%"
  //        " turnSignal=%d"
  //        " doorLock=%s"
  //        " beltWarn=%s"
  //        " highBeam=%s"
  //        " coolantTemp=%.1fC(warn=%s)"
  //        " batteryWarn=%s\n",
  //        state->sequence, state->timestamp_ms, state->speed_kph, state->rpm,
  //        gear_char, state->gear, state->soc, state->turn_signal,
  //        state->door_lock ? "locked" : "unlocked",
  //        state->seatbelt_warning ? "WARN" : "OK",
  //        state->high_beam ? "ON" : "OFF",
  //        state->engine_coolant_temp, state->coolant_warning ? "YES" : "NO",
  //        state->battery_warning ? "LOW" : "OK");

  fflush(stdout);
}
