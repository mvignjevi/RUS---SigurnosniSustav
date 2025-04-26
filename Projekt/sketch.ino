/**
 * @file AlarmSystem.ino
 * @brief Sustav za detekciju pokreta pomoću PIR senzora s alarmom i podrškom za sleep mode.
 * 
 * Glavne funkcije:
 * - Detekcija pokreta pomoću PIR senzora
 * - Aktivacija alarma (LED + buzzer) pri detekciji
 * - Gašenje alarma pomoću gumba ili IR daljinskog
 * - Sleep mode za štednju energije
 * - LCD ispis statusa sustava
 * 
 * @author 
 * Maja Vignjević (uz asistenciju ArduinoGPT | Code Wizzard)
 */

 #include <LiquidCrystal_I2C.h>
 #include <IRremote.hpp>
 #include <avr/sleep.h>
 #include <avr/interrupt.h>
 
 #define WOKWI_SIMULACIJA ///< Definirati samo tijekom simulacije u Wokwi-u
 
 const int ledPin = 6;       ///< Pin LED diode
 const int buzzerPin = 5;    ///< Pin buzzera
 const int pirPin = 4;       ///< Pin PIR senzora
 const int irPin = 3;        ///< Pin IR prijemnika
 const int buttonPin = 2;    ///< Pin fizičkog gumba
 
 LiquidCrystal_I2C lcd(0x27, 20, 4); ///< LCD 20x4 display
 
 volatile bool wakeUpFlag = false; ///< Zastavica za buđenje iz sleep moda
 bool zahtjevZaGasenjeGumbom = false; ///< Zastavica za gašenje alarma gumbom
 bool zahtjevZaGasenjeIR = false; ///< Zastavica za gašenje alarma IR daljinskim
 bool alarmAktivan = false; ///< Trenutno stanje alarma
 bool stanje = false; ///< Trenutno stanje LED/buzzera (ON/OFF)
 
 unsigned long prethodno = 0; ///< Vrijeme zadnje promjene LED/buzzera
 const unsigned long interval = 300; ///< Interval blinkanja LED/buzzera
 unsigned long zadnjeVrijemeDetekcije = 0; ///< Vrijeme zadnje PIR detekcije
 unsigned long zadnjeVrijemeIR = 0; ///< Vrijeme zadnjeg IR signala
 unsigned long zadnjeVrijemeGumba = 0; ///< Vrijeme zadnjeg pritiska gumba
 
 /**
  * @brief Funkcija koja se poziva pri buđenju iz sleep moda.
  * 
  * Funkcija postavlja zastavicu da je PIR senzor aktivirao sustav.
  */
 void wakeUp() {
   wakeUpFlag = true;
 }

/**
 * @brief Resetira stanje sustava nakon deaktivacije alarma.
 * 
 * Funkcija izvršava:
 * - Reset svih internih zastavica povezanih s alarmom:
 *   - @c alarmAktivan postaje @c false
 *   - @c zahtjevZaGasenjeGumbom i @c zahtjevZaGasenjeIR postaju @c false
 * - Upravljanje LCD ekranom:
 *   - Prikazuje korisniku poruku "Stanje mirovanja" tijekom 2 sekunde
 *   - Briše ekran i gasi pozadinsko osvjetljenje radi uštede energije
 * - Stabilizaciju PIR senzora:
 *   - Čeka da PIR senzor više ne registrira pokret (OUT pin u LOW stanju)
 * 
 * @note
 * Stabilizacija PIR senzora je ključna da bi se spriječila lažna reaktivacija alarma
 * odmah nakon što je prethodni pokret detektiran i alarm ugašen.
 */
void resetStanje() {
    Serial.println("-- Reset uvjeta detekcije");

    //! Resetiramo statusne zastavice alarma
    alarmAktivan = false;            //!< Alarm više nije aktivan
    zahtjevZaGasenjeGumbom = false;  //!< Gumb za gašenje više nije potreban
    zahtjevZaGasenjeIR = false;      //!< IR za gašenje više nije potreban

    //! Prikazujemo korisniku da je sustav u stanju mirovanja
    lcd.clear();                     //!< Brišemo prethodni sadržaj s LCD ekrana
    lcd.setCursor(0, 0);              //!< Postavljamo kursor na početak LCD-a
    lcd.print("Stanje mirovanja");    //!< Ispisujemo poruku o mirovanju
    delay(2000);                      //!< Držimo poruku vidljivu 2 sekunde

    //! Gasi se LCD ekran radi štednje energije
    lcd.clear();                      //!< Ponovno brišemo LCD ekran
    lcd.noBacklight();                //!< Isključujemo pozadinsko osvjetljenje LCD-a

    //! Čekamo da se PIR senzor stabilizira (da više nema pokreta)
    Serial.println("-- Čekanje da PIR senzor prestane detektirati pokret...");
    while (digitalRead(pirPin) == HIGH) { 
        delay(100);                   //!< Mali delay kako ne bi nepotrebno opterećivali CPU
    }
    Serial.println("-- PIR senzor se smirio."); //!< Potvrda da je senzor stabilan
}

/**
 * @brief Funkcija za gašenje alarma i prijelaz sustava u pasivno stanje.
 *
 * Ova funkcija deaktivira sve vizualne (LED) i zvučne (buzzer) signale alarma,
 * briše prethodne poruke sa LCD zaslona te prikazuje korisniku informaciju
 * da je alarm isključen. Nakon kratkog vremenskog odmaka, sustav prelazi u
 * stanje čekanja (mirovanja).
 *
 * @note Funkcija uključuje kratku pauzu od 1.5 sekunde kako bi korisnik stigao 
 *       vidjeti obavijest prije nego što se LCD očisti ili isključi.
 */
void ugasiAlarm() {
    Serial.println("<< Alarm iskljucen."); //! Serijski ispis statusa gašenja alarma za debug.

    digitalWrite(ledPin, LOW);             //! Isključivanje LED diode (vizualni signal).
    digitalWrite(buzzerPin, LOW);           //! Isključivanje buzzera (zvučni signal).

    lcd.clear();                            //! Čišćenje LCD zaslona od prethodnih poruka.
    lcd.setCursor(0, 0);                    //! Pozicioniranje kursora na početak zaslona.
    lcd.print("Alarm ugasen");              //! Prikaz poruke o isključenom alarmu.

    delay(1500);                            //! Pauza od 1500 ms za prikaz korisniku prije daljnjih radnji.
}

/**
 * @brief Funkcija za ulazak sustava u sleep mode radi štednje energije.
 * 
 * Funkcija stavlja mikrokontroler u najdublji režim spavanja (Power-down),
 * gdje svi moduli prestaju raditi osim onih koji su potrebni za buđenje (PIR).
 * 
 * Kada se kod izvršava u Wokwi simulatoru, stvarno spavanje nije moguće, 
 * pa se umjesto toga koristi simulacija koja aktivno čeka PIR signal (HIGH).
 *
 * @note Makronaredba `WOKWI_SIMULACIJA` određuje da li se koristi pravo spavanje 
 *       ili simulirano čekanje na pokret.
 */
void enterSleep() {
    #ifndef WOKWI_SIMULACIJA
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);   //! Postavljanje režima najdubljeg spavanja - Power-down.
        sleep_enable();                        //! Omogućavanje mogućnosti spavanja procesora.
        sleep_mode();                          //! Ulazak u režim spavanja - procesor se zaustavlja ovdje.
        sleep_disable();                       //! Deaktiviranje sleep moda nakon buđenja interruptom.
    #else
        Serial.println("-- Simulacija sleep moda: cekam PIR..."); //! Debug poruka za Wokwi simulaciju.
    
        while (digitalRead(pirPin) == LOW) {    //! Aktivno čekanje da PIR senzor detektira pokret (HIGH).
            delay(10);                         //! Kratko čekanje da ne blokiramo procesor nepotrebno.
        }
    
        wakeUpFlag = true;                     //! Postavljanje zastavice za signalizaciju buđenja.
    #endif
}
    
/**
 * @brief Inicijalizacija sustava.
 * 
 * Postavlja konfiguraciju pinova, inicijalizira LCD zaslon,
 * pokreće IR prijemnik, simulira pokretanje sustava s LED i buzzer animacijom
 * te konfigurira eksterni interrupt za PIR senzor (ako nije simulacija).
 */
void setup() {
    Serial.begin(9600);
  
    /** 
     * @brief Postavljanje načina rada pinova.
     */
    pinMode(ledPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(pirPin, INPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(irPin, INPUT);
  
    /** 
     * @brief Inicijalizacija LCD zaslona i uključivanje pozadinskog svjetla.
     */
    lcd.init();
    lcd.backlight();
  
    /** 
     * @brief Prikaz poruke pokretanja sustava.
     */
    lcd.setCursor(0, 0);
    lcd.print("Pokretanje sustava");
  
    /**
     * @brief Animacija pokretanja sustava.
     * 
     * LED dioda i buzzer zajedno blinkaju 3 sekunde
     * kako bi vizualno i zvučno označili inicijalizaciju.
     */
    unsigned long startBlinkanja = millis();
    while (millis() - startBlinkanja < 3000) {
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
      delay(250);
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW);
      delay(250);
    }
  
    /** 
     * @brief Prikaz statusa "Sustav aktivan" na LCD zaslonu.
     */
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sustav aktivan");
    delay(4000);
  
    /** 
     * @brief Prikaz poruke prelaska u stanje mirovanja.
     */
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Prelazak u");
    lcd.setCursor(0, 1);
    lcd.print("  stanje mirovanja");
    delay(2000);
  
    /** 
     * @brief Gašenje pozadinskog svjetla LCD-a nakon inicijalizacije.
     */
    lcd.clear();
    lcd.noBacklight();
  
    /**
     * @brief Pokretanje IR prijemnika.
     */
    IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK);
    Serial.println("IR prijemnik omogućen.");
    Serial.println("Sustav spreman.");
  
  #ifndef WOKWI_SIMULACIJA
    /**
     * @brief Postavljanje eksternog prekida na PIR pin.
     * 
     * Prekid se aktivira na rastući brid (RISING) izlaza PIR senzora,
     * omogućujući buđenje iz sleep moda.
     */
    attachInterrupt(digitalPinToInterrupt(pirPin), wakeUp, RISING);
  #endif
}
  
/**
 * @brief Glavna petlja sustava.
 * 
 * Provjerava stanje gumba, IR signala i PIR detekciju,
 * upravlja aktivacijom i deaktivacijom alarma te prelascima u sleep mode.
 */
void loop() {
    /**
     * @brief Čitanje trenutnog stanja gumba.
     * 
     * 0 = gumb pritisnut (LOW zbog INPUT_PULLUP).
     * 1 = gumb nije pritisnut.
     */
    int gumbStanje = digitalRead(buttonPin);
  
    /**
     * @brief Pohrana prethodnog stanja gumba za detekciju "falling edge" događaja (otpuštanje-pritisak).
     * 
     * Inicijalno postavljeno na HIGH (nepritisnut gumb).
     */
    static bool prosloStanjeGumba = HIGH;
  
    /**
     * @brief Spremanje trenutno pročitanog stanja gumba za usporedbu s prethodnim stanjem.
     */
    bool trenutnoStanjeGumba = gumbStanje;

      /**
   * @brief Detekcija promjene stanja gumba (falling edge).
   * 
   * Ako je prethodno stanje bilo HIGH (gumb nije bio pritisnut), 
   * a trenutno stanje LOW (gumb sada pritisnut),
   * tada bilježimo trenutak pritiska i pokrećemo odgovarajuću akciju.
   */
  if (prosloStanjeGumba == HIGH && trenutnoStanjeGumba == LOW) {
    
    /**
     * @brief Bilježenje vremena pritiska gumba.
     * 
     * Koristi se za eventualno debouncing ili kasnije provjere trajanja pritiska.
     */
    zadnjeVrijemeGumba = millis();

    /**
     * @brief Provjera da li je alarm aktivan.
     * 
     * Ako je aktivan, postavlja se zastavica za gašenje alarma preko gumba.
     */
    if (alarmAktivan) {
      zahtjevZaGasenjeGumbom = true;
      Serial.println(">> LOOP: Gumb pritisnut (aktivni alarm).");
    } 
    else {
      Serial.println(">> LOOP: Gumb pritisnut (ignorisano – alarm nije aktivan).");
    }
    }

    /**
     * @brief Ažuriranje prethodnog stanja gumba za sljedeću iteraciju.
     */
    prosloStanjeGumba = trenutnoStanjeGumba;

        /**
     * @brief Provjerava postoji li novi IR signal i reagira na prihvaćeni kod.
     * 
     * Dekodira primljeni IR signal. Ako je kod prepoznat kao kod za gašenje alarma
     * i prošlo je više od 500 ms od zadnje IR aktivnosti, postavlja zahtjev za gašenje alarma.
     */
    if (IrReceiver.decode()) {
        uint32_t kod = IrReceiver.decodedIRData.decodedRawData; ///< Spremanje dekodiranog IR signala
    
        Serial.print(">> IR signal detektiran. Kod: 0x");
        Serial.println(kod, HEX);
    
        if ((kod == 0xCF30FF00 || kod == 0xFFA25D) && millis() - zadnjeVrijemeIR > 500) {
        zahtjevZaGasenjeIR = true; ///< Postavljanje zastavice za gašenje alarma putem IR
        zadnjeVrijemeIR = millis(); ///< Ažuriranje vremena zadnjeg IR signala
        Serial.println(">> LOOP: IR kod prihvaćen.");
        }
    
        IrReceiver.resume(); ///< Priprema IR prijemnika za sljedeći signal
    }

    /**
     * @brief Aktivira alarm nakon buđenja iz sleep moda.
     * 
     * Ako sustav nije aktivirao alarm i ako je postavljena zastavica buđenja
     * (nakon detekcije pokreta PIR senzorom), pokreće se alarm.
     * Postavlja se vrijeme zadnje detekcije i omogućava pozadinsko svjetlo LCD ekrana.
     */
    if (!alarmAktivan && wakeUpFlag) {
        wakeUpFlag = false;               ///< Resetiranje zastavice buđenja
        zadnjeVrijemeDetekcije = millis(); ///< Pohranjivanje vremena detekcije pokreta
        alarmAktivan = true;               ///< Postavljanje stanja aktivnog alarma
        lcd.backlight();                   ///< Paljenje LCD pozadinskog osvjetljenja
        Serial.println(">> Pokret detektiran. Alarm aktiviran."); ///< Informacija o aktivaciji alarma
    }
  
    /**
 * @brief Upravljanje ponašanjem aktivnog alarma.
 * 
 * Kad je alarm aktiviran, LED dioda i buzzer blinkaju u određenim intervalima.
 * Ispisuje se upozorenje na LCD ekran.
 * Također provjerava zahtjev za gašenje alarma (bilo putem gumba ili IR daljinskog upravljača).
 */
if (alarmAktivan) {
    unsigned long trenutno = millis(); ///< Dohvaća trenutno vrijeme u milisekundama
  
    if (trenutno - prethodno >= interval) { ///< Provjerava je li prošlo dovoljno vremena za promjenu stanja
      prethodno = trenutno; ///< Ažurira vrijeme posljednje promjene
      stanje = !stanje; ///< Inverzira stanje LED diode i buzzera (ON/OFF)
  
      digitalWrite(ledPin, stanje); ///< Uključuje ili isključuje LED diodu ovisno o stanju
      digitalWrite(buzzerPin, stanje); ///< Uključuje ili isključuje buzzer ovisno o stanju
  
      lcd.setCursor(0, 0); ///< Postavlja kursor na početak prvog reda LCD-a
      lcd.print(stanje ? "   UPOZORENJE" : "               "); ///< Ispisuje upozorenje ili briše sadržaj reda
  
      lcd.setCursor(0, 1); ///< Postavlja kursor na početak drugog reda LCD-a
      lcd.print(stanje ? "Pokret detektiran" : "                   "); ///< Ispisuje poruku o detekciji pokreta ili briše red
    }
  
    if (zahtjevZaGasenjeGumbom || zahtjevZaGasenjeIR) { ///< Provjerava je li primljen zahtjev za gašenje alarma
        ugasiAlarm(); ///< Poziva funkciju za gašenje alarma
        zahtjevZaGasenjeGumbom = false; ///< Resetira zastavicu zahtjeva za gašenje preko gumba
        zahtjevZaGasenjeIR = false; ///< Resetira zastavicu zahtjeva za gašenje preko IR signala
        resetStanje(); ///< Resetira sustav u stanje mirovanja
        }
    }  
    /**
     * @brief Upravljanje ulaskom u sleep mod kad je sustav miran.
     * 
     * Ako alarm nije aktivan i PIR senzor ne detektira pokret (LOW stanje),
     * sustav ulazi u simulirani ili stvarni sleep mod ovisno o konfiguraciji.
     */
    else if (!alarmAktivan && digitalRead(pirPin) == LOW) {
        Serial.println("-- Ulazak u sleep mode..."); ///< Ispis informativne poruke o ulasku u sleep
        delay(200); ///< Kratka pauza prije ulaska u sleep zbog stabilizacije sustava
        enterSleep(); ///< Poziv funkcije za ulazak u sleep mod (simulirani ili stvarni)
        Serial.println("-- Probudili smo se!"); ///< Ispis poruke nakon buđenja iz sleep moda
    }  
}