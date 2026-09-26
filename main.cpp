///////////////////////////////////////////////// SETUP //////////////////////////////////////////////////////

//calls in libraries
#include <Arduino.h> //general system
#include <Adafruit_BNO08x.h> //distance/acceleration sensor
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <math.h> //math functions
#include <Wire.h>
#include <time.h>


//define display screen
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);


//define variables
#define BNO08X_RESET -1
void setReports();


//define buttons
//int pinD1 = 1;
//int pinD2 = 2;


//define states for "menuState"
enum menuState {
  steps_taken, //0
  distance_travelled, //1
  stride_length, //2
  raw_data, //3
  mCount
};

//define initial state of "menuState"
menuState menuMode = steps_taken; //device starts on "steps_taken" screen


//define misc variables
Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

float cust_stride_length = 50.; //inches
float step_count = 0.; 

long debounceTime = 80; 

volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;

volatile bool menuButtonFlag = false;
volatile bool strideButtonFlag = false;



//defines "menu_change" button accounting for "debounce" (D1)
void IRAM_ATTR menu_change_button() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    menuButtonFlag = true;
    prevChangeTime = now; 
  }
}


//defines "stride_change" button accounting for "debounce" (D2)
void IRAM_ATTR stride_change_button() {
  long now = millis();
  if (now > prevChangeTimeTwo + debounceTime) {
    strideButtonFlag = true;
    prevChangeTimeTwo = now; 
  }
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  } else {
    Serial.println("Set accelerometer report... success!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  //sets initial state of D1
  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), menu_change_button, RISING);

  //sets initial state of D2
  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), stride_change_button, RISING);
  
  //turn on screen
  display.init(135, 240);
  display.setRotation(3);
  canvas.setTextColor(ST77XX_GREEN);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);
  canvas.setTextSize(2);

  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("BNO08x Found!");

  setReports();
}

///////////////////////////////////////////////// MAIN CODE //////////////////////////////////////////////////////
void loop() {
  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }
  
  //define float variables
  float x = sensorValue.un.accelerometer.x;
  float y = sensorValue.un.accelerometer.y;
  float z = sensorValue.un.accelerometer.z;
  float alpha = atan2(x, sqrt(y*y + z*z)) * RAD_TO_DEG; //angle about x-axis
  float beta = atan2(y, z) * RAD_TO_DEG; //angle about y-axis
  
  float distance = (step_count * cust_stride_length)/12;

  float sq_mag = x*x + y*y + z*z;

  if (sq_mag >= 150) {
    step_count += 1.0;
    //Serial.println("step taken :D");
    delay(300);
  }
  
  //D1 changes menu mode if pressed
  if (menuButtonFlag) {
    menuButtonFlag = false;
    menuMode = (menuState) (((int)menuMode + 1) % (int)menuState::mCount); //button press cycles between menu options (steps/distance/stride)
  }

  //menu 0 ("steps_taken"), screen displays the number of steps taken
  if (menuMode == steps_taken) {
    canvas.fillScreen(ST77XX_ORANGE); //sets background to blue
    canvas.setCursor(0, 20); //sets start position of text   
    canvas.println("# of steps taken: ");
    canvas.print(step_count);
    delay(100);
  }

  //menu 1 ("distance_travelled"), screen displays the total distance travelled
  if (menuMode == distance_travelled) {
    canvas.fillScreen(ST77XX_ORANGE); //sets background to blue
    canvas.setCursor(0, 20); //sets start position of text 
    canvas.println("Distance travelled: ");
    canvas.print(distance);
    canvas.print(" [ft]");
    delay(100);
  }

  //menu 2 ("stride_length"), screen displays stride length
  if (menuMode == stride_length) {
    canvas.fillScreen(ST77XX_ORANGE); //sets background to blue
    canvas.setCursor(0, 20); //sets start position of text       
    canvas.println("Stride length: ");
    canvas.print(cust_stride_length);
    canvas.print(" [in]");
    delay(100);

    //D2 cycles through "cust_stride_length" values - based on avg stride length of 50-65 inches
    if (strideButtonFlag) {
      strideButtonFlag = false;

      cust_stride_length += 1.0;
      if (cust_stride_length > 65.0) {
        cust_stride_length = cust_stride_length - 16.;
      }

    }

  }

  //menu 3 ("raw_data"), screen displays raw data from sensor
  if (menuMode == raw_data) {
      canvas.fillScreen(ST77XX_ORANGE); //sets background to blue
      canvas.setCursor(0, 20); //sets start position of text  
      canvas.println("Accelerometer Data");
      canvas.print("x: ");
      canvas.println(x);
      canvas.print("y: ");
      canvas.println(y);
      canvas.print("z: ");
      canvas.println(z);
      canvas.print("x-angle: ");
      canvas.println(alpha);
      canvas.print("y-angle: ");
      canvas.println(beta);
      Serial.println(sq_mag);
      delay(100); 
  }

  //having this at the end of the code ensures that all messages print to the screen correctly
  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
  delay(50);
}

//?// UNIT CHANGE OPTION //?//
//?// RESET OPTION FOR STEP COUNTER //?//
