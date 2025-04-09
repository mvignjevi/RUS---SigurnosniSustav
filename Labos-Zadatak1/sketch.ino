#include <avr/io.h>
#include <avr/interrupt.h>

/**
 * @file
 * @brief Projekt koji koristi više izvora prekida (vanjski, pin change, tajmer) i senzor udaljenosti
 * za upravljanje LED diodama različitih prioriteta.
 *
 * @author Maja Vignjević
 * @date 5.4.2025.
 */

/**
 * @brief Pin za crvenu LED diodu s visokim prioritetom.
 */
#define LED_0 8
/**
 * @brief Pin za žutu LED diodu sa srednjim prioritetom.
 */
#define LED_1 9
/**
 * @brief Pin za zelenu LED diodu s niskim prioritetom.
 */
#define LED_2 10
/**
 * @brief Pin za crvenu LED diodu za udaljenost manju ili jednaku 20 cm.
 */
#define LED_DIST_RED 11
/**
 * @brief Pin za žutu LED diodu za udaljenost manju ili jednaku 50 cm.
 */
#define LED_DIST_YELLOW 12
/**
 * @brief Pin za zelenu LED diodu za udaljenost manju ili jednaku 100 cm.
 */
#define LED_DIST_GREEN 13

/**
 * @brief Pin za crveni gumb visokog prioriteta (INT0).
 */
#define BTN_0 2
/**
 * @brief Pin za žuti gumb srednjeg prioriteta (INT1).
 */
#define BTN_1 3
/**
 * @brief Pin za zeleni gumb niskog prioriteta (pin change).
 */
#define BTN_2 4

/**
 * @brief Pin za trigger HC-SR04 senzora udaljenosti.
 */
#define TRIG 6
/**
 * @brief Pin za echo HC-SR04 senzora udaljenosti.
 */
#define ECHO 7

/// Stanje prekida za niski prioritet (zeleni gumb).
volatile bool btnLowPressed = false;
/// Stanje prekida za srednji prioritet (žuti gumb).
volatile bool btnMedPressed = false;
/// Stanje prekida za visoki prioritet (crveni gumb).
volatile bool btnHighPressed = false;
/// Zastavica za aktivaciju mjerenja udaljenosti pomoću tajmera.
volatile bool timerFlag = false;
/// Trenutno izmjerena udaljenost.
volatile long distance = 0;

/// Vrijeme uključivanja visoko prioritetne LED.
unsigned long ledHighOnTime = 0;
/// Vrijeme uključivanja srednje prioritetne LED.
unsigned long ledMedOnTime = 0;
/// Vrijeme uključivanja nisko prioritetne LED.
unsigned long ledLowOnTime = 0;
/// Vrijeme uključivanja crvene LED za udaljenost.
unsigned long ledRedDistTime = 0;
/// Vrijeme uključivanja žute LED za udaljenost.
unsigned long ledYellowDistTime = 0;
/// Vrijeme uključivanja zelene LED za udaljenost.
unsigned long ledGreenDistTime = 0;

/// Trajanje osvijetljenja visoko prioritetne LED u ms.
const unsigned long HIGH_DURATION = 3000;
/// Trajanje osvijetljenja srednje prioritetne LED u ms.
const unsigned long MED_DURATION = 2000;
/// Trajanje osvijetljenja nisko prioritetne LED u ms.
const unsigned long LOW_DURATION = 1000;
/// Trajanje osvijetljenja LED dioda za udaljenost u ms.
const unsigned long DIST_DURATION = 1000;

/**
 * @brief Inicijalizacija komponenti, pinova, prekida i tajmera.
 */
void setup() {
  Serial.begin(9600);

  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_DIST_RED, OUTPUT);
  pinMode(LED_DIST_YELLOW, OUTPUT);
  pinMode(LED_DIST_GREEN, OUTPUT);

  pinMode(BTN_0, INPUT_PULLUP);
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  /// Vanjski prekidi
  attachInterrupt(digitalPinToInterrupt(BTN_0), btnHighISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_1), btnMedISR, FALLING);

  /// Pin change prekid za BTN_2
  PCICR |= (1 << PCIE2);      /**< Omogućavanje prekida za PORTD */
  PCMSK2 |= (1 << PCINT20);   /**< Omogućavanje prekida za pin D4 */

  /// Tajmer1 konfiguracija za prekid svake 1 sekunde
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12);  /**< CTC mod, prescaler 256 */
  OCR1A = 62500;  /**< 16MHz / 256 / 1Hz = 62500 */
  TIMSK1 = (1 << OCIE1A);

  sei();  /// Globalno omogućavanje prekida
}

/**
 * @brief ISR rutina za visoki prioritet (crveni gumb).
 */
void btnHighISR() {
  btnHighPressed = true;
}

/**
 * @brief ISR rutina za srednji prioritet (žuti gumb).
 */
void btnMedISR() {
  btnMedPressed = true;
}

/**
 * @brief ISR rutina za niski prioritet (zeleni gumb - pin change).
 */
ISR(PCINT2_vect) {
  if (digitalRead(BTN_2) == LOW) {
    btnLowPressed = true;
  }
}

/**
 * @brief ISR rutina za tajmer – postavlja zastavicu za mjerenje udaljenosti.
 */
ISR(TIMER1_COMPA_vect) {
  timerFlag = true;
}

/**
 * @brief Funkcija za mjerenje udaljenosti pomoću HC-SR04 senzora.
 *
 * @return Udaljenost u centimetrima.
 */
long measureDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  long dist = (duration / 2) * 0.0344;
  return dist;
}

/**
 * @brief Glavna petlja programa.
 *
 * Upravlja LED diodama na temelju stanja prekidača i izmjerene udaljenosti.
 */
void loop() {
  unsigned long currentTime = millis();

  if (btnHighPressed) {
    digitalWrite(LED_0, HIGH);
    ledHighOnTime = currentTime;
    btnHighPressed = false;
  }

  if (btnMedPressed) {
    digitalWrite(LED_1, HIGH);
    ledMedOnTime = currentTime;
    btnMedPressed = false;
  }

  if (btnLowPressed) {
    digitalWrite(LED_2, HIGH);
    ledLowOnTime = currentTime;
    btnLowPressed = false;
  }

  /// Gašenje LED dioda po isteku vremena
  if ((currentTime - ledHighOnTime) >= HIGH_DURATION && ledHighOnTime != 0) {
    digitalWrite(LED_0, LOW);
    ledHighOnTime = 0;
  }

  if ((currentTime - ledMedOnTime) >= MED_DURATION && ledMedOnTime != 0) {
    digitalWrite(LED_1, LOW);
    ledMedOnTime = 0;
  }

  if ((currentTime - ledLowOnTime) >= LOW_DURATION && ledLowOnTime != 0) {
    digitalWrite(LED_2, LOW);
    ledLowOnTime = 0;
  }

  if ((currentTime - ledRedDistTime) >= DIST_DURATION && ledRedDistTime != 0) {
    digitalWrite(LED_DIST_RED, LOW);
    ledRedDistTime = 0;
  }

  if ((currentTime - ledYellowDistTime) >= DIST_DURATION && ledYellowDistTime != 0) {
    digitalWrite(LED_DIST_YELLOW, LOW);
    ledYellowDistTime = 0;
  }

  if ((currentTime - ledGreenDistTime) >= DIST_DURATION && ledGreenDistTime != 0) {
    digitalWrite(LED_DIST_GREEN, LOW);
    ledGreenDistTime = 0;
  }

  /// Mjerenje udaljenosti svakih 1s pomoću tajmera
  if (timerFlag) {
    distance = measureDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance <= 100) {
      digitalWrite(LED_DIST_GREEN, HIGH);
      ledGreenDistTime = currentTime;
    }
    if (distance <= 50) {
      digitalWrite(LED_DIST_YELLOW, HIGH);
      ledYellowDistTime = currentTime;
    }
    if (distance <= 20) {
      digitalWrite(LED_DIST_RED, HIGH);
      ledRedDistTime = currentTime;
    }

    timerFlag = false;
  }
}
