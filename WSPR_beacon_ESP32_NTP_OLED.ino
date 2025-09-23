#include <si5351.h>
#include <JTEncode.h>
#include "Wire.h"
#include "settings.h"
#include "esp32-hal-timer.h"

// OLED display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"
#include "Fixed8x16.h"

// NTP y WiFi
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// WSPR defines
#define WSPR_TONE_SPACING 146
#define WSPR_DELAY        683
#define SYMBOL_PERIOD_US  683000UL

// OLED I2C pins
#define OLED_RST NOT_A_PIN
#define OLED_SDA 21
#define OLED_SCL 22
Adafruit_SSD1306 display(OLED_RST);

// WiFi credentials (puedes mover a settings.h si prefieres)
//const char* ssid = "CASETA";
//const char* password = "Damacla1115";
// UTIEL
//char MaidenHead[7] = "IM99kp";
const char* ssid = "TP-Link_7254";
const char* password = "63341379";
// VALENCIA
char MaidenHead[7] = "IM99tl";
// NTP setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // UTC, actualiza cada 60s

// Si5351 y WSPR
Si5351 si5351;
JTEncode jtencode;

uint8_t tx_buffer[255];
uint8_t symbol_count = WSPR_SYMBOL_COUNT;
uint16_t tone_spacing = WSPR_TONE_SPACING;
unsigned long long freq = 0ULL;
volatile bool proceed = false;

// Timer usando esp_timer (software timer)
#include "esp_timer.h"
esp_timer_handle_t periodic_timer;

// Estructura para datos WSPR
struct TGPS {
  bool sendMsg1 = false;
  char MH_1[5] = {'I','M','9','9', 0};  // Locator fijo para NTP
  uint8_t dbm_1 = DBMPOWER;

  int Hours = 0;
  int Minutes = 0;
  int Seconds = 0;

  bool GPS_valid = false;
};

TGPS UGPS;

// Callback del timer
void timer_callback(void* arg) {
  proceed = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[WSPR Beacon] Inicializando...");

  // Conectar a Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado.");

  // Configurar el reloj de sistema con NTP (UTC)
 configTime(0, 0, "pool.ntp.org");

  // Iniciar cliente NTP
  timeClient.begin();
  timeClient.setTimeOffset(0);  // UTC
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
  Serial.println("⏱ Sincronizado con NTP: " + timeClient.getFormattedTime());

  // Iniciar Si5351
  if (!si5351.init(SI5351_CRYSTAL_LOAD_10PF, SI5351FREQ, 0)) {
    Serial.println("[ERROR] No se detectó el Si5351.");
    while (1);
  }
  si5351.set_correction(SI5351_CORRECTION, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // cambiar CLKx por el usado CLK0, CLK1, CLK2
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK1, 0);
  Serial.println("[Si5351] Inicializado correctamente.");

  // Configurar timer software con esp_timer
  esp_timer_create_args_t periodic_timer_args = {
      .callback = &timer_callback,
      .arg = NULL,
      .name = "periodic"
  };

  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, SYMBOL_PERIOD_US);

  Serial.println("[Timer] Configurado e iniciado.");

  // OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, OLED_SDA, OLED_SCL);
  display.clearDisplay();
  display.setFont(&Fixed8x16);
  display.setTextColor(WHITE);
  display.setCursor(0, 12);
  display.println("WSPR TX - EA5JTT");
  display.setCursor(0, 27);
  display.println("Usando NTP...");
  display.display();
  delay(2000);
  Serial.println("✅ OLED inicializada, mostrando mensaje...");
}

void loop() {
  timeClient.update();
  CheckNTP();

  if (UGPS.sendMsg1 && freq != 0ULL) {
    Serial.print("[TX] Frecuencia seleccionada: ");
    Serial.println(freq);

    display.clearDisplay();
    display.setCursor(0, 12);
    display.println("WSPR TX - EA5JTT");
    display.setCursor(0, 27);
    display.print("Frec: ");
    display.println(freq);
    display.display();

    encode();
    UGPS.sendMsg1 = false;
  }

  delay(900);
}

void CheckNTP() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    UGPS.Hours = timeinfo.tm_hour;
    UGPS.Minutes = timeinfo.tm_min;
    UGPS.Seconds = timeinfo.tm_sec;
    UGPS.GPS_valid = true;

    strcpy(UGPS.MH_1, "IM99");
    UGPS.dbm_1 = DBMPOWER;

    if ((UGPS.Seconds <= 2) && (UGPS.Minutes % 2 == 0)) {
      freq = freqArray[(UGPS.Minutes / 2) % 10];
      UGPS.sendMsg1 = true;
    }
  } else {
    Serial.println("[NTP] Error al obtener la hora.");
  }
}

void encode() {
  set_tx_buffer();
  Serial.println("[TX] Iniciando transmisión WSPR...");
  display.setCursor(0, 39);
  display.println("Inicio TX");
  display.display();
// cambiar CLKx por el usado CLK0, CLK1, CLK2
  si5351.output_enable(SI5351_CLK1, 1);

  for (uint8_t i = 0; i < symbol_count; i++) {
    unsigned long long tx_freq = (freq * 100ULL) + (tx_buffer[i] * tone_spacing);
    // cambiar CLKx por el usado CLK0, CLK1, CLK2
    si5351.set_freq(tx_freq, SI5351_CLK1);
    Serial.print("[TX] Símbolo ");
    Serial.print(i);
    Serial.print(" → Frecuencia: ");
    Serial.println(tx_freq);
    display.setCursor(i * 6, 51);  // Ajustado para mejor espaciado
    display.print(".");
    display.display();
    proceed = false;
    while (!proceed);
  }
// cambiar CLKx por el usado CLK0, CLK1, CLK2
  si5351.output_enable(SI5351_CLK1, 0);
  Serial.println("[TX] Transmisión finalizada.");
  display.setCursor(0, 63);
  display.println("Fin TX");
  display.display();
}

void set_tx_buffer() {
  // MYCALL esta definido en setting.h
  // UGPS.MH_1 es el localtor definido en ChekNTP
  // UGPS.dbm_1 copia DBMPOWER que esta definido en setting.h
  memset(tx_buffer, 0, 255);
  if (UGPS.sendMsg1) {
    Serial.print("[ENCODE] Codificando mensaje: ");
     Serial.print(MYCALL); Serial.print(" ");
    Serial.print(UGPS.MH_1);
    Serial.print(" ");
    Serial.print(UGPS.dbm_1);
    Serial.println(" dBm");
    jtencode.wspr_encode(MYCALL, UGPS.MH_1, UGPS.dbm_1, tx_buffer);
  }
}
