// CURRENT PROJECT
//written by Lukas Janky
//last edit on 24.10.2021
//use with st7789 display

#include <VescUart.h>
#include <SimpleKalmanFilter.h>


//Library for the Display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>


//#define TFT_CS    6
#define TFT_DC 9
#define TFT_RST 8
#define SCR_WD 240
#define SCR_HT 240  // 320 - to allow access to full 240x320 frame buffer
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);
//Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST, TFT_CS);


//some colors
#define Black 0x0000       /*   0,   0,   0 */
#define Navy 0x000F        /*   0,   0, 128 */
#define DarkGreen 0x03E0   /*   0, 128,   0 */
#define DarkCyan 0x03EF    /*   0, 128, 128 */
#define Maroon 0x7800      /* 128,   0,   0 */
#define Purple 0x780F      /* 128,   0, 128 */
#define Olive 0x7BE0       /* 128, 128,   0 */
#define LightGrey 0xC618   /* 192, 192, 192 */
#define DarkGrey 0x7BEF    /* 128, 128, 128 */
#define Blue 0x001F        /*   0,   0, 255 */
#define Green 0x07E0       /*   0, 255,   0 */
#define Cyan 0x07FF        /*   0, 255, 255 */
#define Red 0xF800         /* 255,   0,   0 */
#define Magenta 0xF81F     /* 255,   0, 255 */
#define Yellow 0xFFE0      /* 255, 255,   0 */
#define White 0xFFFF       /* 255, 255, 255 */
#define Orange 0xFD20      /* 255, 165,   0 */
#define GreenYellow 0xAFE5 /* 173, 255,  47 */
#define Pink 0xF81F


/** Initiate VescUart class */
VescUart UART;

int rpm;
float voltage;
float current;
int power;
float amphour;
float tach;
float distance;
float velocity;
float watthour;
float batpercentage;
float temp;


//state of a switch
int switchstate = 1;

//maximum values
int maxa = 0;
int maxvel = 0;
int maxtemp = 0;

//avg consumption wh/km
float whkm = 0;

//filter initialization
SimpleKalmanFilter Filter1(2, 2, 0.01);


//SETUP...............................................................................................................................................................
void setup() {

  /** Setup Serial port to display data */
  Serial.begin(115200);

  //Display initialization and LOADING SCREEN
  lcd.init(SCR_WD, SCR_HT);
  lcd.fillScreen(BLACK);
  lcd.setCursor(10, 100);
  lcd.setTextColor(WHITE);
  lcd.setTextSize(3);
  lcd.println("INITIALIZING");
  lcd.setCursor(85, 130);
  lcd.print("VESC");


  //Vesc reading initialization
  while (!Serial) { ; }

  /** Define which ports to use as UART */
  UART.setSerialPort(&Serial);

  pinMode(2, INPUT_PULLUP);


  delay(1000);
}


//LOOP START............................................................................................................................................................
void loop() {


  ////////// Read values //////////
  if (UART.getVescValues()) {

    rpm = (UART.data.rpm) / 7;  // The '7' is the number of pole pairs in the motor. Most motors have 14 poles, therefore 7 pole pairs
    voltage = (UART.data.inpVoltage);
    current = (UART.data.avgInputCurrent);
    power = voltage * current;
    temp = (UART.data.tempFET);
    amphour = (UART.data.ampHours);
    watthour = amphour * voltage;
    tach = (UART.data.tachometerAbs) / 42;                             // The '42' is the number of motor poles multiplied by 3
    distance = tach * 3.142 * (1.0 / 1000.0) * 0.083 * (16.0 / 36.0);  // Motor RPM x Pi x (1 / meters in a mile or km) x Wheel diameter x (motor pulley / wheelpulley)
    velocity = rpm * 3.142 * (60.0 / 1000.0) * 0.083 * (16.0 / 36.0);  // Motor RPM x Pi x (seconds in a minute / meters in a mile) x Wheel diameter x (motor pulley / wheelpulley)
    batpercentage = (((voltage - 41) / 12) + 0.04) * 100;              // ((Battery voltage - minimum voltage) / number of cells) x 100

    ////////// Filter //////////
    // calculate the estimated value with Kalman Filter
    float powerfiltered = Filter1.updateEstimate(power);


    delay(50);
  }

  ///////MAXIMUMS/////////
  if (current > maxa) {
    maxa = current;
  }

  if (velocity > maxvel) {
    maxvel = velocity;
  }

  if (temp > maxtemp) {
    maxtemp = temp;
  }


  //avg consumption

  whkm = watthour / distance;


  ////////BUTTON////////

  if (digitalRead(2) == 0) {
    switchstate++;
    delay(10);
  }

  if (switchstate == 6) {
    switchstate = 1;
  }




  ////////DSIPLAY//////


  ///DEFAULT SCREEN///
  if (switchstate == 1) {

    lcd.setTextColor(0xFFFF);
    lcd.clearScreen();
    lcd.setCursor(10, 10);
    lcd.setTextSize(2);
    lcd.print(distance, 2);
    lcd.setCursor(70, 10);
    lcd.print("km");

    lcd.drawRect(180, 2, 50, 25, 0xFFFF);
    lcd.fillRect(175, 10, 5, 10, 0xFFFF);
    lcd.setCursor(186, 8);
    lcd.setTextSize(2);
    lcd.print(batpercentage, 0);
    lcd.setCursor(215, 8);
    lcd.print("%");



    lcd.setCursor(10, 60);
    lcd.setTextSize(9);
    lcd.print(velocity, 1);

    lcd.setCursor(10, 145);
    lcd.setTextSize(5);
    lcd.print("km/h");

    lcd.drawRoundRect(10, 205, 220, 20, 50, 0xFFFF);
    lcd.fillRoundRect(10, 205, (velocity + 10) * 3.85, 20, 50, 0xFFFF);
  }


  if (switchstate == 2) {

    lcd.clearScreen();
    lcd.setTextColor(WHITE);
    lcd.setTextSize(2);

    lcd.setCursor(150, 10);
    lcd.print(voltage, 2);
    lcd.setCursor(5, 10);
    lcd.print("Voltage:");

    lcd.setCursor(150, 30);
    lcd.print(watthour, 2);
    lcd.setCursor(5, 30);
    lcd.print("Wh used:");

    lcd.setCursor(150, 50);
    lcd.print(amphour, 2);
    lcd.setCursor(5, 50);
    lcd.print("Ah used:");

    lcd.setCursor(150, 70);
    lcd.print(distance, 2);
    lcd.setCursor(5, 70);
    lcd.print("Distance:");

    lcd.setCursor(150, 90);
    lcd.print(temp, 2);
    lcd.setCursor(5, 90);
    lcd.print("Temperature:");

    lcd.setCursor(150, 110);
    lcd.print(power);
    lcd.setCursor(5, 110);
    lcd.print("Power(W):");

    lcd.setCursor(150, 130);
    lcd.print((voltage / 12) + 0.02);
    lcd.setCursor(5, 130);
    lcd.print("Single V:");

    lcd.setCursor(150, 150);
    lcd.print(rpm);
    lcd.setCursor(5, 150);
    lcd.print("RPM:");

    lcd.setCursor(150, 170);
    lcd.print(current);
    lcd.setCursor(5, 170);
    lcd.print("Current(A):");

    lcd.setCursor(150, 190);
    lcd.print(batpercentage, 0);
    lcd.setCursor(5, 190);
    lcd.print("Percentage:");

    lcd.setCursor(150, 210);
    lcd.print(velocity, 1);
    lcd.setCursor(5, 210);
    lcd.print("Speed:");
  }

  if (switchstate == 3) {

    lcd.clearScreen();
    lcd.setTextColor(WHITE);

    lcd.fillRect(0, 38, 240, 3, 0xFFFF);

    lcd.setTextSize(3);
    lcd.setCursor(20, 10);
    lcd.print("TEMPERATURE");

    lcd.setTextSize(9);
    lcd.setCursor(10, 70);
    lcd.print(temp, 1);

    lcd.setTextSize(3);
    lcd.setCursor(55, 145);
    lcd.print("celsius");

    lcd.setTextSize(3);
    lcd.setCursor(10, 200);
    lcd.print("Max:");
    lcd.setCursor(110, 200);
    lcd.print(maxtemp);
  }

  if (switchstate == 4) {

    lcd.clearScreen();
    lcd.setTextColor(WHITE);

    lcd.fillRect(0, 38, 240, 3, 0xFFFF);

    lcd.setTextSize(3);
    lcd.setCursor(40, 10);
    lcd.print("TRIP INFO");

    lcd.setTextSize(3);
    lcd.setCursor(25, 60);
    lcd.print(distance, 2);
    lcd.setCursor(45, 85);
    lcd.print("km");

    lcd.setTextSize(3);
    lcd.setCursor(145, 60);
    lcd.print(watthour, 2);
    lcd.setCursor(165, 85);
    lcd.print("Wh");

    lcd.setTextSize(4);
    lcd.setCursor(10, 160);
    lcd.print(whkm, 2);
    lcd.setTextSize(3);
    lcd.setCursor(135, 160);
    lcd.print("Wh/km");
  }

  if (switchstate == 5) {
    lcd.clearScreen();
    lcd.setTextColor(WHITE);

    lcd.fillRect(0, 38, 240, 3, 0xFFFF);

    lcd.setTextSize(3);
    lcd.setCursor(70, 10);
    lcd.print("SPEED");

    lcd.setTextSize(9);
    lcd.setCursor(10, 70);
    lcd.print(velocity, 1);

    lcd.setTextSize(3);
    lcd.setCursor(80, 145);
    lcd.print("KM/H");

    lcd.setTextSize(3);
    lcd.setCursor(10, 200);
    lcd.print("Max:");
    lcd.setCursor(110, 200);
    lcd.print(maxvel);
  }
  
}
