#include <MD_MAX72xx.h> // Biblioteka potrzebna do matrycy ledowych
#include <NewPing.h> // Bibloteka porzebna do sensorów

// Hardware interface
#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

// Piny do matrycy pierwszej (podłączonej do arduino)
#define CLK_PIN   13
#define DATA_PIN  11  
#define CS_PIN    10  // Chip Select pin
#define MAX_DEVICES 2 // Liczba wyświetlaczy

// Pin analogowy do fotorezystora
#define LIGHT A5 

// Inicjalizacja wyświetlaczy
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Piny TRIGGER oraz ECHO do sensorów
//sensor 1
#define  t1  2 
#define  e1  3  
//sensor 2
#define  t2  4  
#define  e2  5 

// Maksymalny dystans jaki sensor może odczytać (w cm)
#define maxDist 400

// Inicjalizacja sensorów
NewPing eyeR(t2, e2, maxDist ); 
NewPing eyeL(t1, e1, maxDist );

// Zmienna do śledzenia stanu wyświetlaczy, potrzebna żeby powrócić do odpowiedniej funkcji skierowania "oczu" po animacjach
// 0: Animacja patrzenia wprost
// 1: Animacja patrzenia w prawo
// 2: Animacja patrzenia w lewo
int currentState = 3; 

// Zmienne do przechowywania odległości wykrywanych przez sensory
//long duration1, duration2;
int distance1, distance2;

// Losowe wstawianie animacji mrugnięcia
float nextBlink = millis() + 1000;

// Zmienna do przechowywania natężenia światła
float lightAmount = 0;

// Tablice przechowujące animacje oczu dla wyświetlaczy
uint8_t eye_forward[COL_SIZE] =
{
  0b00111100,
  0b01000010,
  0b01011010,
  0b10101101,
  0b10111101,
  0b10011001,
  0b01000010,
  0b00111100
};

uint8_t eye_right[COL_SIZE] =
{
  0b00111100,
  0b01000010,
  0b01110010,
  0b11011001,
  0b11111001,
  0b10110001,
  0b01000010,
  0b00111100
};

uint8_t eye_left[COL_SIZE] =
{
  0b00111100,
  0b01000010,
  0b01001110,
  0b10010111,
  0b10011111,
  0b10001101,
  0b01000010,
  0b00111100
};

uint8_t eye_blink[COL_SIZE] =
{
  0b00000000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b10111101,
  0b11000011,
  0b01111110,
  0b00111100
};

// Zmienna do testowania, zakomentować finalnie
bool debug = true;

void setup()
{
  // Testing
  if (debug) 
  {
    Serial.begin(115200);
    Serial.println("Siema");
  }

  // Inicjalizacja biblioteki potrzebnej do matryc
  mx.begin();

  // Ustawienie trybów pinów dla obu sensorów
  pinMode( t1, OUTPUT );
  pinMode( e1, INPUT );

  pinMode( t2, OUTPUT );
  pinMode( e2, INPUT );

  // Ustawienie pinów TRIGGER domyślnie na tryb LOW
  digitalWrite( t1, LOW );
  digitalWrite( t2, LOW );

  // Ustawienie trybu pinu dla fotorezystora
  pinMode( LIGHT, INPUT );

  // Na początku oczy zawsze patrzą na wprost
  ShowEye_Forward();
  currentState = 0;

}

void loop()
{
  // Odczytujemy aktualny poziom natężenia światła
  lightAmount = analogRead( LIGHT );
  // Upewniamy się, że wartość światła mieści się w zakresie maksymalnego natężenia wyświetlaczy
  lightAmount = ( lightAmount / 255 ) * MAX_INTENSITY;
  // Ustawiamy intensywność na ledach wyświetlaczy
  mx.control(MD_MAX72XX::INTENSITY, lightAmount);

  // Ping the Left eye with a recursion of 5
  distance1 = eyeL.ping_median( 5 );
  // Opóźnienie żeby uniknąć konfliktów danych
  delay(500);
  // Ping the Right eye with recursion of 5 
  distance2 = eyeR.ping_median( 5 );

  // Sprawdzamy czy pora wykonać animację mrugnięcia
  if ( nextBlink < millis() )
  {
    // Ustawiamy wykonanie następnej animacji mrugnięcia na losowy czas w przedziale 5-12 sekund od teraz
    nextBlink = millis() + random(5000, 12000);

    // Wybieramy losową liczbę aby zdecydować czy wykonana zostanie animacja pojedynczego czy podwójnego mrugnięcia
    if ( random(1,13) <= 6 )
    {
      ShowEye_Blink_Dbl();
    }
    else
    {
      ShowEye_Blink();
    }

    //Opóźnienie aby animacja mogła bez przeszkód się wykonać
    delay(250);

    return;
  }
  
  // W zależności od dystansu w jakim sensory wykryją obiekt, wykonane zostaną odmienne animacje (na matrycach wyświetlą się inne wzory)

  // Zmienna przechowująca różnicę w odległościach wykrytych przez sensory
  float difference = ( distance2 - distance1 );

  // Jeżeli różnica jest nieznacząca LUB nic nie zostało wykryte LUB wykryto obiekt na zbyt dużej odległości
  // wykonuje się animacja patrzenia wprost
  // || (distance1 > 4000 && distance2 > 4000) 
  if ( abs( difference ) < 500 || (distance1 == 0 && distance2 == 0 ))
  {
    ShowEye_Forward();
    currentState = 0;
  }
  // Jeżeli prawy sensor wykrył obiekt i znajduje się on bliżej tego sensora niż obiekt wykryty przez sensor drugi
  // wykonuje się animacja patrzenia w prawo
  else if ( distance2 < distance1 && distance1 > 0)
  {
    ShowEye_Right();
    currentState = 1;
  }
  // Jeżeli lewy sensor wykrył obiekt i znajduje się on bliżej tego sensora niż obiekt wykryty przez sensor drugi
  // wykonuje się animacja patrzenia w lewo
  else if ( distance1 < distance2 && distance2 > 0 )
  {
    ShowEye_Left();
    currentState = 2;
  }

  // Opóźnienie aby animacje mogły wykonać się poprawnie
  delay(250);

  // Testing
  if (debug) 
  {
    Serial.print("lightAmount: ");
    Serial.print(lightAmount);
    Serial.println("");
    Serial.print("distance1/distance2: ");
     Serial.print(distance1);
     Serial.print("/");
     Serial.print(distance2);
     Serial.println("");
     Serial.print("distance2 - distance1: ");
     Serial.print(distance2-distance1);
     Serial.println("");
     Serial.println("");
  }

}

// Funkcje odpowiedzialne za odpowiednie wyświetlenie animacji przez wyświetlacze

// Patrzenie w prawo
void ShowEye_Right()
{
  // Czyścimy cokolwiek wyświetlało się na matrycach
  mx.clear();
  // Ustawiamy intensywność matryc
  mx.control(MD_MAX72XX::INTENSITY, lightAmount );

  // Pętla przez wszystkie rzędy matryc
  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
    // Ustawiamy odpowiednie wartości dla matrycy pierwszej
    mx.setColumn(row, eye_right[row]);
    // Ustawiamy odpowiednie wartości dla matrycy drugiej
    mx.setColumn(row+8, eye_right[row]);
  }
}

// Patrzenie w lewo
void ShowEye_Left()
{

  mx.clear();

  mx.control(MD_MAX72XX::INTENSITY, lightAmount );


  for (uint8_t row=0; row<ROW_SIZE; row++)
  {

    mx.setColumn(row, eye_left[row]);

    mx.setColumn(row+8, eye_left[row]);
  }
}


void ShowEye_Forward()
{

  mx.clear();

  mx.control(MD_MAX72XX::INTENSITY, lightAmount );


  for (uint8_t row=0; row<ROW_SIZE; row++)
  {

    mx.setColumn(row, eye_forward[row]);

    mx.setColumn(row+8, eye_forward[row]);
  }
}

// Mrugnięcie
void ShowEye_Blink()
{

  mx.clear();

  mx.control(MD_MAX72XX::INTENSITY, lightAmount );

 
  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
    
    mx.setColumn(row, eye_blink[row]);
    
    mx.setColumn(row+8, eye_blink[row]);
  }

  // Opóźnienie dla poprawnego wyświetlenia
  delay(150);

  // W zależności od tego, jaka animacja była wyświetlana poprzednio, wracamy do niej po mrugnięciu
  if ( currentState == 0 )
    ShowEye_Forward();
  else if ( currentState == 1 )
    ShowEye_Right();
  else if ( currentState == 2 )
    ShowEye_Left();

}

// Podwójne mrugnięcie
void ShowEye_Blink_Dbl()
{

  mx.clear();

  mx.control(MD_MAX72XX::INTENSITY, lightAmount );

 
  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
     
    mx.setColumn(row, eye_blink[row]);
    
    mx.setColumn(row+8, eye_blink[row]);
  }

  
  delay(75);

  // Wracamy na chwilę do poprzedniej animacji, żeby ruch wyglądał bardziej realistycznie...
  if ( currentState == 0 )
    ShowEye_Forward();
  else if ( currentState == 1 )
    ShowEye_Right();
  else if ( currentState == 2 )
    ShowEye_Left();

  
  delay(75);

  //... po czym wykonujemy animacje mrugnięcia ponownie
  ShowEye_Blink();
}

