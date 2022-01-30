# Watering Station


Jest to urządzenie do automatycznego podlewania roślin, na podstawie danych odczytywanych z sensorów wilgotności gleby, oraz informacji zapisanych podczas konfiguracji urządzenia.

#### Główne założenia:

- Urządzenie obsługuje do 4 roślin (tyle przyjąłem jako średnią ilość roślin na parapecie).
- Ziemia roślin nie może być za bardzo przepuszczalana, ze względu na pracę na podstwie odczytów z sensorów wilgotności.

_Urządzenie średnio się spisze przy bardzo dużych roślinach (ze względu wydajność pomp i wielkość zbiornika, ale nic nie stoi na przeszkodzie aby je odpowiednio dostosować)._

#### Skrócony opis działania:

Po prawidłowym skonfigurowaniu urządzenia, pobierane są watrtości odczytów z sensorów wilgotności gleby. Konfiguracja dla danej rośliny przewiduje ustawienie jej nazwy, minimalnej i maksymalnej wilgotności oraz parametru czy jest ona aktywna, tzn czy dany sensor ma być używany. Gdy odczyt z sensora pokaze wartość poniżej minimalnej (wynikającej z konfiguraji) uruchamiany jest proces podlewania dla danej rośliny. Obliczany jest środek zakresu między minimalną a maksymalną wilgotnością i uruchamiana jest odpowiednia pompa. Podelwanie odbywa się w krótich cyklach podawania wody. Po podlaniu następuje okres stabilizacji sensorów. Następnie gdy ponowne odczyty pokażą wartość mniejszą niż średnia, proces podelwania będzie dalej kontynuowany. Po przekroczeniu średniej wilgotności następuje zakończenie procesu podlewania i zostanie uruchomiony dopier gdy wilgotność gleby znowu spadnie poniżej minimum. Gdy po podlaniu i czasie stabilizacji sensora, wilgotność będzie równa, lub mniejsznia niż przed podlaniem, zwiększany jest wewnętrzny licznik. Gdy po 5 podlaniach i okresach stabilizacji, wilgotność nie zwiększa się, proces podlewania zostaje przerwany z komunikatem błędu pompy lub sensora. Należy wtedy sprawdzić urządzenie, poprawić jeżeli wymaga i ponownie uruchomić.

Pompy zanurzone są w zbiorniku z wodą. Poziom wody jest badany za pomocą sesnora HC-SR04. Brak wody sygnalizowany jest przez diodę czerwoną. Brak wody przerywa procesy podelwania.

#### W skład urządnie wchodzą:
| Nazwa | Ilość | Img |
| ----- | ----- | ----- |
|ESP8266 NodeMCU v3 ESP8266 ESP-12 | 1 |![img](https://i.imgur.com/fw5sBvDs.jpg)|
|Capacitibe Mositure Sensor v2.0 | 4 |![img](https://i.imgur.com/4h78UHHs.jpg)|
|DHT22 czujnik temperatruy i wilgotności powietrza | 1 |![img](https://i.imgur.com/Pcq8Idbs.jpg)|
|GY-30 czujniuk natężenia światła | 1 |![img](https://i.imgur.com/Xp8wtgBs.jpg)|
|4 kanałowy moduł przekaźników | 1 |![img](https://i.imgur.com/0rViYLxs.jpg)|
|HC-SR04 ultradźwiękowy moduł do pomiaru odległości| 1 |![img](https://i.imgur.com/B3sBicss.jpg)|
|ADS1115 ekspander wejść analogowych | 1 |![img](https://i.imgur.com/TryQZXrs.jpg)|
|PCF8574 ekspander GPIO | 1 |![img](https://i.imgur.com/SflWvGYs.jpg)|
|Ekran OLED 0.96" | 1 |![img](https://i.imgur.com/VnkeFgQs.jpg)|
|Konwerter poziomów logicznych 3.3V-5V | 1 |![img](https://i.imgur.com/kPtF2XYs.jpg?1)|
|Przycisk | 2 |![img](https://i.imgur.com/R2r8Cjjs.jpg)|
|Dioda czerwona | 1 |![img](https://i.imgur.com/MqZdERVs.jpg?1)|
|LM2596 DC-DC przetwonica step-down | 1 |![img](https://i.imgur.com/3U2xtRMs.jpg)|
|Pompa zanurzeniowa 3-6V | 4 |![img](https://i.imgur.com/cMeIU7Is.jpg)|
|Zasilacz 6V 2A | 1 |![img](https://i.imgur.com/dzSWiQjs.jpg)|
|Pojemnik na wodę 2L | 1 |![img](https://i.imgur.com/Wr88GiGs.jpg)|


Esp8266 umożliwia konfigurację poprzez dowolną przeglądarkę. Po uruchomieniu urządzenie sprawdza czy posiada zapisane poświadczenia do sieci WiFi jak nie to przechodzi w tryb konfiguracji. Tworzy sieć:
|SSID|Hasło|
|---|---|
|WateringStation| qwerty123|

 Po podłączeniu do wyżej wymienionej sieci można dokonać konfiguracji sieci WiFi, opcjonalne połączenia z serwerem MQTT oraz konfigurację roślin (nazwa, min i max wilgotność, czy aktywna). Konfiguracji można dokonać z dowolnej przeglądarki, pod adresem:
 ```sh
192.168.4.1
```

Po dokonaniu konfiguracji urządznie połączy się z siecią WiFi oraz serwerm MQTT. Gdy połączenie do WiFi się nie uda urządzneie ponownie przejdzie w tryb konfiguracji. Po 5min przejdzie w tryb monitorowania i podlewania roślin, jeżeli w pamięcie znajdą się zapisane konfiguracje roślin.

Urządzenie poza odczytem sensorów wilgotności gleby, bada temperaturę i wilgotność powietrza oraz poziom nasłonecznienia. Dodatkowo monitorowany jest stan wody w zbiorniku, stan połączenia WiFi oraz MQTT. Dane na temat bierzących wartości sensorów, stanu WiFi i MQTT dostępne są również na małym wyświetlaczu OLED. A po krótkim naciśnięciu Przycisku nr1 zostanie pokazana zapisana konfiguracja roślin.

Urządzenie posiada 2 przyciski:

|Przycisk nr 1| Czas |
|---|---|
|Krótki czas naciśnięcia -  pokazuje zapisaną konfigurację roślin|t<500ms|
|Średni czas naciśnięcia -  powoduje ponowne uruchomienia urządzenia| 500ms>t<5s
|Długie czas naciśnięcia -  czyści dane dotyczące WiFi, ponownie uruchamia urządzenuie w trybie konfiguracji| t>5s |

|Przycisk nr 2| Czas |
|---|---|
|Krótki czas naciśnięcia -  włącza/wyłącza ekran OLED|t<500ms|
|Średni czas naciśnięcia -  nie wykorzystane| 500ms>t<5s
|Długie czas naciśnięcia -  formatuje wewnętrzną pamięć SPIFFS (usuwana jest konfiguracja roślin)| t>5s |

Urządzenie po podłączeniu do WiFi pod przydzielonym adresem IP wyświetal stronę z aktualnymi danymi sensorów.
_Należy ją manulanie odświeżyć w razie potrzeby uzyskania najświeższych wartości._

##### Urządzenie po każdym odczycie wysyła wiadomości do serwera MQTT:
- wiadomość z danymi powietrza (temp, wilgotność, nasłonecznie, poziom wody w zbiorniku)
- wiadomość z danymi 1 rośliny (nazwa, wilgotność gleby, ostatni czas podlewania)
- wiadomość z danymi 2 rośliny (nazwa, wilgotność gleby, ostatni czas podlewania)
- wiadomość z danymi 3 rośliny (nazwa, wilgotność gleby, ostatni czas podlewania)
- wiadomość z danymi 4 rośliny (nazwa, wilgotność gleby, ostatni czas podlewania)

_Wiadomości dotyczące konkretnych roślin wysyłane są tylko przy prawidłowym skonfigurowaniu danych do serwera MQTT._

Dane z sensorów odczytywane są co pewien czas. Na razie 5min. Czas stabilizacji sesnsorów to 2h (minimalny rozsądny czas). Czas podlewania (działania pompy) 2s.

Sensor DHT22 oraz GY-30 są jako dodatek, ich wskazania nie są uwzględniane w żadnym algorytmie. Sensor DHT22 działa niezadowalająco ze względu na częste i znaczne rozbierzności we wskazaniach wilgotności.

_Należy zapewnić dopływ powietrza do zbiornika z wodą w celu wyrównania ciśnień po podlewaniu. Zapobiega to wypompowania wody po wyłączeniu pompy gdy wylot znajduje się poniżej poziomu wody w zbiorniku._

##### _Ze względu na spore zakłócenia generowane przez silniki pomp (jak i zapewne przez pozostałe urządzenia z otoczenia), przy każdym użytym module zostały dołożone kondensatory ceramiczne 100nF. Dodatkowo sensory wilgotności zostały przerobione. Został zastosowany wtórnik napięcia aby oddzielić wejście sensora od wyjścia. Dodatkowo udało się wzmocnić sygnał wyjściowy senora aby zakłócenia indukowane przez przewody były pomijalne. Testy pokazały, że stabilność wskazań sensorów znacznie się poprawiła. Kolejną przeróbką, była zmiana rezystora odpowedzialengo za prędkość ładowania i rozładowania kondensatora. Dzięki temu sensory szybciej reagują na zmiany wilgotności._
#
#

#### Kalibracja:
Początkowo kalibracja sensorów wilgotności gleby była przerowadzona w powietrzu i szklance z wodą. Po testach została zmieniona i do ustawienia wartości użyto ziemi, która od kilku miesięcy nie była zraszana wodą. Można użyć ziemi wysuszonej w pierkarniku. W moim przypadku przy wcześniejszeja kalibracji (w powietrzu i szklance wody), wolgotność użytej ziemi wynosiła 7%. Kalibrację należy wykonać bo przeróbkach.

#### Wnioski i spotrzerzenia (na podstawie testów działania):
 - czujnik DHT22 się nie nadaje (randomowe wskazania wilgotności powietrza).
 - sensory wilgotności należy umieścić w pewnym odstępie od rośliny, w moim przypadku umieściłem przy krawędzi doniczki
 - punkt podlewania najlepiej umieścić w równej odległości od sensora i środka rośliny
 - nie należy dotykać sensora po umieszczenieu w ziemi, nie zmieniać co chwila jego położenia
 - czujnik HC-SR04 czasem łapał dziwne wartości, zostało zmienione oprogramowanie tak, że wykonywane jest 8 pomiarów i odrzucana jest maksymalna i minimalan wartość i wyliczana średnia. Dodatkowo jeżeli następne wskazanie będzie mniejsze od poprzedniego o 4cm, pomiar jest odrzucany (nie jest to idealne rozwiązanie ale do przyjęcia)
 - badanie wilgotności gleby również jest średnią z 8 pomiarów po wykluczeniu minimalnej i maksynalnej wartości.
 - minimalny sensowny czas stabilizacji senorów to 2h ale zalecam konfiguracje na poziomie 3-5h (znaczenie ma wielność doniczki, odelgłość punktu podlewania od sensora, rodzaj gleby i prędkość rozchodzenia się wody w danej ziemi)
 - minimalną i maksymalną wilgotność dla danej rośliny należy dobrać doświadczalnie (nie należy się sugerować informacjami z internetu czy aplikacji np. Mi Flora, oraz wartościami z sensorów rezystancyjnych). W moim przypadku sprawdza się ustawienie 25%-60% dla rośliny Zamiokulkas oraz 20%-60% dla Sansevieria Superba.

### Użyte biblioteki:

[ADS1115](https://github.com/adafruit/Adafruit_ADS1X15)
[BH1750](https://github.com/claws/BH1750) - dla GY-30
[PCF8574](https://github.com/RobTillaart/PCF8574)
[DHT22](https://github.com/adafruit/DHT-sensor-library)
[OLED SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
[PubSubClient for MQTT](https://github.com/knolleary/pubsubclient)
[WiFiManager](https://github.com/tzapu/WiFiManager)
Arduino.h
Adafruit_Sensor.h
Adafruit_GFX.h
FS.h
ESP8266WiFi.h
DNSServer.h
ESP8266WebServer.h
ArduinoJson.h
ESP8266WiFi.h
DNSServer.h
NTPClient.h
WiFiUdp.h

### Połączenie:
[Schemat](https://i.imgur.com/Qk4lYX7.png)
![img](https://i.imgur.com/Qk4lYX7h.png)

### Projekt w ciągłej rozbudowie

# Buy Me A beer! 🍺

Buymeacoffee is a website that contacts developers, designers, artists, etc. with their communities so that people can contribute and help them out so that the content they offer is better since the rewarding system encourages creators to continue doing what they like and helping others.

If you can contribute or you want to, feel free to do it at [__Buy me a beer! :beer:__](https://www.buymeacoffee.com/pawelm), I will be really thankfull for anything, because that helps me a lot.
### Be careful, there is no refund system. Remember that you don't need to donate, it is just a free choice for you. Thank you!
