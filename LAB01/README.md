# Upravljanje višestrukim prekidima i njihovim prioritetima

## Općenito
U mapi se nalaze datoteke preuzete sa WokWi simulacije sa samim kodom(sketch.ino), datoteka spojenih komponenti za vizualni prikaz (diagram.json) i  datoteka sa detaljima o samom zadatku koju upravo čitate. Ona se sastoji od:
- cilja zadatka
- specifikacija zadatka
- Link na WokWi simulaciju 

## Cilj zadatka
Proučiti koncept višestrukih prekida i njihovih prioriteta na odabranom procesoru te implementirati program koji efikasno upravlja različitim događajima koristeći odgovarajuće strategije rukovanja prekidima.

## Zadatak

### 1. Implementacija višestrukih interrupta
- **Definirajte i implementirajte više prekidnih rutina** (*ISR – Interrupt Service Routines*) koje će reagirati na različite događaje, uključujući:
  - Pritisak različitih tipkala
  - Aktivaciju internih timera
  - Očitavanje različitih senzora
  - Serijsku komunikaciju ili druge vanjske prekide

### 2. Postavljanje prioriteta prekida
- **Postavite različite prioritete za prekide** kako biste osigurali da važniji događaji imaju prednost pri obradi.
- **Omogućite preklapanje prekida** (*nested interrupts*) ako razvojna platforma podržava tu funkcionalnost.

### 3. Efikasno upravljanje resursima
- **Spriječite konflikte pristupa resursima** korištenjem odgovarajućih mehanizama poput:
  - Semafora
  - Kritičnih sekcija
  - Zastavica (*flags*)
- **Minimizirajte vrijeme izvršavanja ISR funkcija** kako biste izbjegli blokade drugih prekida.

### 4. Demonstracija rada s vanjskim sklopovima
- **Koristite dodatne vanjske sklopove** (npr. senzore, tipkala, LED diode, serijske module ili eksterne kontrolere) kako biste demonstrirali rad višestrukih prekida u stvarnim situacijama.
- **Implementirajte logiku** koja pokazuje kako različiti prioriteti utječu na obradu događaja u sustavu.

### 5. Dokumentacija i testiranje
- **Jasno dokumentirajte** način rada vašeg programa, uključujući opis svakog prekida i njegovog prioriteta.
- **Provedite testove** kako biste osigurali da sustav pravilno reagira na različite događaje i da ne dolazi do sukoba između prekida.

## Dodatni zahtjevi (po izboru jedan)
- **Implementirati mehanizam detekcije i rješavanja situacija** gdje se višestruki prekidi javljaju istovremeno.
- **Korištenje DMA (*Direct Memory Access*)** ako je dostupno kako bi se smanjio utjecaj prekida na glavni procesor.

## WokWi simulacija
Ovaj zadatak riješen je pomoću Arduino Uno kontrolera, sadrži 2 LED diode, 2 otpornika od 220 ohma, 2 gumba i potenciometar.
Simulaciju se može pokrenuti na linku: https://wokwi.com/projects/426522381219313665
