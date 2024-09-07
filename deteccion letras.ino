/*
Descripción de los Cambios y Funcionamiento del Filtro
Filtro de Media Móvil: Cada sensor tiene una ventana de tamaño filterWindowSize (en este caso, 10 lecturas) que mantiene las últimas lecturas y calcula su media para reducir el ruido. Esto ayuda a estabilizar los valores y mejora la precisión en la detección de gestos.

Inicialización de Lecturas: La función initFlexReadings() inicializa los arrays de lecturas de los sensores a cero.

Actualización de Lecturas: En la función readFlexSensors(int voltages[]), se actualizan las lecturas con cada nueva entrada, ajustando la suma y calculando la media.
*/
#include <Arduino.h>

// Definir los pines analógicos conectados a los sensores flex
const int flexSensorPins[5] = {36, 39, 35, 32, 34}; // Actualizar con los números de pines correctos

// Definir el pin del botón
const int buttonPin = 0; // GPIO0

// Variables para el manejo del botón
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Letras del alfabeto
const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int currentLetterIndex = 0;

// Tamaño de la ventana para el filtro de media móvil
const int filterWindowSize = 10;  // Tamaño de la ventana de la media móvil
int flexReadings[5][filterWindowSize];  // Almacenar las lecturas recientes de los sensores
int readingIndex[5] = {0};  // Índices de las ventanas de media móvil
long flexSums[5] = {0};  // Suma de las lecturas recientes para cada sensor

// Función para inicializar los arrays de lecturas
void initFlexReadings() {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < filterWindowSize; j++) {
      flexReadings[i][j] = 0;
    }
  }
}

// Función para leer los voltajes de los sensores flex con filtro de media móvil
void readFlexSensors(int voltages[]) {
  for (int i = 0; i < 5; i++) {
    int currentValue = analogRead(flexSensorPins[i]);

    // Actualizar la suma eliminando la lectura más antigua y añadiendo la nueva
    flexSums[i] -= flexReadings[i][readingIndex[i]];
    flexReadings[i][readingIndex[i]] = currentValue;
    flexSums[i] += currentValue;

    // Calcular el valor promedio
    voltages[i] = flexSums[i] / filterWindowSize;

    // Mover al siguiente índice de la ventana
    readingIndex[i] = (readingIndex[i] + 1) % filterWindowSize;
  }
}

// Función para manejar el botón con debounce
void handleButton() {
  int reading = digitalRead(buttonPin);

  if (reading == LOW && !buttonPressed) { // Botón presionado
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      buttonPressed = true;
      lastDebounceTime = currentTime;
    }
  } else if (reading == HIGH && buttonPressed) { // Botón liberado
    buttonPressed = false;
    currentLetterIndex++;
    if (currentLetterIndex >= sizeof(alphabet) - 1) {
      currentLetterIndex = 0; // Reiniciar al principio del alfabeto
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configurar pines de sensores flex como entradas
  for (int i = 0; i < 5; i++) {
    pinMode(flexSensorPins[i], INPUT);
  }

  // Configurar el pin del botón como entrada con pull-up interno
  pinMode(buttonPin, INPUT_PULLUP);

  // Inicializar los arrays de lecturas para el filtro
  initFlexReadings();
}

void loop() {
  handleButton();

  // Leer voltajes de los sensores flex con filtro de media móvil
  int voltages[5];
  readFlexSensors(voltages);

  // Imprimir la letra actual y sus voltajes asociados al presionar el botón
  if (buttonPressed) {
    Serial.print("Letra: ");
    Serial.println(alphabet[currentLetterIndex]);
    Serial.print("Voltajes: ");
    for (int i = 0; i < 5; i++) {
      Serial.print(voltages[i]);
      if (i < 4) {
        Serial.print(", ");
      }
    }
    Serial.println();
    delay(500); // Pequeña demora para evitar lecturas múltiples
  }

  delay(100); // Ajustar según sea necesario
}
