#include <sps30.h>
#include <FastLED.h>
using namespace std;

#define NUM_LEDS 125 // Antall LED i rekken
#define DATA_PIN 12 // Pin ut
#define FARGE CRGB(R,G,B)

struct CRGB leds[NUM_LEDS];

//#define PLOTTER_FORMAT
float fargevariabel;
float skalar;
float koeffisient;
float saturation = 255;
int i = 0;
float absolutt;

//____________________________STARTUP FUNKSJON__________________________________//

void setup() {
  int16_t ret;
  uint8_t auto_clean_days = 4;
  uint32_t auto_clean;
  skalar = 0;
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB rekkefølge ikke RGB

  Serial.begin(9600);
  delay(2000);

  sensirion_i2c_init();

  while (sps30_probe() != 0) {
    Serial.print("SPS sensor probing failed\n");
    delay(500);
  }

#ifndef PLOTTER_FORMAT
  Serial.print("SPS sensor probing successful\n");
#endif /* PLOTTER_FORMAT */

  ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  if (ret) {
    Serial.print("error setting the auto-clean interval: ");
    Serial.println(ret);
  }

  ret = sps30_start_measurement();
  if (ret < 0) {
    Serial.print("error starting measurement\n");
  }

#ifndef PLOTTER_FORMAT
  Serial.print("measurements started\n");
#endif /* PLOTTER_FORMAT */

#ifdef SPS30_LIMITED_I2C_BUFFER_SIZE
  Serial.print("Your Arduino hardware has a limitation that only\n");
  Serial.print("  allows reading the mass concentrations. For more\n");
  Serial.print("  information, please check\n");
  Serial.print("  https://github.com/Sensirion/arduino-sps#esp8266-partial-legacy-support\n");
  Serial.print("\n");
  delay(2000);
#endif


  delay(1000);
}

//______________________________MAIN FUNKSJON__________________________________//

void loop() {
  struct sps30_measurement m;
  char serial[SPS30_MAX_SERIAL_LEN];
  uint16_t data_ready;
  int16_t ret;

  do {
    ret = sps30_read_data_ready(&data_ready);
    if (ret < 0) {
      Serial.print("error reading data-ready flag: ");
      Serial.println(ret);
    } else if (!data_ready)
      Serial.print("data not ready, no new measurement available\n");
    else
      break;
    delay(100); /*retry in 100ms*/
  } while (1);

  ret = sps30_read_measurement(&m);
  if (ret < 0) {
    Serial.print("error reading measurement\n");
  } else {

#ifndef PLOTTER_FORMAT
    Serial.print("PM  1.0: ");
    Serial.println(m.mc_1p0);
    Serial.print("PM  2.5: ");
    Serial.println(m.mc_2p5);
    Serial.print("PM  4.0: ");
    Serial.println(m.mc_4p0);
    Serial.print("PM 10.0: ");
    Serial.println(m.mc_10p0);
    //Serial.print(millis());

#ifndef SPS30_LIMITED_I2C_BUFFER_SIZE
    Serial.print("NC  0.5: ");
    Serial.println(m.nc_0p5);
    Serial.print("NC  1.0: ");
    Serial.println(m.nc_1p0);
    Serial.print("NC  2.5: ");
    Serial.println(m.nc_2p5);
    Serial.print("NC  4.0: ");
    Serial.println(m.nc_4p0);
    Serial.print("NC 10.0: ");
    Serial.println(m.nc_10p0); // Antall partikler av størrelsen lik eller mindre enn 10 mikrometer
    
/*
    Serial.print("Typical partical size: ");
    Serial.println(m.typical_particle_size);*/
#endif

    Serial.println();

#else
    // since all values include particles smaller than X, if we want to create buckets we
    // need to subtract the smaller particle count.
    // This will create buckets (all values in micro meters):
    // - particles        <= 0,5
    // - particles > 0.5, <= 1
    // - particles > 1,   <= 2.5
    // - particles > 2.5, <= 4
    // - particles > 4,   <= 10

    Serial.print(m.nc_0p5);
    Serial.print(" ");
    Serial.print(m.nc_1p0  - m.nc_0p5);
    Serial.print(" ");
    Serial.print(m.nc_2p5  - m.nc_1p0);
    Serial.print(" ");
    Serial.print(m.nc_4p0  - m.nc_2p5);
    Serial.print(" ");
    Serial.print(m.nc_10p0 - m.nc_4p0);
    Serial.println();
    Serial.print("Else");


#endif /* PLOTTER_FORMAT */

  }

//____________________________LED FUNKSJON__________________________________//


fargevariabel = m.nc_2p5;

if (fargevariabel < 50 && fargevariabel > 0){koeffisient = 27;}{saturation = 255;}          //Det vi kan gjøre er å definere en grense som er konstant, og si alt over eller likt den er rød,alt under er porsjonert på 100, dersom man ønsker fra grønn til rød, deler
if (fargevariabel < 100 && fargevariabel > 50){koeffisient = 4;}{saturation = 255;}
if (fargevariabel < 500 && fargevariabel > 100){koeffisient = 1;}{saturation = 255;}
if (fargevariabel < 5000 && fargevariabel > 500){koeffisient = 1/4;}{saturation = 255;}
if (fargevariabel < 50000 && fargevariabel > 5000){koeffisient = 1/8;}{saturation = 255;}

if (fargevariabel >= 50000){
  if (i%2 == 0){
    saturation = 0;
    i = i+1;
   }
   else if(i%2 == 1){
    koeffisient = 0;
    saturation = 255;
    i=i+1;
   }
}


{skalar = 255 - (fargevariabel*koeffisient);}
{absolutt = abs(skalar);}

    Serial.print("Fargevaiabel: ");
    Serial.println(fargevariabel , 2);
    Serial.print("Koeffisiet: ");
    Serial.println(koeffisient, 2);
    Serial.print("Skalar: ");
    Serial.println(skalar , 2);
    Serial.print("Absoluttverdi skalar: ");
    Serial.println(absolutt);
/* 
 if (i%2==0){skalar = 100; ++i;}
 else if (i%2==1){skalar = 100; ++i;}
  skalar = skalar + 10;
*/

 for (int i=0; i < NUM_LEDS; i++){
    leds[i] = CHSV(absolutt, saturation,255);

  }


  FastLED.show();
  Serial.print(" \n -------II------- \n \n");

 //____________________________DELAY__________________________________//


  delay(1000);

  
}
