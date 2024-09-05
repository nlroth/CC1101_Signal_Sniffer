/*
L. Rothman Aug 27, 2024
This program reads the CC1101 for activity in the 315MHz, 433MHz & 860-900MHz ISM ranges
and displays on an OLED along with the RSSI level.

The original application at: https://github.com/smolbun/CC1101-Frequency-Analyzer
only sends output to a serial terminal.

*/


#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Set up new I2C GPIOs for 1" OLED display on ESP32-CAM module
#define I2C_SDA 5      // board specific for HW-724
#define I2C_SCL 4      // board specific for HW-724 

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RADIOLIB_CHECK_PARAMS (0)

#define SS_PIN 25
#define gdo2 14
#define gdo0 26

#define HSPI_SCK   13    // GPIO SPI Clock  
#define HSPI_MISO  15    // GPIO Data Input
#define HSPI_MOSI  12    // GPIO Data Output
#define HSPI_CS    25    // Chip Select - Active LOW

char buff[127];


const int rssi_threshold = -74;

CC1101 radio = new Module(SS_PIN, gdo0, RADIOLIB_NC, gdo2);

static const uint32_t subghz_frequency_list[] = {
  /* 300 - 348 */
  300000000,
  302757000,
  303875000,
  303900000,
  304250000,
  307000000,
  307500000,
  307800000,
  309000000,
  310000000,
  312000000,
  312100000,
  312200000,
  313000000,
  313850000,
  314000000,
  314350000,
  314980000,
  315000000,
  318000000,
  330000000,
  345000000,
  348000000,
  350000000,

  /* 387 - 464 */
  387000000,
  390000000,
  418000000,
  430000000,
  430500000,
  431000000,
  431500000,
  433075000, /* LPD433 first */
  433220000,
  433420000,
  433657070,
  433889000,
  433920000, /* LPD433 mid */
  434075000,
  434176948,
  434190000,
  434390000,
  434420000,
  434620000,
  434775000, /* LPD433 last channels */
  438900000,
  440175000,
  464000000,
  467750000,

  /* 779 - 928 */
  779000000,
  868350000,
  868400000,
  868800000,
  868950000,
  906400000,
  915000000,
  925000000,
  928000000,
};

typedef struct
{
  uint32_t frequency_coarse;
  int rssi_coarse;
  uint32_t frequency_fine;
  int rssi_fine;
} FrequencyRSSI;

int linecount=3;

void setup() {
  Serial.begin(115200);

  // Initialise alternate SPI lines for the CC1101
  SPI.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, HSPI_CS);

  Wire.begin(I2C_SDA, I2C_SCL);  // set up SDA and SCL lines for built-in OLED

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE); 

  // Show startup on OLED
  sprintf(buff, "Starting...\n");
  display.println(buff);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();

  Serial.printf("Frequency Analyzer Starting...");
  radio.begin();
  Serial.printf("Frequency Analyzer Started!");

  // init line count on oled

}

void loop() {

  int rssi;
  FrequencyRSSI frequency_rssi = {
    .frequency_coarse = 0, .rssi_coarse = -100, .frequency_fine = 0, .rssi_fine = -100
  };

  // First stage: coarse scan
  radio.setRxBandwidth(650); // 58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650 and 812 kHz    (81kHz seems to work best for me)
  size_t array_size = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
  for (size_t i = 0; i < array_size; i++) {
    uint32_t frequency = subghz_frequency_list[i];
    if (frequency != 467750000 && frequency != 464000000 && frequency != 390000000 && frequency != 312000000 && frequency != 312100000 && frequency != 312200000 && frequency != 440175000) {
      radio.setFrequency((float)frequency / 1000000.0);
      radio.receiveDirect();
      delay(2);
      rssi = radio.getRSSI();

      if (frequency_rssi.rssi_coarse < rssi) {
        frequency_rssi.rssi_coarse = rssi;
        frequency_rssi.frequency_coarse = frequency;
      }
    }
  }

  // Second stage: fine scan
  if (frequency_rssi.rssi_coarse > rssi_threshold) {
    // for example -0.3 ... 433.92 ... +0.3 step 20KHz
    radio.setRxBandwidth(58);
    for (uint32_t i = frequency_rssi.frequency_coarse - 300000; i < frequency_rssi.frequency_coarse + 300000; i += 20000) {
      uint32_t frequency = i;
      radio.setFrequency((float)frequency / 1000000.0);
      radio.receiveDirect();
      delay(2);
      rssi = radio.getRSSI();

      if (frequency_rssi.rssi_fine < rssi) {
        frequency_rssi.rssi_fine = rssi;
        frequency_rssi.frequency_fine = frequency;
      }
    }
  }



  // Deliver results fine
  if (frequency_rssi.rssi_fine > rssi_threshold) {
   linecount++;
    if (linecount > 3) {
      linecount=1;
      display.clearDisplay(); 
      display.setCursor(0,0);
    }
    // Buffer the display line then output
    sprintf(buff, "FINE Freq: %.2f     RSSI: %d\n", (float)frequency_rssi.frequency_fine / 1000000.0, frequency_rssi.rssi_fine);
    Serial.print(buff);
    display.println(buff);
    display.display();      // Show text 

//    Serial.printf("FINE        Frequency: %.2f  RSSI: %d\n", (float)frequency_rssi.frequency_fine / 1000000.0, frequency_rssi.rssi_fine);
//    display.println("FINE        Frequency: %.2f  RSSI: %d\n", (float)frequency_rssi.frequency_fine / 1000000.0, frequency_rssi.rssi_fine);
  }

  // Deliver results coarse
  else if (frequency_rssi.rssi_coarse > rssi_threshold) {
   linecount++;
    if (linecount > 3) {
      linecount=1;
      display.clearDisplay(); 
      display.setCursor(0,0);
    }
    // Buffer the display line then output
    sprintf(buff, "COARSE Freq: %.2f   RSSI: %d\n", (float)frequency_rssi.frequency_coarse / 1000000.0, frequency_rssi.rssi_coarse);
    Serial.print(buff);
    display.println(buff);
    display.display();      // Show text 

//    Serial.printf("COARSE      Frequency: %.2f  RSSI: %d\n", (float)frequency_rssi.frequency_coarse / 1000000.0, frequency_rssi.rssi_coarse);
//    display.println("COARSE      Frequency: %.2f  RSSI: %d\n", (float)frequency_rssi.frequency_coarse / 1000000.0, frequency_rssi.rssi_coarse);
  }


  delay(10);
}
