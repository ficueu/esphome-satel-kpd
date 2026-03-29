# Satel-KPD Custom Component for ESPHome

Komponent dla ESPHome pozwalający na dwukierunkową komunikację z centralą alarmową **CA-6** (oraz potencjalnie innymi) poprzez magistralę manipulatora **KPD, COM, DATA, CLK**.

### Funkcje:
* odczyt stanów wejść Zx,
* odczyt diod statusowych (zasilanie, telefon, awaria, buzzer, czuwanie a/b, alarm a/b),
* emulowanie manipulatora,
* automatyczny odczyt awarii,
* załączanie/wyłączanie czuwania.

---

## Hardware

Magistrala manipulatora działa na logice **11V-16V** więc niezbędne jest zastosowanie konwertera poziomów logicznych.

### Wariant 1: Podstawowy (tylko odczyt):
Niezbędny będzie minimum 2 kanałowy konwerter poziomów logicznych akceptujący napięcia do 16V (np. **Pololu 2595**). Konwerter obniży napięcia ~12V z szyn zegara (CLK) i danych (DATA) do poziomu 3.3V akceptowanego przez ESP. 

### Wariant 2: Pełny (odczyt + emulacja)
Aby wysyłać sygnały do centrali, musimy mieć element wykonawczy, który będzie zwierał sygnał DATA do masy - wymagany będzie np. układu z **tranzystorem MOSFET** oraz **rezystorem 100Ω**.

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
```

* **data_pin** (Wymagany, Pin): Pin wejściowy odczytujący dane z magistrali (DATA). 
* **ckl_pin** (Wymagany, Pin): Pin wejściowy odczytujący sygnał zegara z centrali (CLK). Musi obsługiwać przerwania na obu zboczach.
* **prs_pin** (Opcjonalny, Pin): Pin wyjściowy sterujący tranzystorem MOSFET. Wymagany do wysyłania komend.
* **simulated_keypad** (Opcjonalny, boolean): Włącza tryb emulacji klawiatury. Komponent będzie wysyłał potwierdzenia do centrali udając fizyczny manipulator. Wymaga podłączenia pinu `prs_pin`. Domyślnie `false`.
* **trouble_lang** (Opcjonalny, string): Język wypluwanych awarii dla sensora tekstowego. Obsługiwane to `en` oraz `pl`. Domyślnie `en`.

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

https://github.com/ficueu/esphome-satel-kpd/examples/lovelace_card.yaml

<img src="https://github.com/ficueu/esphome-satel-kpd/blob/main/docs/lovelace_card.png" height="300">

---

## Pełna konfiguracja

https://github.com/ficueu/esphome-satel-kpd/examples/full_config.yaml

---

## Plany na rozbudowę
Obecna implementacja została zoptymalizowana w oparciu o protokół dla centrali CA-6. W kolejnych krokach będę chciał sprawdzić kompatybilność z:
* [ ] **CA-10**,
* [ ] **Perfecta**,
* [ ] **Integra**.