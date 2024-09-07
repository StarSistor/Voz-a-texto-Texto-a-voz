/*
Moving Average Filter:

A moving average filter is applied to each flex sensor value. This filter smooths the readings by averaging the values over the last filterWindowSize samples, reducing the impact of noise.
Filter Variables:

filterWindowSize: Defines the number of samples over which the average is calculated. Adjust this value to balance between responsiveness and noise reduction.
filterValues: An array to store the recent readings for each sensor.
filterIndex: Keeps track of the current position in the circular buffer.
Tips for Further Optimization:
Adjust filterWindowSize: Start with a smaller window size (e.g., 3) and increase it if more smoothing is needed. Larger sizes make the system less responsive but more stable.
Test with Actual Gestures: Calibrate the code with real gestures and fine-tune the tolerance and gestureDurationThreshold based on the results.
Exponential Moving Average (EMA): For a smoother response that reacts faster to changes, consider using EMA. It gives more weight to recent values while still smoothing out the data.
*/
#include <WiFi.h>
#include <Audio.h>

// Pines para los sensores flex
const int flexSensorPins[5] = {36, 39, 35, 32, 34};

// Pines para el parlante I2S
#define I2S_DOUT 26
#define I2S_BCLK 25
#define I2S_LRC 33

const int ledPin = 27;
bool ledState = LOW;

// Parámetros de WiFi y Audio
const char* ssid = "";
const char* password = "";
Audio audio;

// Tolerancia para la detección de gestos
int tolerance = 130;

// Umbral de duración para considerar un gesto válido (ms)
const int gestureDurationThreshold = 2000;

// Umbral de tiempo para separar palabras (ms)
const unsigned long wordSeparatorTimeout = 5000; // Increased to 5 seconds

// Voltajes predefinidos para cada letra
const int letterVoltages[26][5] PROGMEM = {
    {858, 897, 850, 1040, 1009},
};

char currentLetter = '\0';
unsigned long gestureStartTime = 0;
String currentWord = "";
unsigned long lastLetterTime = 0;
char lastAddedLetter = '\0'; // Track the last added letter

// Moving average filter settings
const int filterWindowSize = 5; // Number of samples to average
int filterValues[5][filterWindowSize] = {0}; // Stores recent readings for each sensor
int filterIndex = 0; // Current index in the filter array

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  for (int i = 0; i < 5; i++) {
    pinMode(flexSensorPins[i], INPUT);
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
}

void loop() {
  int flexValues[5];
  
  // Read and filter the sensor values
  for (int i = 0; i < 5; i++) {
    int rawValue = analogRead(flexSensorPins[i]);
    filterValues[i][filterIndex] = rawValue; // Add new reading to the filter array
    
    // Calculate the moving average
    int sum = 0;
    for (int j = 0; j < filterWindowSize; j++) {
      sum += filterValues[i][j];
    }
    flexValues[i] = sum / filterWindowSize; // Average of the window
  }

  filterIndex = (filterIndex + 1) % filterWindowSize; // Move to the next index
  
  // Print filtered values for debugging
  Serial.print("Filtered Flex values: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(flexValues[i]);
    Serial.print(" ");
  }
  Serial.println();

  char detectedLetter = detectLetter(flexValues);

  if (detectedLetter != '\0') {
    if (detectedLetter != currentLetter) {
      currentLetter = detectedLetter;
      gestureStartTime = millis();
    } else {
      if (millis() - gestureStartTime > gestureDurationThreshold) {
        if (millis() - lastLetterTime > gestureDurationThreshold) {
          // Cambiar el estado del LED
          digitalWrite(ledPin, ledState);
          ledState = !ledState;

          // Add letter to word if it's not a repeated letter
          if (detectedLetter != lastAddedLetter) {
            currentWord += detectedLetter;
            lastAddedLetter = detectedLetter;
            Serial.println(currentLetter);
          }

          lastLetterTime = millis();
        }
      }
    }
  } else {
    lastAddedLetter = '\0'; // Reset lastAddedLetter when no letter detected
  }

  if (millis() - lastLetterTime > wordSeparatorTimeout && currentWord.length() > 0) {
    Serial.println("Sending word: " + currentWord);
    speakWord(currentWord);
    currentWord = "";
    lastLetterTime = millis(); // Reset timer to prevent immediate re-processing
  }

  audio.loop();
}

char detectLetter(int flexValues[5]) {
  for (int i = 0; i < 26; i++) {
    bool match = true;
    for (int j = 0; j < 5; j++) {
      if (abs(flexValues[j] - pgm_read_word(&(letterVoltages[i][j]))) > tolerance) {
        match = false;
        break;
      }
    }
    if (match) {
      return 'a' + i;
    }
  }
  return '\0';
}

void speakWord(String word) {
  word.toLowerCase();
  audio.connecttospeech(word.c_str(), "es");
}

void audio_info(const char *info) {
  Serial.println(info);
}
