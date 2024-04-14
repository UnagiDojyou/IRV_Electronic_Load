#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define Vmes 29
#define Imes 28
#define SETmes 27
#define Tsens 26
#define OLEDSDA 6
#define OLEDSCL 7
#define VIR 3
#define ONOFF 2
#define RIV 4
#define FANPWM 0
#define FANONOFF 1

#define Vmes_ref1 5.0
#define Vmes_val1 743
#define Vmes_ref2 18.0
#define Vmes_val2 2627
#define R1 0.47
#define R2 3300.0
#define R3 5100.0
#define R4 22000.0
#define R5 3000.0
#define R22 22000.0
#define R23 3000.0

//TK15J50DのSOA情報
#define SOA1_V 25.0
#define SOA1_I 15.0
#define SOA2_V 50.0
#define SOA2_I 4.0
#define SOA3_V 500.0
#define SOA3_I 0.005

//ADCのサンプリング数
#define averageN 1000

//FANをONにする温度
#define FANONTEMP 40

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C  //0x78のときは0x3C、0x7Aのときは0x3A

#define Imode 0;
#define Rmode 1;
#define Vmode 2;

bool ONOFFstatus = false;
int MODEstatus = Imode;
bool Fsetup = false;

const float aV = (Vmes_ref2 - Vmes_ref1) / (Vmes_val2 - Vmes_val1);
const float bV = Vmes_ref1 - (aV * Vmes_val1);
//const float a = aV * (R23 / (R22 + R23));
//const float b = bV * (R23 / (R22 + R23));
const float a = 3.3 / 4095;
const float b = 0;

const float SOA1a = (SOA2_I - SOA1_I) / (SOA2_V - SOA1_V);
const float SOA1b = SOA1_I - (SOA1a * SOA1_V);
const float SOA2a = (SOA3_I - SOA2_I) / (SOA3_V - SOA2_V);
const float SOA2b = SOA2_I - (SOA2a * SOA2_V);


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

void pinbegin() {
  analogReadResolution(12);
  pinMode(Vmes, INPUT);
  pinMode(Imes, INPUT);
  pinMode(SETmes, INPUT);
  pinMode(Tsens, INPUT);
  pinMode(VIR, OUTPUT);
  digitalWrite(VIR, LOW);  //IorR mode
  pinMode(ONOFF, OUTPUT);
  digitalWrite(ONOFF, LOW);  //OFF
  pinMode(RIV, OUTPUT);
  digitalWrite(RIV, LOW);  //IorV mode → Imode
  pinMode(FANPWM, OUTPUT);
  analogWriteFreq(25000);
  analogWriteRange(4096);
  analogWrite(FANPWM, 0);  //FAN 0%
  pinMode(FANONOFF, OUTPUT);
  digitalWrite(FANONOFF, LOW);  //FAN OFF
}

void openingceremony() {
  Serial.println("Welcome to IRV Electronic Load");
  Serial.println("unagidojyou.com");
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("     Welcome to"));
  display.setTextSize(2);
  display.println(F("    IRV"));
  display.println(F("Electronic"));
  display.println(F("   Load"));
  display.setTextSize(1);
  display.println(F("   unagidojyou.com"));
  display.display();
}

void setup() {
  // put your setup code here, to run once:
  pinbegin();
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  openingceremony();
  delay(2000);
  Fsetup = true;
}

void setup1() {
  while (!Fsetup) {
    delay(10);
  }
}

String digit(float num) {
  char cnum[10];
  if (num < 10) {
    sprintf(cnum, "%1.2f", num);
  } else if (num < 100) {
    sprintf(cnum, "%2.1f", num);
  } else {
    sprintf(cnum, "%d", num);
  }
  return cnum;
}

bool blink = false;
void drawdisplay(float V, float I, int T, float SET, bool SOAover) {
  float W = V * I;
  char buff[50];
  sprintf(buff, "V:%2.1f I:%2.1f W:%3.1f T:%d MODE:%d SET:%2.1f", V, I, W, T, MODEstatus, SET);
  Serial.println(buff);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  if (SOAover) {
    if (blink) {
      display.print("     ");
      blink = false;
    } else {
      display.print("!SOA!");
      blink = true;
    }
  } else {
    if (ONOFFstatus) {
      display.print("ON   ");
    } else {
      display.print("OFF  ");
    }
  }
  //sprintf(buff,);
  display.print(digit(V));
  display.println("V");
  if (MODEstatus == 0) {
    display.print("CC   ");
  } else if (MODEstatus == 1) {
    display.print("CR   ");
  } else {
    display.print("CV   ");
  }
  display.print(digit(I));
  display.println("A");
  display.print(digit(SET));
  if (MODEstatus == 0) {
    display.print("A");
  } else if (MODEstatus == 1) {
    display.print("R");
  } else {
    display.print("V");
  }
  display.print(digit(W));
  display.println("W");
  display.print("     ");
  display.print(T);
  display.println("C");
  display.display();
}

bool checkSOA(float V, float I) {  //trueでOK falseでOUT
  if (V <= SOA1_V) {               //１つ目の領域
    if (I <= SOA1_I) {
      return true;
    } else {
      return false;
    }
  }
  if (V <= SOA2_V) {
    float Imax = SOA1a * V + SOA1b;
    if (I <= Imax) {  //2つ目の領域
      return true;
    } else {
      return false;
    }
  }
  if (V <= SOA3_V) {
    float Imax = SOA2a * V + SOA2b;
    if (I <= Imax) {  //3つ目の領域
      return true;
    } else {
      return false;
    }
  }
  return false;
}

float mespin(int pinnum, int samplingN) {
  analogRead(pinnum);
  delay(1);
  unsigned long sum = 0;
  for (int i = 0; i < samplingN; i++) {
    sum += analogRead(pinnum);
  }
  float result = sum / samplingN;
  return result;
}

bool fanonhold = false;
bool fanonoff(int temp) {
  if (temp >= FANONTEMP) {
    if (ONOFFstatus) {  //ONの時
      fanonhold = true;
      return true;
    } else {  //OFFの時
      fanonhold = false;
      return true;
    }
  } else {
    if (!ONOFFstatus) {
      return false;
    } else if (fanonhold) {
      return true;
    }
  }
  return false;
}

int count = 500;
bool SOAover = true;

void loop() {
  float V = (aV * mespin(Vmes, averageN)) + bV;
  if (V < 0) {
    V = 0;
  }
  float I = (a * mespin(Imes, averageN) + b) / R1;
  if (I < 0 || !ONOFFstatus) {
    I = 0;
  }
  float SET = (a * mespin(SETmes, averageN)) + b;
  if (SET < 0) {
    SET = 0;
  }
  if (MODEstatus == 0) {  //Imode
    SET = SET / R1;
  } else if (MODEstatus == 1) {  //Rmode
    SET = R1 * V / SET;
  } else {  //Vmode
    SET = SET * (R4 + R5) / R5;
  }
  int T = 100 * (a * mespin(Tsens, averageN) + b - 0.5);

  if (ONOFFstatus) {  //ON
    if (!checkSOA(V, I)) {
      digitalWrite(ONOFF, LOW);
      ONOFFstatus = false;
      SOAover = true;
    } else {
      SOAover = false;
    }
  } else {                  //OFF
    if (MODEstatus == 0) {  //Imode
      SOAover = !checkSOA(V, SET);
    } else if (MODEstatus == 1) {  //Rmode
      SOAover = !checkSOA(V, V / SET);
    } else {  //Vmode
      SOAover = (SET < V);
    }
  }
  count++;
  if (count >= 1 || SOAover) {  //about 500ms
    drawdisplay(V, I, T, SET, SOAover);
    count = 0;
  }
  if (fanonoff(T)) {
    digitalWrite(FANONOFF, HIGH);
    analogWrite(FANPWM, 4095);  //FAN 100%
  } else {
    digitalWrite(FANONOFF, LOW);
    analogWrite(FANPWM, 0);  //FAN 0%
  }
}

void loop1() {
  if (BOOTSEL) {
    int time = 0;
    bool pushed = false;
    while (BOOTSEL) {
      delay(10);
      time += 10;
      if (!pushed && time >= 4 * 100) {  //4s
        if (!ONOFFstatus) {
          if (MODEstatus == 0) {  //Imode
            MODEstatus = 1;       //Rmode
            digitalWrite(RIV, HIGH);
            digitalWrite(VIR, LOW);
          } else if (MODEstatus == 1) {  //Rmode
            MODEstatus = 2;              //Vmode
            digitalWrite(RIV, LOW);
            digitalWrite(VIR, HIGH);
          } else {           //Vmode
            MODEstatus = 0;  //Imode
            digitalWrite(RIV, LOW);
            digitalWrite(VIR, LOW);
          }
        }
        pushed = true;
      }
    }
    if (!pushed) {
      if (!ONOFFstatus && !SOAover) {
        ONOFFstatus = true;
        digitalWrite(ONOFF, HIGH);
      } else if (ONOFFstatus) {
        digitalWrite(ONOFF, LOW);
        ONOFFstatus = false;
      }
      pushed = true;
    }
  }
  delay(1);
}
