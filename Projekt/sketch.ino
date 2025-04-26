/**
 * @file sketch.ino
 * @brief Upravljanje alarmnim sustavom temeljenim na PIR senzoru uz mogućnost kontrole putem IR daljinskog upravljača.
 *
 * @details
 * Ovaj program omogućava:
 * - Detekciju pokreta pomoću PIR senzora
 * - Aktivaciju alarma (LED + buzzer) pri detekciji pokreta
 * - Gašenje alarma pomoću fizičkog gumba ili IR daljinskog
 * - Upravljanje sleep modom za uštedu energije
 * - Pokretanje i gašenje cijelog sustava putem IR daljinskog (PLAY/POWER tipke)
 * - Prikaz stanja na LCD zaslonu
 *
 * @author
 * Maja Vignjević - mvignjevi@tvz.hr
 */

#include <LiquidCrystal_I2C.h> ///< Knjižnica za upravljanje LCD 20x4 preko I2C sučelja
#include <IRremote.hpp>        ///< Knjižnica za upravljanje IR daljinskim prijemom
#include <avr/sleep.h>         ///< Knjižnica za upravljanje sleep modom procesora
#include <avr/interrupt.h>     ///< Knjižnica za konfiguraciju prekida

#define WOKWI_SIMULACIJA ///< Definicija za rad u Wokwi simulatoru (onemogućava stvarni sleep)

/** @name Pinovi uređaja
 * Definicije svih digitalnih pinova korištenih u sustavu.
 */
///@{
const int ledPin = 6;    ///< Digitalni pin na kojem je spojena LED dioda
const int buzzerPin = 5; ///< Digitalni pin na kojem je spojen buzzer
const int pirPin = 4;    ///< Digitalni pin na kojem je spojen PIR senzor
const int irPin = 3;     ///< Digitalni pin za IR prijemnik
const int buttonPin = 2; ///< Digitalni pin za fizički gumb (INPUT_PULLUP)
///@}

/**
 * @brief LCD objekt za prikaz poruka korisniku (20x4 znakova, I2C adresa 0x27).
 */
LiquidCrystal_I2C lcd(0x27, 20, 4);

/** @name Statusne zastavice sustava
 * Varijable koje čuvaju trenutno stanje sustava i signale za akcije.
 */
///@{
volatile bool wakeUpFlag = false;     ///< Zastavica koju postavlja PIR prekid za buđenje iz sleep moda
bool zahtjevZaGasenjeGumbom = false;  ///< Zastavica za gašenje alarma putem gumba
bool zahtjevZaGasenjeIR = false;      ///< Zastavica za gašenje alarma putem IR daljinskog (tipka 1)
bool alarmAktivan = false;            ///< Trenutno stanje alarma (aktiviran / deaktiviran)
bool stanje = false;                  ///< Trenutno stanje LED diode i buzzera (blinking efekt)
bool sustavPokrenut = false;          ///< Zastavica je li sustav pokrenut IR PLAY tipkom
bool zahtjevZaGasenjeSustava = false; ///< Zastavica za potpuno gašenje sustava putem IR POWER tipke
///@}

/** @name Definirani IR kodovi
 * Konstantne vrijednosti za interpretaciju pritisnutih tipki na daljinskom.
 */
///@{
const uint32_t IR_KOD_PALJENJE = 0x57A8FF00;       ///< IR kod za PLAY tipku (pokretanje sustava)
const uint32_t IR_KOD_GASENJE = 0x5DA2FF00;        ///< IR kod za POWER tipku (gašenje cijelog sustava)
const uint32_t IR_KOD_GASENJE_ALARMA = 0xCF30FF00; ///< IR kod za tipku 1 (gašenje samo aktivnog alarma)
///@}

/** @name Varijable za mjerenje vremena
 * Pomoćne varijable za kontrolu vremena događaja u sustavu.
 */
///@{
unsigned long prethodno = 0;              ///< Vrijeme zadnje promjene stanja LED/buzzera za efekt blinkanja
const unsigned long interval = 300;       ///< Interval blinkanja LED/buzzera u milisekundama
unsigned long zadnjeVrijemeDetekcije = 0; ///< Vrijeme zadnje registracije pokreta PIR senzorom
unsigned long zadnjeVrijemeIR = 0;        ///< Vrijeme posljednjeg primljenog IR signala
unsigned long zadnjeVrijemeGumba = 0;     ///< Vrijeme posljednjeg pritiska fizičkog gumba
///@}

/**
 * @brief Funkcija prekida koja se poziva pri buđenju sustava.
 *
 * @details
 * Kada PIR senzor detektira pokret (preko eksternog prekida),
 * ova funkcija postavlja zastavicu @c wakeUpFlag na @c true.
 * Glavna petlja (loop) koristi ovu zastavicu kako bi znala da treba aktivirati alarm.
 *
 * @note Funkcija treba biti što brža jer se izvršava unutar ISR (Interrupt Service Routine).
 */
void wakeUp()
{
    wakeUpFlag = true; ///< Postavljanje zastavice za buđenje iz sleep moda.
}

/**
 * @brief Resetira sustav nakon deaktivacije alarma.
 *
 * @details
 * Funkcija:
 * - Resetira sve zastavice vezane uz alarm (alarmAktivan, zahtjevi za gašenje)
 * - Prikazuje poruku "Stanje mirovanja" korisniku
 * - Gasi LCD pozadinsko svjetlo za štednju energije
 * - Čeka stabilizaciju PIR senzora (da prestane detektirati pokret)
 *
 * @note
 * Bez čekanja da PIR padne na LOW, mogla bi se dogoditi trenutna reaktivacija alarma.
 */
void resetStanje()
{
    alarmAktivan = false;           ///< Deaktivira stanje aktivnog alarma
    zahtjevZaGasenjeGumbom = false; ///< Briše zahtjev za gašenje gumba
    zahtjevZaGasenjeIR = false;     ///< Briše zahtjev za gašenje putem IR-a

    lcd.clear();                   ///< Briše sve s LCD zaslona
    lcd.setCursor(0, 0);           ///< Postavlja kursor na početak prvog reda
    lcd.print("Stanje mirovanja"); ///< Prikazuje korisniku status "Stanje mirovanja"
    delay(2000);                   ///< Drži poruku 2 sekunde vidljivom

    lcd.clear();       ///< Ponovno briše zaslon
    lcd.noBacklight(); ///< Gasi pozadinsko osvjetljenje radi uštede energije

    ///< Čeka da PIR prestane detektirati pokret
    while (digitalRead(pirPin) == HIGH)
    {
        delay(100); ///< Provjerava svakih 100 ms je li pokret prestao
    }
}

/**
 * @brief Funkcija za gašenje aktivnog alarma.
 *
 * @details
 * Funkcija deaktivira LED diodu i buzzer (isključuje ih),
 * briše sve s LCD zaslona, prikazuje poruku "Alarm ugasen"
 * te zadržava prikaz te poruke 1.5 sekundi prije nastavka rada.
 *
 * @note
 * Korisniku se daje vizualna potvrda da je alarm ugašen.
 */
void ugasiAlarm()
{
    Serial.println("<< Alarm iskljucen."); ///< Ispis u serijski monitor za potvrdu gašenja alarma

    digitalWrite(ledPin, LOW);    ///< Gasi LED diodu
    digitalWrite(buzzerPin, LOW); ///< Gasi buzzer

    lcd.clear();               ///< Čisti sadržaj LCD ekrana
    lcd.setCursor(0, 0);       ///< Postavlja kursor na početak prvog reda
    lcd.print("Alarm ugasen"); ///< Ispisuje poruku korisniku o gašenju alarma

    delay(1500); ///< Kratko zadržavanje poruke 1.5 sekundi
}

/**
 * @brief Funkcija za ulazak sustava u sleep mode radi uštede energije.
 *
 * @details
 * Kada se koristi stvarni hardver:
 * - Mikrokontroler ulazi u Power-down sleep mode gdje prestaje s radom do prekida (PIR).
 *
 * Kada se koristi Wokwi simulator:
 * - Sleep mode se simulira aktivnim čekanjem da PIR senzor registrira pokret
 * - Tijekom čekanja, funkcija detektira i IR signal za gašenje sustava (IR POWER tipka)
 *
 * @note
 * Makronaredba @c WOKWI_SIMULACIJA odlučuje koristi li se stvarni ili simulirani sleep način rada.
 */
void enterSleep()
{
#ifndef WOKWI_SIMULACIJA
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); ///< Postavljanje najdubljeg sleep moda
    sleep_enable();                      ///< Omogućavanje sleep moda
    sleep_mode();                        ///< Ulazak u sleep stanje (mikrokontroler se zaustavlja)
    sleep_disable();                     ///< Onemogućavanje sleep moda nakon buđenja
#else
    Serial.println("-- Simulacija sleep moda: cekam PIR..."); ///< Ispis informativne poruke za Wokwi simulaciju

    ///< Aktivno čekanje dok PIR ne registrira pokret ili ne primimo IR naredbu za gašenje
    while (digitalRead(pirPin) == LOW && !zahtjevZaGasenjeSustava)
    {
        delay(10); ///< Kratko čekanje za rasterećenje CPU-a

        ///< Provjera ima li novih IR signala
        if (IrReceiver.decode())
        {
            uint32_t kod = IrReceiver.decodedIRData.decodedRawData; ///< Spremanje primljenog IR koda

            Serial.print(">> IR signal detektiran. Kod: 0x");
            Serial.println(kod, HEX);

            if (kod == IR_KOD_GASENJE)
            {
                zahtjevZaGasenjeSustava = true; ///< Postavlja se zahtjev za gašenje sustava
                Serial.println(">> Sustav isključen (IR POWER) tijekom cekanja PIR!");
            }
            IrReceiver.resume(); ///< Priprema IR prijemnika za sljedeći signal
        }
    }

    wakeUpFlag = true; ///< Postavljanje zastavice za buđenje iz simuliranog sleepa
#endif
}

/**
 * @brief Inicijalizacija sustava prilikom pokretanja.
 *
 * @details
 * Ova funkcija inicijalizira:
 * - Serijsku komunikaciju za debug poruke
 * - Definicije načina rada svih pinova (INPUT, OUTPUT)
 * - LCD zaslon za ispis statusa
 * - IR prijemnik za primanje signala s daljinskog upravljača
 * - Eksterni prekid za PIR senzor (ako nije simulacija)
 *
 * Također ispisuje početnu poruku "Čekanje PLAY IR" na LCD zaslon.
 */
void setup()
{
    Serial.begin(9600); ///< Pokretanje serijske komunikacije pri brzini 9600 bps

    pinMode(ledPin, OUTPUT);          ///< Postavljanje pina LED diode kao izlaznog
    pinMode(buzzerPin, OUTPUT);       ///< Postavljanje pina buzzera kao izlaznog
    pinMode(pirPin, INPUT);           ///< Postavljanje pina PIR senzora kao ulaznog
    pinMode(buttonPin, INPUT_PULLUP); ///< Postavljanje pina gumba kao ulaznog s internim pull-up otpornikom
    pinMode(irPin, INPUT);            ///< Postavljanje pina IR prijemnika kao ulaznog

    lcd.init();      ///< Inicijalizacija LCD zaslona
    lcd.backlight(); ///< Uključivanje pozadinskog osvjetljenja LCD-a

    lcd.setCursor(0, 0);          ///< Postavljanje kursora na prvi red, prvi stupac LCD-a
    lcd.print("Cekanje PLAY IR"); ///< Ispis početne poruke korisniku: čekanje IR PLAY signala

    IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK); ///< Pokretanje IR prijemnika s uključenim LED feedbackom
    Serial.println("IR prijemnik omogućen.");     ///< Informativna poruka o uspješnoj inicijalizaciji IR prijemnika
    Serial.println("Sustav spreman.");            ///< Poruka o završetku inicijalizacije sustava

#ifndef WOKWI_SIMULACIJA
    attachInterrupt(digitalPinToInterrupt(pirPin), wakeUp, RISING); ///< Postavljanje eksternog interrupta za PIR senzor (na RISING brid)
#endif
}

/**
 * @brief Glavna petlja sustava.
 *
 * @details
 * Funkcija neprekidno izvršava sljedeće:
 * - Provjerava zahtjev za gašenje cijelog sustava (IR POWER tipkom)
 * - Čita stanje fizičkog gumba
 * - Sprema trenutno i prethodno stanje gumba za detekciju pritiska
 */
void loop()
{
    /**
     * @brief Provjera zahtjeva za isključivanje cijelog sustava.
     *
     * Ako je primljen zahtjev za gašenje (IR POWER) i sustav je pokrenut, sustav se isključuje:
     * - Briše LCD ekran
     * - Prikazuje poruku o gašenju sustava
     * - Resetira sve kontrolne varijable
     * - Gasi LED i buzzer
     * - Izlazi iz funkcije loop()
     */
    if (zahtjevZaGasenjeSustava && sustavPokrenut)
    {
        Serial.println(">> Sustav isključen (IR POWER)."); ///< Ispis poruke o isključenju sustava
        lcd.clear();                                       ///< Brisanje trenutnog sadržaja LCD zaslona
        lcd.setCursor(0, 0);                               ///< Postavljanje kursora na početak LCD-a
        lcd.print("Sustav ugasen");                        ///< Prikaz poruke o isključenju sustava
        delay(3000);                                       ///< Pauza kako bi korisnik mogao pročitati poruku
        lcd.clear();                                       ///< Ponovno čišćenje LCD zaslona
        lcd.noBacklight();                                 ///< Gašenje pozadinskog osvjetljenja LCD zaslona

        //! Resetiranje stanja sustava
        sustavPokrenut = false;          ///< Sustav više nije pokrenut
        alarmAktivan = false;            ///< Alarm se gasi
        wakeUpFlag = false;              ///< Resetiranje buđenja
        zahtjevZaGasenjeSustava = false; ///< Resetiranje zahtjeva za gašenje

        //! Gašenje svih signalnih uređaja
        digitalWrite(ledPin, LOW);    ///< Gašenje LED diode
        digitalWrite(buzzerPin, LOW); ///< Gašenje buzzera

        return; ///< Izlazak iz funkcije loop() — ništa dalje se ne izvršava
    }

    /**
     * @brief Čitanje stanja fizičkog gumba.
     *
     * Očitava se trenutačno stanje gumba:
     * - LOW = pritisnut
     * - HIGH = nije pritisnut (INPUT_PULLUP logika)
     */
    int gumbStanje = digitalRead(buttonPin);

    /**
     * @brief Spremanje prethodnog stanja gumba.
     *
     * Potrebno za detekciju "falling edge" događaja (s visoke na nisku razinu).
     *
     * @note
     * Inicijalno postavljeno na HIGH jer gumb nije pritisnut (zbog pull-upa).
     */
    static bool prosloStanjeGumba = HIGH;

    /**
     * @brief Spremanje trenutnog stanja gumba.
     *
     * Koristi se za usporedbu s prethodnim stanjem.
     */
    bool trenutnoStanjeGumba = gumbStanje;

    /**
     * @brief Detekcija pritiska fizičkog gumba (falling edge detekcija).
     *
     * @details
     * Provjerava prijelaz stanja gumba s HIGH na LOW, što znači da je korisnik pritisnuo gumb.
     * Ako je alarm trenutno aktivan, postavlja se zahtjev za njegovo gašenje.
     */
    if (prosloStanjeGumba == HIGH && trenutnoStanjeGumba == LOW)
    {
        zadnjeVrijemeGumba = millis(); ///< Bilježenje trenutnog vremena pritiska gumba

        /**
         * @brief Provjera je li alarm trenutno aktivan.
         *
         * Ako jest, postavlja se zastavica za gašenje alarma putem gumba.
         */
        if (alarmAktivan)
        {
            zahtjevZaGasenjeGumbom = true;                              ///< Postavlja zahtjev za gašenje alarma pritiskom gumba
            Serial.println(">> LOOP: Gumb pritisnut (aktivni alarm)."); ///< Informativni ispis za korisnika / debug
        }
    }

    /**
     * @brief Ažuriranje prethodnog stanja gumba.
     *
     * Sprema trenutno stanje gumba za sljedeću iteraciju loop() funkcije,
     * kako bi se i dalje mogla detektirati promjena stanja.
     */
    prosloStanjeGumba = trenutnoStanjeGumba;

    /**
     * @brief Obrada primljenih IR signala.
     *
     * @details
     * Dekodira se primljeni IR signal i provjerava kojem tipu naredbe pripada:
     * - Ako je IR_KOD_PALJENJE (PLAY tipka), pokreće sustav ako još nije aktiviran.
     * - Ako je IR_KOD_GASENJE (POWER tipka), postavlja zahtjev za gašenje cijelog sustava.
     * - Ako je IR_KOD_GASENJE_ALARMA (tipka 1) i alarm je aktivan, postavlja zahtjev za gašenje alarma.
     */
    if (IrReceiver.decode())
    {
        uint32_t kod = IrReceiver.decodedIRData.decodedRawData; ///< Spremanje dekodiranog IR koda

        Serial.print(">> IR signal detektiran. Kod: 0x");
        Serial.println(kod, HEX);

        /**
         * @brief Provjera je li primljena IR naredba za pokretanje sustava (PLAY tipka).
         */
        if (kod == IR_KOD_PALJENJE)
        {
            if (!sustavPokrenut)
            {                          ///< Pokreni sustav samo ako već nije pokrenut
                sustavPokrenut = true; ///< Označi da je sustav pokrenut
                wakeUpFlag = false;    ///< Resetiraj zastavicu za buđenje

                lcd.clear();
                lcd.backlight();
                lcd.setCursor(0, 0);
                lcd.print("Pokretanje sustava");

                /**
                 * @brief Animacija pokretanja - LED i buzzer blinkaju 3 sekunde.
                 */
                unsigned long startBlinkanja = millis();

                while (millis() - startBlinkanja < 3000)
                {
                    digitalWrite(ledPin, HIGH);
                    digitalWrite(buzzerPin, HIGH);
                    delay(250);
                    digitalWrite(ledPin, LOW);
                    digitalWrite(buzzerPin, LOW);
                    delay(250);
                }

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Sustav aktivan");
                delay(4000);

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Prelazak u");
                lcd.setCursor(0, 1);
                lcd.print("  stanje mirovanja");
                delay(2000);

                lcd.clear();
                lcd.noBacklight(); ///< Isključi LCD pozadinsko svjetlo radi štednje

                Serial.println(">> Sustav pokrenut (IR PLAY)."); ///< Informacija da je sustav pokrenut
            }
        }

        /**
         * @brief Provjera je li primljena IR naredba za gašenje cijelog sustava (POWER tipka).
         */
        else if (kod == IR_KOD_GASENJE)
        {
            zahtjevZaGasenjeSustava = true; ///< Postavljanje zahtjeva za gašenje sustava
        }

        /**
         * @brief Provjera je li primljena IR naredba za gašenje samo aktivnog alarma (tipka 1).
         */
        else if (kod == IR_KOD_GASENJE_ALARMA && alarmAktivan)
        {
            zahtjevZaGasenjeIR = true;                                      ///< Postavljanje zahtjeva za gašenje alarma putem IR
            Serial.println(">> LOOP: IR kod za gašenje ALARMA (tipka 1)."); ///< Informativna poruka
        }

        IrReceiver.resume(); ///< Priprema IR prijemnika za primanje novih signala
    }

    /**
     * @brief Provjera stanja pokrenutosti sustava.
     *
     * Ako sustav nije pokrenut putem IR PLAY tipke, izlazi iz funkcije @c loop() bez daljnje obrade.
     *
     * @note
     * Sustav neće reagirati na PIR, gumb ili IR signale dok nije aktivno pokrenut korisničkom naredbom.
     */
    if (!sustavPokrenut)
    {
        return; ///< Prekid funkcije - sustav čeka na pokretanje.
    }

    /**
     * @brief Aktivacija alarma nakon buđenja iz sleep moda.
     *
     * Ako alarm trenutno nije aktiviran, a zastavica za buđenje (PIR) je postavljena,
     * sustav prelazi u aktivno stanje alarma.
     */
    if (!alarmAktivan && wakeUpFlag)
    {
        wakeUpFlag = false; ///< Resetiranje zastavice buđenja da se spriječi višestruka aktivacija.

        zadnjeVrijemeDetekcije = millis(); ///< Spremanje trenutnog vremena detekcije za kasniju obradu.

        alarmAktivan = true; ///< Označavanje da je alarm sada aktivan.

        lcd.backlight(); ///< Paljenje pozadinskog svjetla LCD ekrana za prikaz alarma.

        Serial.println(">> Pokret detektiran. Alarm aktiviran."); ///< Informacija u serijskom monitoru o aktivaciji alarma.
    }

    /**
     * @brief Upravljanje aktivnim alarmom i njegovim gašenjem.
     *
     * Ako je alarm aktiviran:
     * - LED dioda i buzzer blinkaju u definiranim vremenskim intervalima.
     * - Ispisuju se odgovarajuće poruke upozorenja na LCD ekranu.
     * - Provjerava se zahtjev za gašenje alarma (preko gumba ili IR signala).
     */
    if (alarmAktivan)
    {
        unsigned long trenutno = millis(); ///< Dohvaćanje trenutnog vremena u milisekundama.

        /**
         * @brief Upravljanje blinkanjem LED/buzzera.
         *
         * Ako je proteklo dovoljno vremena od zadnje promjene stanja (definirano s @c interval),
         * mijenja se stanje LED diode i buzzera.
         */
        if (trenutno - prethodno >= interval)
        {
            prethodno = trenutno; ///< Ažurira vrijeme posljednje promjene.

            stanje = !stanje; ///< Inverzija stanja (ON/OFF) za LED i buzzer.

            digitalWrite(ledPin, stanje);    ///< Postavlja stanje LED diode.
            digitalWrite(buzzerPin, stanje); ///< Postavlja stanje buzzera.

            lcd.setCursor(0, 0);                                     ///< Postavlja kursor na početak prvog reda LCD-a.
            lcd.print(stanje ? "   UPOZORENJE" : "               "); ///< Prikazuje ili briše upozorenje o pokretu.

            lcd.setCursor(0, 1);                                             ///< Postavlja kursor na početak drugog reda LCD-a.
            lcd.print(stanje ? "Pokret detektiran" : "                   "); ///< Prikazuje ili briše opis pokreta.
        }

        /**
         * @brief Provjera zahtjeva za gašenje alarma.
         *
         * Ako je korisnik pritisnuo gumb ili poslao IR signal za gašenje:
         * - Alarm se gasi.
         * - Resetiraju se zastavice zahtjeva za gašenje.
         * - Sustav se vraća u stanje mirovanja.
         */
        if (zahtjevZaGasenjeGumbom || zahtjevZaGasenjeIR)
        {
            ugasiAlarm(); ///< Poziv funkcije za gašenje alarma.

            zahtjevZaGasenjeGumbom = false; ///< Resetiranje zahtjeva za gašenje putem gumba.
            zahtjevZaGasenjeIR = false;     ///< Resetiranje zahtjeva za gašenje putem IR signala.

            resetStanje(); ///< Povratak sustava u pasivno stanje (stanje mirovanja).
        }
    }

    /**
     * @brief Upravljanje ulaskom u sleep mode kada alarm nije aktivan.
     *
     * Ako alarm nije aktivan i PIR senzor detektira stanje mirovanja (LOW na PIR izlazu):
     * - Sustav se prebacuje u sleep mode (ili simulaciju sleepa).
     */
    else if (!alarmAktivan && digitalRead(pirPin) == LOW)
    {
        Serial.println("-- Ulazak u sleep mode..."); ///< Poruka o ulasku u sleep mode.

        delay(200); ///< Kratka pauza radi stabilizacije sustava.

        enterSleep(); ///< Poziv funkcije za ulazak u sleep mode.

        Serial.println("-- Probudili smo se!"); ///< Poruka o buđenju iz sleepa.
    }
}
