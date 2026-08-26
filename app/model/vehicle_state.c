#include "vehicle_state.h"

#include <cjson/cJSON.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool read_optional_int(const cJSON *root, const char *name,
                              int minimum, int maximum, int *value) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) {
    return true;
  }
  if (!cJSON_IsNumber(item) || item->valuedouble < minimum ||
      item->valuedouble > maximum) {
    return false;
  }
  *value = item->valueint;
  return true;
}

static bool read_optional_bool(const cJSON *root, const char *name,
                               bool *value) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsBool(item)) return false;
  *value = cJSON_IsTrue(item);
  return true;
}

static bool read_optional_double(const cJSON *root, const char *name,
                                 double minimum, double maximum,
                                 double *value) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsNumber(item)) return false;
  if (!isfinite(item->valuedouble)) return false;
  if (item->valuedouble < minimum || item->valuedouble > maximum) {
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
    // fprintf(stderr, "[VehicleState] Invalid JSON syntax\n");
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

  int gear_val = 0;
  bool gear_valid = false;
  if (cJSON_IsNumber(gear)) {
    gear_val = gear->valueint;
    if (gear_val >= VEHICLE_GEAR_P && gear_val <= VEHICLE_GEAR_D) {
      gear_valid = true;
    }
  } else if (cJSON_IsString(gear) && gear->valuestring != NULL &&
             strlen(gear->valuestring) == 1) {
    switch (gear->valuestring[0]) {
      case 'P':
        gear_val = VEHICLE_GEAR_P;
        gear_valid = true;
        break;
      case 'R':
        gear_val = VEHICLE_GEAR_R;
        gear_valid = true;
        break;
      case 'N':
        gear_val = VEHICLE_GEAR_N;
        gear_valid = true;
        break;
      case 'D':
        gear_val = VEHICLE_GEAR_D;
        gear_valid = true;
        break;
    }
  }

  bool fields_valid = cJSON_IsNumber(version) && cJSON_IsNumber(sequence) &&
                      cJSON_IsNumber(timestamp) && cJSON_IsNumber(speed) &&
                      cJSON_IsNumber(rpm) && gear_valid &&
                      cJSON_IsNumber(soc);

  if (!fields_valid) {
    // fprintf(stderr, "[VehicleState] Missing or invalid field type\n");
    cJSON_Delete(root);
    return false;
  }

  /* Boundary check according to PROTOCOL.md:
   * version >= 0 (suggested >= 1)
   * seq >= 0
   * timestampMs > 0
   * speedKph: 0 - 200
   * rpm: 0 - 8000
   * soc: 0 - 100
   */
  bool ranges_valid = version->valueint >= 0 && sequence->valuedouble >= 0 &&
                      timestamp->valuedouble > 0 && speed->valueint >= 0 &&
                      speed->valueint <= 200 && rpm->valueint >= 0 &&
                      rpm->valueint <= 8000 && soc->valueint >= 0 &&
                      soc->valueint <= 100;

  if (!ranges_valid) {
    // fprintf(stderr, "[VehicleState] Vehicle field out of range (speed=%d, rpm=%d, soc=%d)\n",
    //         speed->valueint, rpm->valueint, soc->valueint);
    cJSON_Delete(root);
    return false;
  }

  vehicle_state_t parsed = {.version = version->valueint,
                            .sequence = (uint64_t)sequence->valuedouble,
                            .timestamp_ms = (uint64_t)timestamp->valuedouble,
                            .speed_kph = speed->valueint,
                            .rpm = rpm->valueint,
                            .gear = gear_val,
                            .soc = soc->valueint,
                            .turn_signal = VEHICLE_TURN_SIGNAL_NONE,
                            .door_lock = true,
                            .parking_brake = false,
                            .warning = VEHICLE_WARNING_NONE,
                            .validity = VEHICLE_VALIDITY_VALID,
                            .headlights_state = 0,
                            .high_beam_lights_state = 0,
                            .engine_coolant_temp = 90.0,
                            .ev_battery_level = (double)soc->valueint,
                            .data_status = VEHICLE_DATA_STATUS_NORMAL,
                            .range_km = soc->valueint * 12,
                            .outside_temp_c = -5,
                            .load_tenths = 64,
                            .load_max_tenths = 220,
                            .trip_tenths = 0};

  int steering_buttons = 0;
  bool optional_fields_valid =
      read_optional_int(root, "turnSignal", VEHICLE_TURN_SIGNAL_NONE,
                        VEHICLE_TURN_SIGNAL_HAZARD, &parsed.turn_signal) &&
      read_optional_bool(root, "doorLock", &parsed.door_lock) &&
      read_optional_bool(root, "parkingBrake", &parsed.parking_brake) &&
      read_optional_int(root, "warning", VEHICLE_WARNING_NONE,
                        VEHICLE_WARNING_CRITICAL, &parsed.warning) &&
      read_optional_int(root, "validity", VEHICLE_VALIDITY_VALID,
                        VEHICLE_VALIDITY_STALE, &parsed.validity) &&
      read_optional_int(root, "headlightsState", 0, 100,
                        &parsed.headlights_state) &&
      read_optional_int(root, "highBeamLightsState", 0, 100,
                        &parsed.high_beam_lights_state) &&
      read_optional_double(root, "engineCoolantTemp", -50.0, 200.0,
                           &parsed.engine_coolant_temp) &&
      read_optional_double(root, "evBatteryLevel", 0.0, 100.0,
                           &parsed.ev_battery_level) &&
      read_optional_int(root, "dataStatus", VEHICLE_DATA_STATUS_NORMAL,
                        VEHICLE_DATA_STATUS_TRANSPORT_DISCONNECTED,
                        &parsed.data_status) &&
      read_optional_int(root, "rangeKm", 0, 5000, &parsed.range_km) &&
      read_optional_int(root, "outsideTempC", -100, 100,
                        &parsed.outside_temp_c) &&
      read_optional_int(root, "loadTenths", 0, 5000, &parsed.load_tenths) &&
      read_optional_int(root, "loadMaxTenths", 1, 5000,
                        &parsed.load_max_tenths) &&
      read_optional_int(root, "tripTenths", 0, 1000000000,
                        &parsed.trip_tenths) &&
      read_optional_bool(root, "engineWarning", &parsed.engine_warning) &&
      read_optional_int(root, "steeringButtons", 0, 0x7fffffff,
                        &steering_buttons) &&
      read_optional_bool(root, "beltWarning", &parsed.seatbelt_warning) &&
      read_optional_bool(root, "seatbeltWarning", &parsed.seatbelt_warning) &&
      read_optional_bool(root, "handbrake", &parsed.handbrake) &&
      read_optional_bool(root, "brakingWarning", &parsed.braking_warning) &&
      read_optional_bool(root, "coolantWarning", &parsed.coolant_warning) &&
      read_optional_bool(root, "highBeam", &parsed.high_beam) &&
      read_optional_bool(root, "lowBeam", &parsed.low_beam);

  if (!optional_fields_valid) {
    // fprintf(stderr, "[VehicleState] Invalid optional vehicle field out of range or malformed\n");
    cJSON_Delete(root);
    return false;
  }

  /* PROTOCOL.md Section 6.2: dataStatus == 1 (INVALID) indicates illegal/invalid frame.
   * Discard invalid data packets to protect the system.
   */
  if (parsed.data_status == VEHICLE_DATA_STATUS_INVALID) {
    // fprintf(stderr, "[VehicleState] Discarded illegal frame with dataStatus=1 (INVALID)\n");
    cJSON_Delete(root);
    return false;
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

  parsed.steering_buttons = (uint32_t)steering_buttons;

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
