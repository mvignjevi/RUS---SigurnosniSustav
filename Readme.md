# RUS - Sigurnosni Sustav

Ovaj repozitorij sadrži projektni zadatak i laboratorijske vježbe izrađene u sklopu kolegija _Razvoj ugradbenih sustava_ na Tehničkom veleučilištu u Zagrebu.

---

## Sadržaj

- [Projekt: Sigurnosni sustav](#projekt-sigurnosni-sustav)
- [Laboratorijske vježbe](#laboratorijske-vježbe)
- [Tehnologije](#tehnologije)
- [Član tima](#član-tima)

---

## Projekt: Sigurnosni sustav

Ovaj projekt predstavlja razvoj jednostavnog alarmnog sustava sa senzorom pokreta.

### Funkcionalnosti

- Detekcija pokreta pomoću PIR senzora
- Aktivacija alarma (buzzer, LED) pri detekciji pokreta
- Deaktivacija alarma putem fizičkog gumba ili IR daljinskog upravljača
- Optimizacija potrošnje energije korištenjem "sleep" modova

### Korištene komponente

- Mikrokontroler
- PIR senzor pokreta
- LED indikator
- Zvučni signalizator (buzzer)
- LCD zaslon za prikaz informacija
- IR prijemnik i daljinski upravljač
- Tipkalo za ručno gašenje alarma

Projekt je razvijan i testiran pomoću **Wokwi online simulatora**.

---

## Laboratorijske vježbe

Repozitorij također sadrži dvije laboratorijske vježbe:

### Labos-Zadatak1

- Rad s višestrukim prekidima: tipkala, tajmer, ultrazvučni senzor
- Simulacija u Wokwi simulatoru
- Upravljanje LED signalizacijom ovisno o udaljenosti i događajima prekida

### Labos-Zadatak2

- Upravljanje potrošnjom energije mikrokontrolera
- Korištenje različitih "sleep" modova
- Aktivacija sustava pomoću prekida iz stanja mirovanja

---

## Tehnologije

- **Arduino IDE**
- **C/C++ programski jezik**
- **Wokwi online simulator**
- **GitHub Actions** za automatsku generaciju Doxygen dokumentacije
- **Doxygen** i **Graphviz** za izradu tehničke dokumentacije

---

## Član tima

**Vignjević Maja**  
mvignjevi@tvz.hr
