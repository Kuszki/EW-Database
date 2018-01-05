# EW-Database

Edycja baz danych programu EW-Mapa

## Wyświetlanie obiektów

- Wyświetlanie atrybutów obiektów
- Wielokrotne grupowanie obiektów na podstawie atrybutów
- Możliwość wczytania listy obiektów z pliku tekstowego

## Eksport obiektów

- Możliwość eksportu wybranych atrybutów obiektów do pliku tekstowego

## Filtrowanie obiektów

- Filtrowanie na podstawie kodu obiektu

- Filtrowanie na podstawie atrybutów
	- Operatory `=`, `<>`, `>=`, `>`, `<=`, `<`, `BETWEEN`, `LIKE`, `NOT LIKE`, `IN`, `NOT IN`, `IS NULL`, `IS NOT NULL`
	- Możliwość wyboru operatorów logicznych `AND` lub `OR`

- Filtrowanie na podstawie geometrii
	- Na podstawie długości elementów
	- Na podstawie powierzchni elementów
	- Na podstawie relacji geometrycznych
		- Obiekt dotyka inny obiekt
		- Obiekt zawiera inny obiekt
		- Obiekt jest częścią innego obiektu
		- Obiekt kończy się na innym obiekcie
	- Na podstawie relacji logicznych
		- Objekt posiada podobiekt
		- Obiekt jest podobiektem
	- Na podstawie rodzaju geometrii
	- Możliwość ograniczenia listy obiektów wchodzących w skład relacji
	- Możliwość ustalenia kodów obiektów dla każdego z warunków
	- Możliwość negacji każdego z warunków
	- Możliwość zadania promienia poszukiwań

- Filtrowanie na podstawie elementów redakcji
	- Na podstawie kątu obrotu symbolu
	- Na podstawie kątu obrotu etykiety
	- Na podstawie atrybutów etykiety
	- Na podstawie rodzaju i istnienia symbolu
	- Na podstawie rodzaju i istnienia etykiety
	- Na podstawie stylu i istnienia linii
	
- Możliwość zastosowania filtrów do wybranej listy obiektów
	- Iloczyn z wynikiem działania filtra
	- Różnica z wynikiem działania filtra
	- Suma z wynikiem działania filtra
	
## Edycja atrybutów obiektów

- Edycja dowolnych atrybutów zaznaczonych obiektów
- Edycja wsadowa na podstawie pliku tekstowego
- Możliwość ustalenia wartości specjalnych atrybutów

## Zmiana kodów obiektów

- Możliwość zmiany kodu zaznaczonych obiektów
- Możliwość zmiany symboli zaznaczonych obiektów
- Możliwość zmiany treści etykiet zaznaczonych obiektów
- Możliwość zmiany stylu linii zaznaczonych obiektów
- Możliwość zmiany warstw elementów zaznaczonych obiektów

## Edycja geometrii obiektów

- Tworzenie relacji pomiędzy obiektami
- Scalanie obiektów liniowych na podstawie wybranych atrybutów
- Podział obiektów liniowych
	- W punkcie wstawienia obiektu punktowego
	- W punkcie wspólnym kilku obiektów liniowych
- Korekcja geometrii obiektów
	- Dociąganie punktów załamania i symboli do zadanych punktów
	- Przedłużanie i docinanie obiektów liniowych do zadanych linii
	- Możliwość zadania promienia poszukiwań i maksymalnych różnic w długości linii
- Modyfikacja geometrii obiektów
	- Wstawianie punktów załamania w miejscu obiektów punktowych
	- Wstawianie punktów załamania w miejscu przecięcia segmentów
	- Wstawianie punktów załamania w miejscu istniejących punktów załamania
	- Wstawianie punktów załamania w miejscu styku z końcem obiektów liniowych
	
## Edycja redakcji obiektów

- Modyfikacja treści etykiet obiektów
- Wstawianie opisów obiektów
	- Powtarzanie etykiet co zadany dystans
	- Określenie minimalnej długości obiektów liniowych
	- Automatyczny obrót i justyfikacja etykiet
- Automatyczna redakcja opisów
	- Nasuwanie i automatyczny obrót etykiet zgodnie z kierunkiem osi
	- Automatyczny wybór poprawnej justyfikacji
- Usuwanie etykiet wybranych obiektów

## Integracja z oprogramowaniem EW-Mapa

- Możliwość zaznaczania obiektów na liście poprzez ich aktywacje w programie EW-Mapa
- Możliwość tworzenia listy obiektów poprzez ich aktywacje w programie EW-Mapa
- Możliwość zaznaczenia wybranych obiektów w programie EW-Mapa

## Dodatkowe możliwości

- Przywracanie pierowtnego operatu zaznaczonych obiektów
- Usuwanie wersji historycznych zaznaczonych obiektów
