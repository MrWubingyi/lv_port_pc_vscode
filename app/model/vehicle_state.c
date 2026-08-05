#include "vehicle_state.h"

#include <cjson/cJSON.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static bool is_valid_gear(const char *gear) {
  if (gear == NULL || strlen(gear) != 1) {
    return false;
  }

  return strchr("PRND", gear[0]) != NULL;
}

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

  bool fields_valid = cJSON_IsNumber(version) && cJSON_IsNumber(sequence) &&
                      cJSON_IsNumber(timestamp) && cJSON_IsNumber(speed) &&
                      cJSON_IsNumber(rpm) && cJSON_IsString(gear) &&
                      cJSON_IsNumber(soc);

  if (!fields_valid) {
    fprintf(stderr, "Missing or invalid field type\n");
    cJSON_Delete(root);
    return false;
  }

  bool ranges_valid = version->valueint == 1 && sequence->valuedouble >= 0 &&
                      timestamp->valuedouble >= 0 && speed->valueint >= 0 &&
                      speed->valueint <= 200 && rpm->valueint >= 0 &&
                      rpm->valueint <= 8000 && soc->valueint >= 0 &&
                      soc->valueint <= 100 && is_valid_gear(gear->valuestring);

  if (!ranges_valid) {
    fprintf(stderr, "Vehicle field out of range\n");
    cJSON_Delete(root);
    return false;
  }

  vehicle_state_t parsed = {.version = version->valueint,
                            .sequence = (uint64_t)sequence->valuedouble,
                            .timestamp_ms = (uint64_t)timestamp->valuedouble,
                            .speed_kph = speed->valueint,
                            .rpm = rpm->valueint,
                            .gear = gear->valuestring[0],
                            .soc = soc->valueint,
                            .range_km = soc->valueint * 12,
                            .outside_temp_c = -5,
                            .load_tenths = 64,
                            .load_max_tenths = 220,
                            .trip_tenths = 0};

  int steering_buttons = 0;
  bool optional_fields_valid =
      read_optional_int(root, "rangeKm", 0, 5000, &parsed.range_km) &&
      read_optional_int(root, "outsideTempC", -100, 100,
                        &parsed.outside_temp_c) &&
      read_optional_int(root, "loadTenths", 0, 5000,
                        &parsed.load_tenths) &&
      read_optional_int(root, "loadMaxTenths", 0, 5000,
                        &parsed.load_max_tenths) &&
      read_optional_int(root, "tripTenths", 0, 1000000000,
                        &parsed.trip_tenths) &&
      read_optional_bool(root, "engineWarning", &parsed.engine_warning) &&
      read_optional_int(root, "steeringButtons", 0, 0x7fffffff,
                        &steering_buttons) &&
      read_optional_bool(root, "seatbeltWarning", &parsed.seatbelt_warning) &&
      read_optional_bool(root, "handbrake", &parsed.handbrake) &&
      read_optional_bool(root, "parkingBrake", &parsed.handbrake) &&
      read_optional_bool(root, "brakingWarning", &parsed.braking_warning) &&
      read_optional_bool(root, "coolantWarning", &parsed.coolant_warning) &&
      read_optional_bool(root, "highBeam", &parsed.high_beam) &&
      read_optional_bool(root, "lowBeam", &parsed.low_beam);
  if (!optional_fields_valid) {
    fprintf(stderr, "Invalid optional vehicle field\n");
    cJSON_Delete(root);
    return false;
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

  printf("Parsed: seq=%" PRIu64 " time=%" PRIu64 " speed=%d km/h"
         " rpm=%d"
         " gear=%c"
         " soc=%d%%\n",
         state->sequence, state->timestamp_ms, state->speed_kph, state->rpm,
         state->gear, state->soc);

  fflush(stdout);
}
