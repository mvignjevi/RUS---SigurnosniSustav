#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

/**
 * @brief Pin na kojem je spojena LED dioda.
 */
const int ledPin = 13;

/**
 * @brief Pin na kojem je spojena tipka za buđenje.
 */
const int buttonPin = 2;

/**
 * @brief Zastavica koja označava da je Arduino probuđen gumbom.
 */
volatile bool wakeUpByButton = false;

/**
 * @brief Zastavica koja označava da je Arduino probuđen putem Watchdog timera.
 */
volatile bool wakeUpByWDT = false;

/**
 * @brief Vrijeme posljednjeg prekida (za debounce).
 */
volatile unsigned long lastInterruptTime = 0;

/**
 * @brief Inicijalna funkcija postavki: pinovi, serijska veza, watchdog i inicijalno paljenje LED-ice.
 */
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.begin(9600);
  delay(100); ///< Stabilizacija serijskog porta

  attachInterrupt(digitalPinToInterrupt(buttonPin), wakeUp, FALLING);

  setupWatchdog(); ///< Inicijalizacija Watchdog timera

  /// Početna faza: LED svijetli 5 sekundi
  digitalWrite(ledPin, HIGH);
  Serial.println("LED svijetli 5 sekundi na početku...");
  delay(5000);
  digitalWrite(ledPin, LOW);
  Serial.println("LED ugašena. Ulazim u sleep mod...");
}

/**
 * @brief Glavna petlja programa: upravlja sleep ciklusima i buđenjem iz različitih izvora.
 */
void loop() {
  /// Ako nema buđenja — uđi u sleep mod
  if (!wakeUpByButton && !wakeUpByWDT) {
    enterSleep();
  }

  /// Buđenje pomoću gumba
  if (wakeUpByButton) {
    wakeUpByButton = false;

    Serial.println("Probudio me gumb! LED svijetli 5 sekundi...");
    digitalWrite(ledPin, HIGH);
    delay(5000);
    digitalWrite(ledPin, LOW);

    /// Debounce: pričekaj da korisnik pusti gumb
    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }

    Serial.println("LED ugašena. Ponovno idem spavati...");
  }

  /// Buđenje pomoću Watchdog timera
  if (wakeUpByWDT) {
    wakeUpByWDT = false;

    Serial.println("Probudio me Watchdog! LED svijetli 1 sekundu.");
    digitalWrite(ledPin, HIGH);
    delay(1000);
    digitalWrite(ledPin, LOW);
    Serial.println("LED ugašena. Ponovno idem spavati...");
  }
}

/**
 * @brief Ulazak u sleep mod (Power-down).
 */
void enterSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();           ///< Onemogući prekide tijekom konfiguracije
  sleep_enable();  ///< Omogući sleep
  sei();           ///< Ponovno omogući prekide
  sleep_cpu();     ///< Uđi u sleep (Zzz...)
  sleep_disable(); ///< Onemogući sleep nakon buđenja
}

/**
 * @brief ISR (prekidna rutina) za buđenje putem gumba (eksterni prekid).
 * Uključuje softverski debounce.
 */
void wakeUp() {
  unsigned long currentTime = millis();

  /// Debounce: dozvoli prekid samo ako je prošlo 250ms od prethodnog
  if (currentTime - lastInterruptTime > 250) {
    wakeUpByButton = true;
    lastInterruptTime = currentTime;
  }
}

/**
 * @brief ISR za buđenje putem Watchdog timera (bez resetiranja).
 */
ISR(WDT_vect) {
  wakeUpByWDT = true;
}

/**
 * @brief Inicijalizacija Watchdog timera za buđenje svakih 8 sekundi.
 * Konfigurira WDT da generira prekid (ne reset) svake 8 sekundi.
 */
void setupWatchdog() {
  MCUSR &= ~(1 << WDRF); ///< Očisti WDT reset flag

  // Omogući izmjenu postavki
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Postavi WDT za interrupt samo (bez reseta), 8s timeout
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0); ///< 8 sekundi (najdulje moguće)
}
