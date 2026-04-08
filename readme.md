# Satel-KPD Custom Component for ESPHome

Komponent dla ESPHome pozwalający na dwukierunkową komunikację z centralą alarmową **CA-6** lub **CA-10** (oraz potencjalnie innymi) poprzez magistralę manipulatora **KPD, COM, DATA, CLK**.

### Funkcje:
* odczyt stanów wejść Zx,
* odczyt diod statusowych (zasilanie, telefon, awaria, buzzer, czuwanie, alarm),
* emulowanie manipulatora,
* automatyczny odczyt awarii,
* załączanie/wyłączanie czuwania.

---

## Hardware

Magistrala manipulatora działa na logice **10-16V** więc niezbędne jest zastosowanie konwertera poziomów logicznych.

### Wariant 1: podstawowy (tylko odczyt):
Niezbędny będzie minimum 2 kanałowy konwerter poziomów logicznych akceptujący napięcia do 16V (np. **Pololu 2595**). Konwerter obniży napięcia ~12V z szyn zegara (CLK) i danych (DATA) do poziomu 3.3V akceptowanego przez ESP. 

### Wariant 2: pełny (odczyt + emulacja)
Aby wysyłać sygnały do centrali, musimy dodatkowo mieć element wykonawczy, który będzie zwierał sygnał DATA do masy - wymagany będzie np. układu z **tranzystorem MOSFET** oraz **rezystorem 100Ω**.

Powstał też dedykowany układ zawierającego konwerter poziomów logicznych oraz moduł odpowiedzialny za nadawanie sygnałów.

<img src="https://github.com/ficueu/esphome-satel-kpd/blob/main/docs/konwerter.jpg" height="300">

---

## Podstawowa konfiguracja

```
satel_kpd:
  id: my_satel
  data_pin: GPIO4
  ckl_pin: GPIO3
  prs_pin: GPIO5
  simulated_keypad: false
  trouble_lang: pl
  variant: ca6
```

* **data_pin** (Wymagany, Pin): Pin wejściowy odczytujący dane z magistrali (DATA). 
* **ckl_pin** (Wymagany, Pin): Pin wejściowy odczytujący sygnał zegara z centrali (CLK). Musi obsługiwać przerwania na obu zboczach.
* **prs_pin** (Opcjonalny, Pin): Pin wyjściowy sterujący tranzystorem MOSFET. Wymagany do wysyłania komend.
* **simulated_keypad** (Opcjonalny, boolean): Włącza tryb emulacji klawiatury. Komponent będzie wysyłał potwierdzenia do centrali udając fizyczny manipulator. Wymaga podłączenia pinu `prs_pin`. Domyślnie `false`.
* **trouble_lang** (Opcjonalny, string): Język wypluwanych awarii dla sensora tekstowego. Obsługiwane to `en` oraz `pl`. Domyślnie `en`.
* **variant** (Opcjonalny, string): Wybór wariantu centrali, co wpływa na maskowanie wejść i sensorów. Obsługiwane to `ca6` oraz `ca10`. Domyślnie `ca6`.

---

## Przykładowa konfiguracja

Przykład konfiguracji z uwzględnieniem usług do wywoływania prosto z Home Assistant (np. naciskanie wirtualnych przycisków w Lovelace UI).

```
esphome:
  name: esp32-satel

external_components:
  - source: github://ficueu/esphome-satel-kpd
    components: [ satel_kpd ]

satel_kpd:
  id: my_satel
  data_pin: GPIO4
  ckl_pin: GPIO3
  prs_pin: GPIO5
  simulated_keypad: false
  trouble_lang: pl
  variant: ca6

api:
  reboot_timeout: 0s
  services:
    - service: press_sequence
      variables:
        sequence: string
      then:
        - lambda: 'id(my_satel).press_sequence(sequence);'
    - service: check_trouble
      then:
        - lambda: 'id(my_satel).trigger_trouble_check();'

text_sensor:
  - platform: satel_kpd
    satel_kpd_id: my_satel
    trouble_text:
      name: "Odczytane Awarie"

binary_sensor:
  - platform: satel_kpd
    satel_kpd_id: my_satel   
    armed_a:
      name: "Czuwanie Strefa A"
    armed_b:
      name: "Czuwanie Strefa B"
    alarm_a:
      name: "Alarm Strefa A"
    alarm_b:
      name: "Alarm Strefa B"
    trouble:
      name: "Awaria"
      filters:
        - delayed_off: 0.6s
    power:
      name: "Zasilanie"
    input_1:
      name: "Wejście 1"
    input_2:
      name: "Wejście 2"
    input_3:
      name: "Wejście 3"
    input_4:
      name: "Wejście 4"
    input_5:
      name: "Wejście 5"
    input_6:
      name: "Wejście 6"
    input_7:
      name: "Wejście 7"
    input_8:
      name: "Wejście 8"
```

---

## Opcje bitowe - debugowanie

Komponent posiada wbudowane maski ułatwiające przypisanie funkcji. Jeśli jednak chcesz debugować sygnał, możesz skorzystać z sensorów bitowych (0 do 31).

```
binary_sensor:
  - platform: satel_kpd
    satel_kpd_id: my_satel
    bit_0:
      name: "Debug Bit 0"
    ...
    bit_31:
      name: "Debug Bit 31"
```

<img src="https://github.com/ficueu/esphome-satel-kpd/blob/main/docs/test.jpg" height="300">

---

## Karta manipulatora dla lovelace

https://github.com/ficueu/esphome-satel-kpd/blob/main/examples/lovelace_card.yaml

<img src="https://github.com/ficueu/esphome-satel-kpd/blob/main/docs/lovelace_card.png" height="300">

---

## Pełna konfiguracja

https://github.com/ficueu/esphome-satel-kpd/blob/main/examples/full_config.yaml

---

## Changelog
### v1.1:
* dodano wsparcie dla ESP8266,
* dodano obsługę central CA-10 (zmienna **variant**),
* zwiększono pulę odczytywanych wejść z 8 do 12 (CA-10),
* wprowadzono auto-ładowanie komponentów binary_sensor i text_sensor,
* poprawiono przykład dla lovelace.

---

## Testy / plany na rozbudowę
### CA-6:
* [x] manipulator,
* [x] wejścia 1-8,
* [x] strefy alarmu (A/B),
* [x] strefy czuwania (A/B),
* [x] sprawdzanie awarii,

### CA-10:
* [x] manipulator,
* [x] wejścia 1-12,
* [ ] wejścia 13-16,
* [x] strefy alarmu (A),
* [ ] strefy alarmu (B/C/D),
* [x] strefy czuwania (A),
* [ ] strefy czuwania (B/C/D),
* [x] sprawdzanie awarii,

### Inne centrale:
* [ ] Perfecta - zupełnie inny protokół komunikacji, brak dostępu do centrali wraz z manipulatorem do testów "na stole".
* [ ] Integra - nie podjęto próby - brak manipulatora do testów do testów "na stole".