#include "vehicle_state.h"

#include <cjson/cJSON.h>
#include <inttypes.h>
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
                                 double *value) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
  if (item == NULL) return true;
  if (!cJSON_IsNumber(item)) return false;
  *value = item->valuedouble;
  return true;
}

bool vehicle_state_parse_json(const char *json_text, vehicle_state_t *state) {
  if (json_text == NULL || state == NULL) {
    return false;
  }

  cJSON *root = cJSON_Parse(json_text);

  if (root == NULL) {
    fprintf(stderr, "Invalid JSON\n");
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
    if (gear_val >= 0 && gear_val <= 3) {
      gear_valid = true;
    }
  } else if (cJSON_IsString(gear) && gear->valuestring != NULL &&
             strlen(gear->valuestring) == 1) {
    switch (gear->valuestring[0]) {
      case 'P':
        gear_val = 0;
        gear_valid = true;
        break;
      case 'R':
        gear_val = 1;
        gear_valid = true;
        break;
      case 'N':
        gear_val = 2;
        gear_valid = true;
        break;
      case 'D':
        gear_val = 3;
        gear_valid = true;
        break;
    }
  }

  bool fields_valid = cJSON_IsNumber(version) && cJSON_IsNumber(sequence) &&
                      cJSON_IsNumber(timestamp) && cJSON_IsNumber(speed) &&
                      cJSON_IsNumber(rpm) && gear_valid &&
                      cJSON_IsNumber(soc);

  if (!fields_valid) {
    fprintf(stderr, "Missing or invalid field type\n");
    cJSON_Delete(root);
    return false;
  }

  bool ranges_valid = version->valueint >= 0 && sequence->valuedouble >= 0 &&
                      timestamp->valuedouble >= 0 && speed->valueint >= 0 &&
                      speed->valueint <= 200 && rpm->valueint >= 0 &&
                      rpm->valueint <= 8000 && soc->valueint >= 0 &&
                      soc->valueint <= 100;

  if (!ranges_valid) {
    // fprintf(stderr, "Vehicle field out of range\n");
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
                            .turn_signal = 0,
                            .door_lock = true,
                            .parking_brake = false,
                            .warning = 0,
                            .validity = 0,
                            .headlights_state = 0,
                            .high_beam_lights_state = 0,
                            .engine_coolant_temp = 90.0,
                            .ev_battery_level = (double)soc->valueint,
                            .data_status = 0,
                            .range_km = soc->valueint * 12,
                            .outside_temp_c = -5,
                            .load_tenths = 64,
                            .load_max_tenths = 220,
                            .trip_tenths = 0};

  int steering_buttons = 0;
  bool optional_fields_valid =
      read_optional_int(root, "turnSignal", 0, 3, &parsed.turn_signal) &&
      read_optional_bool(root, "doorLock", &parsed.door_lock) &&
      read_optional_bool(root, "parkingBrake", &parsed.parking_brake) &&
      read_optional_int(root, "warning", 0, 2, &parsed.warning) &&
      read_optional_int(root, "validity", 0, 3, &parsed.validity) &&
      read_optional_int(root, "headlightsState", 0, 100,
                        &parsed.headlights_state) &&
      read_optional_int(root, "highBeamLightsState", 0, 100,
                        &parsed.high_beam_lights_state) &&
      read_optional_double(root, "engineCoolantTemp",
                           &parsed.engine_coolant_temp) &&
      read_optional_double(root, "evBatteryLevel",
                           &parsed.ev_battery_level) &&
      read_optional_int(root, "dataStatus", 0, 4, &parsed.data_status) &&
      read_optional_int(root, "rangeKm", 0, 5000, &parsed.range_km) &&
      read_optional_int(root, "outsideTempC", -100, 100,
                        &parsed.outside_temp_c) &&
      read_optional_int(root, "loadTenths", 0, 5000, &parsed.load_tenths) &&
      read_optional_int(root, "loadMaxTenths", 0, 5000,
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
    // fprintf(stderr, "Invalid optional vehicle field\n");
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
  parsed.battery_warning = (parsed.soc <= 15 || parsed.ev_battery_level <= 15.0);

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
