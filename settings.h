/***********************************************************************************
* Use serial console Output for debugging and development
*  
* Change if needed
************************************************************************************/
// Comment out if you do not want Serial console output
// Comment out DEVMODE in a production environment as it will degrade performance!
#define DEVMODE

// Your own HAM call. Change it
#define MYCALL "XXXXXX"

// The power of your transmission in dBm. 
// for the si5351 this should be set to 10 (=10 milliwatts).
// Es la potencia que entrega Si5351
#define DBMPOWER 7


// If you have a valid GPS lock, the software will make this HIGH twice for 20ms
// You could connect a LED to it.
// Set LED_PIN to 0 if you do not want to use a pin
// Pongo el 25 que es el que normalmente usamos 
#define LED_PIN 25       

// Frequency of the SI5351 Oscillator in Hertz
// for example #define SI5351FREQ 26000000 if you have a 26MHz XO
// Use 0 if you have a 25MHz Oscillator
// segun los datos de Aliexpress funciona a 25 MHz asíe s que pongo 0
#define SI5351FREQ 0


/***********************************************************************************
* Todos los chips  Si5351 preceisan calibrar la frecuencia y para ello hay que calcular el valor de ajuste:
* - mediante el progrma de calibraciónd e ejemplo
* - mediante un frecuencimetro
* - mediante un receptor 
*
* Se calibre con centesimas de Hz
* por algun motivo el ajuste de 2660 Hz para 18MHZ se convierte en 1160 ¿?
************************************************************************************/
// #define SI5351_CORRECTION 116000
#define SI5351_CORRECTION 0


/***********************************************************************************
* FRECUENCIAS WSPR 
* Frecuencia en  Hz con - correcccion si #define SI5351_CORRECTION 0
*            0ULL,         No transmite
* 60m  5286600ULL, no se usa pues no esta consensuada a nivel mundial por asignacioens dispares de esta banda
* 6m  50294500ULL, precisa filtro especifico
* LAs bandas son de 200 Hz por lo que se pone la frecuecnia central y el margen es de +/- 100 Hz
************************************************************************************/
// INTERBATIONAL WSPR BEACON PROJECT https://github.com/HB9VQQ/WSPRBeacon
static const unsigned long long freqArray[10] = {
     1838100ULL,   // 160m 0,20,40
     3570100ULL,   // 80m 2,22,42
     5288700ULL,   // 60m 4,24,44
     7040100ULL,   // 40m 6,26,46
    10140200ULL,   // 30m 8,28,48
    14097100ULL,   // 20m 10,30,50
    18106100ULL,   // 18m 12,32,52
    21096100ULL,   // 15m 14,34,54
    24926100ULL,   // 12m 16,36,56 
    28126100ULL    // 10m 18,38,58 
    };

/***********************************************************************************
* GPS SETTINGS
*  
* Change if needed
************************************************************************************/
//Para Lilygo  T-Beam V1.2 la velocidad es 9600 y los pines 12-TX 34-RX  
static const int Rx = 34, Tx = 12; 
static const uint32_t GPSBaud = 9600;  
                         
