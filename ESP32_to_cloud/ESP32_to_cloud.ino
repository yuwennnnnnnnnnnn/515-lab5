#include <MagicWand_inferencing.h>
#include <Adafruit_LSM6DS3.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define BUTTON_PIN      D1
#define NEOPIXEL_PIN    D0
#define NEOPIXEL_COUNT  8
#define CONFIDENCE_THRESHOLD 80.0

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://YOUR_SERVER_IP:8000/predict";

Adafruit_LSM6DS3    imu;
Adafruit_NeoPixel   pixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define SAMPLE_RATE_MS      10
#define CAPTURE_DURATION_MS 1000
#define FEATURE_SIZE        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

bool          capturing         = false;
unsigned long last_sample_time  = 0;
unsigned long capture_start_time = 0;
int           sample_count      = 0;
bool          button_last       = HIGH;

float features[FEATURE_SIZE];

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void set_led(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        pixel.setPixelColor(i, pixel.Color(r, g, b));
    pixel.show();
}

void set_rainbow() {
    uint8_t hue = (millis() / 3) & 0xFF;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        uint8_t h = (hue + i * (256 / NEOPIXEL_COUNT)) & 0xFF;
        uint8_t r, g, b;
        if      (h < 85)  { r = h * 3;         g = 255 - h * 3; b = 0;           }
        else if (h < 170) { h -= 85; r = 255 - h*3; g = 0;      b = h * 3;       }
        else              { h -= 170; r = 0;    g = h * 3;       b = 255 - h * 3; }
        pixel.setPixelColor(i, pixel.Color(r, g, b));
    }
    pixel.show();
}

void actuateLED(const char* gesture) {
    if      (strcmp(gesture, "Z") == 0) set_led(200, 0,   0);    // red   = Freeze Ray
    else if (strcmp(gesture, "O") == 0) set_led(200, 200, 200);  // white = McFlurry Barrier
    else if (strcmp(gesture, "V") == 0) set_led(0,   200, 0);    // green = Caramel Drizzle
    else                                set_led(80,  80,  80);
    delay(3000);
    set_led(0, 0, 200);
}

void sendRawDataToServer() {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Build JSON: {"data": [f0, f1, ..., fn]}
    String jsonPayload = "{\"data\":[";
    for (int i = 0; i < FEATURE_SIZE; i++) {
        jsonPayload += String(features[i], 4);
        if (i < FEATURE_SIZE - 1) jsonPayload += ",";
    }
    jsonPayload += "]}";

    int httpResponseCode = http.POST(jsonPayload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server response: " + response);

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, response);
        if (!error) {
            const char* gesture    = doc["gesture"];
            float       confidence = doc["confidence"];
            Serial.println("Server Inference Result:");
            Serial.print("Gesture: ");    Serial.println(gesture);
            Serial.print("Confidence: "); Serial.print(confidence); Serial.println("%");
            actuateLED(gesture);
        } else {
            Serial.print("Failed to parse server response: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

void print_inference_result(ei_impulse_result_t result) {
    float max_value = 0;
    int   max_index = -1;

    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_value) {
            max_value = result.classification[i].value;
            max_index = i;
        }
    }

    float confidence = max_value * 100.0f;
    const char* label = (max_index != -1) ? ei_classifier_inferencing_categories[max_index] : "UNKNOWN";

    Serial.print("Local prediction: "); Serial.print(label);
    Serial.print(" ("); Serial.print(confidence); Serial.println("%)");

    if (confidence < CONFIDENCE_THRESHOLD) {
        Serial.println("Low confidence — sending raw data to server...");
        sendRawDataToServer();
    } else {
        Serial.println("High confidence — performing local inference.");
        actuateLED(label);
    }
}

void run_inference() {
    if (sample_count * 3 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }

    ei_impulse_result_t result = { 0 };
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    features_signal.get_data     = &raw_feature_get_data;

    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier ("); Serial.print(res); Serial.println(")");
        return;
    }

    print_inference_result(result);
}

void start_capture() {
    Serial.println("Starting gesture capture...");
    sample_count = 0;
    capturing    = true;
    capture_start_time = millis();
    last_sample_time   = millis();
}

void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();

        sensors_event_t a, g, temp;
        imu.getEvent(&a, &g, &temp);

        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx]     = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }

        if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
            capturing = false;
            Serial.println("Capture complete");
            run_inference();
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pixel.begin();
    pixel.setBrightness(255);
    set_led(0, 0, 0);

    Serial.println("Initializing LSM6DS3...");
    if (!imu.begin_I2C(0x6B)) {
        Serial.println("Failed to find LSM6DS3 chip");
        while (1) { delay(10); }
    }
    imu.setAccelRange(LSM6DS_ACCEL_RANGE_8_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    Serial.println("LSM6DS3 initialized");

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected. IP: "); Serial.println(WiFi.localIP());

    set_led(0, 0, 200);
    delay(300);
    set_led(0, 0, 0);
    Serial.println("Press button (D1) or send 'o' to start gesture capture");
}

void loop() {
    bool button_now = digitalRead(BUTTON_PIN);
    if (button_now == LOW && button_last == HIGH && !capturing)
        start_capture();
    button_last = button_now;

    if (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 'o' && !capturing) start_capture();
    }

    if (capturing) {
        set_rainbow();
        capture_accelerometer_data();
    }
}
