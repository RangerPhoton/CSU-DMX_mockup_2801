#include <FastLED.h>
#include <TeensyDMX.h>

namespace teensydmx = ::qindesign::teensydmx;

// LED hardware setup
constexpr int kLEDDataPin     = 18;
constexpr int kLEDClockPin    = 19;
#define kLEDType WS2801
constexpr auto kLEDColorOrder = BGR;  

constexpr int kNumLEDs = 7;

// Display setup
constexpr int kFPS = 60;

// Set the global brightness modifier.
constexpr uint8_t kInitialBrightness = 255;

boolean serialenable = 1; // print detailed DMX In & RGB out data 

CRGB leds[kNumLEDs];

// DMX setup
// Create the DMX receiver on Serial1.
teensydmx::Receiver dmxRx{Serial1};
// To send, must use a different serial port.
//teensydmx::Sender dmxTx{Serial2};

// These are the known DMX channels, kNumLEDs lights, each with R, G, B, and W.
// Note that the non-contiguous DMX addresses - the actual fixtures 
// use 2 additional channels for tilt control (R, G, B, W, X, Y) 
constexpr int kDMXChannels[kNumLEDs][4] = {
  {1, 2, 3, 4},      // LED 1
  {7, 8, 9, 10},      // LED 2
  {13, 14, 15, 16},   // LED 3
  {19, 20, 21, 22},  // LED 4
  {25, 26, 27, 28},  // LED 5
  {31, 32, 33, 34},  // LED 6
  {37, 38, 39, 40},  // LED 7
};

// --------------------
// Main program
// --------------------

void setup() {
  // Serial monitor setup
  Serial.begin(9600);
  delay(2000); // Delay to guarantee the Serial port is set up on the Teensy

  // LED setup
  Serial.print("Setting up LEDs...");
  FastLED.addLeds<kLEDType, kLEDDataPin, kLEDClockPin, kLEDColorOrder>(leds, kNumLEDs).
      setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(kInitialBrightness);
  fill_rainbow(leds, kNumLEDs, 0, 255/kNumLEDs);
  FastLED.show();
  delay(2000);
  Serial.println("done.");

  // DMX setup
  Serial.print("Setting up DMX...");
  dmxRx.begin();
  Serial.println("done.");
}

int count = 0;
int packlength = 0;
int lost = 0;
void loop() {
  // Read DMX data
  static uint8_t buf[teensydmx::kMaxDMXPacketSize]{0};
  int read = dmxRx.readPacket(buf, 0, teensydmx::kMaxDMXPacketSize);
  if (count == 0 && read > 0) packlength=read;
  if (count > 0 && packlength < read) lost++;
  

  // Fill in the pixel values from the DMX packet
  // Only process the data if we've received valid data
  // (Technically, we should really be using a value different than zero
  // for the comparison with read.)
  if (read > 0) {
    if (serialenable) {
    Serial.print("DMX Length: ");
    Serial.print(packlength);
    Serial.print("  DMX Read: ");
    Serial.print(read);
    Serial.print("  Loop: ");
    Serial.print(count);
    Serial.print("  Lost: ");
    Serial.print(lost);
    count++;
    Serial.println();
    }
    
    for (int i = 0; i < kNumLEDs; i++) {
      const int *ledChannels = kDMXChannels[i];
      if (serialenable) {
      Serial.print("Pix ");
      Serial.print(i);
      Serial.print(" In R:");
      Serial.printf("%03d", buf[ledChannels[0]]);
      Serial.print(", G:");
      Serial.printf("%03d", buf[ledChannels[1]]);
      Serial.print(", B:");
      Serial.printf("%03d", buf[ledChannels[2]]);
      Serial.print(", W:");
      Serial.printf("%03d", buf[ledChannels[3]]);
      }
      
      CRGB rgb{buf[ledChannels[0]], buf[ledChannels[1]], buf[ledChannels[2]]};
      modifyWithWhite(&rgb, buf[ledChannels[3]]);

      // Assign it to the LED
      leds[i] = rgb;
      
      if (serialenable) Serial.println();
    }
    if (serialenable) Serial.println();
  }

  EVERY_N_MILLIS(1000 / kFPS) {
    FastLED.show();
  }
}

// Modifies the saturation of a CRGB value, given a white value.
void modifyWithWhite(CRGB *rgb, uint8_t white) {
  // From http://www.qlcplus.org/forum/viewtopic.php?t=7491
  // RGBx=RGB+(255-RGB)*W/3/255
  rgb->r += (uint16_t{255} - rgb->r)*white/3/255;
  rgb->g += (uint16_t{255} - rgb->g)*white/3/255;
  rgb->b += (uint16_t{255} - rgb->b)*white/3/255;

      if (serialenable) {
      Serial.print(" | Out R:");
      Serial.printf("%03d", rgb->r);
      Serial.print(", G:");
      Serial.printf("%03d", rgb->g);
      Serial.print(", B:");
      Serial.printf("%03d", rgb->b);
      }

  // Notes:
  // If white is 255, we want the saturation to be:
  // If white is 0, don't modify at all.

  // Cases:
  // RGB all off, white is brightness
  // RGB all full on, they may need to be dimmed

  // White may modify both saturation *and* brightness

  // Refs:
  // http://stackoverflow.com/questions/21117842/converting-an-rgbw-color-to-a-standard-rgb-hsb-rappresentation
  // http://www.qlcplus.org/forum/viewtopic.php?t=7491
}

