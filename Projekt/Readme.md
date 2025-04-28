# Sigurnosni sustav za detekciju pokreta

## Opis projekta
Ovaj projekt predstavlja sigurnosni sustav temeljen na Arduino platformi koji koristi:

- PIR senzor za detekciju pokreta
- LED diodu i buzzer za aktivaciju alarma
- LCD zaslon za prikaz statusa
- Fizički gumb i IR daljinski upravljač za upravljanje alarmom i sustavom
- Sleep mode za štednju energije kada nema aktivnosti

Projekt je realiziran u okviru kolegija **Razvoj Ugradbenih Sustava** putem **Wokwi simulacije**.

---

## Tehnički zahtjevi

- Wokwi online Arduino simulator
- Arduino UNO kompatibilna pločica (u simulaciji)
- PIR senzor
- LED dioda
- Buzzer
- IR prijemnik
- Fizički gumb
- LCD zaslon 20x4 I2C
- IR daljinski upravljač

---

## Glavne funkcionalnosti

- Aktivacija alarma pri detekciji pokreta
- Gašenje alarma fizičkim gumbom ili IR daljinskim (tipka 1)
- Pokretanje sustava IR daljinskim (tipka PLAY)
- Gašenje cijelog sustava IR daljinskim (tipka POWER)
- Sleep mode kada nije detektiran pokret radi uštede energije
- LCD prikaz statusa sustava

---

## Arhitektura sustava

- Mikrokontroler čita ulaze s PIR senzora, IR prijemnika i gumba
- Aktuatori (LED, buzzer) se pale/gase ovisno o stanju sustava
- LCD zaslon prikazuje trenutno stanje sustava
- Sleep mode omogućava štednju energije

---

## Upute za korištenje

1. Pokrenuti simulaciju putem Wokwi linka.
2. Pritisnuti PLAY tipku na IR daljinskom za pokretanje sustava.
3. Detekcija pokreta automatski aktivira alarm (LED + buzzer).
4. Isključiti alarm pritiskom fizičkog gumba ili tipke 1 na IR daljinskom.
5. Isključiti cijeli sustav pritiskom na POWER tipku na daljinskom.

---

## Link na simulaciju

> [Otvorite projekt u Wokwi simulatoru](https://wokwi.com/projects/429157360097100801)


---

## Testiranje

Projekt je testiran isključivo u **Wokwi simulacijskom okruženju**.

Funkcionalnosti koje su uspješno potvrđene:

- Detekcija pokreta
- Aktivacija alarma
- Gašenje alarma gumbom i IR daljinskim
- Pokretanje i gašenje sustava putem IR upravljača
- Sleep mode prelazak i buđenje PIR signalom

---

## Budućnost razvoja

- Ugradnja kamere za naprednu video analizu pokreta
- Povezivanje sustava putem IoT tehnologija (Wi-Fi/Bluetooth)
- Upravljanje putem mobilne aplikacije
- Integracija sa pametnim kućama (Home Assistant, Google Home)
