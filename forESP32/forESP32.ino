#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

//ピン配置
/*
//XIAO ESP32C3
#define Vmes 2
#define Imes 3
#define SETmes 4
#define Tsens 5
#define OLEDSDA 6
#define OLEDSCL 7
#define VIR 21
#define ONOFF 10
#define Button 9
#define RIV 8
#define FANPWM 20
*/
//ESP32 Devboard
#define Vmes 36
#define Imes 39
#define SETmes 34
#define Tsens 35
#define OLEDSDA 21
#define OLEDSCL 22
#define VIR 32
#define ONOFF 2   //onboard led
#define Button 0  //onboard
#define RIV 33
#define FANPWM 25
#define FANONOFF 26
#define FANmes 27

#define Vmes_ref1 2.5
#define Vmes_val1 2543
#define Vmes_ref2 1.3
#define Vmes_val2 1328
#define R1 0.5
#define R2 3300
#define R3 5000
#define R4 22000
#define R5 3000
#define R22 0
#define R23 3000

//TK15J50DのSOA情報
#define SOA1_V 25
#define SOA1_I 15
#define SOA2_V 50
#define SOA2_I 4
#define SOA3_V 500
#define SOA3_I 0.005

//ADCのサンプリング数
#define averageN 1000

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C  //0x78のときは0x3C、0x7Aのときは0x3A

#define Imode 0;
#define Rmode 1;
#define Vmode 2;

bool ONOFFstatus = false;
int MODEstatus = Imode;

const float aV = (Vmes_ref2 - Vmes_ref1) / (Vmes_val2 - Vmes_val1);
const float bV = Vmes_ref1 - (aV * Vmes_val1);
const float a = aV / (R23 / (R22 + R23));
const float b = bV;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

void pinbegin() {
  pinMode(Vmes, INPUT);
  pinMode(Imes, INPUT);
  pinMode(SETmes, INPUT);
  pinMode(Tsens, INPUT);
  pinMode(VIR, OUTPUT);
  digitalWrite(VIR, LOW);
  pinMode(ONOFF, OUTPUT);
  digitalWrite(ONOFF, LOW);
  pinMode(Button, INPUT_PULLUP);
  pinMode(RIV, OUTPUT);
  digitalWrite(RIV, LOW);
  pinMode(RIV, FANPWM);
  ledcSetup(0, 25000, 8);
  ledcAttachPin(FANPWM, 0);
  ledcWrite(0, 25);
  pinMode(FANONOFF, OUTPUT);
  digitalWrite(FANONOFF, LOW);
  pinMode(FANmes, INPUT_PULLUP);
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
  display.print(V);
  display.println("V");
  if (MODEstatus == 0) {
    display.print("fixI ");
  } else if (MODEstatus == 1) {
    display.print("fixR ");
  } else {
    display.print("fixV ");
  }
  display.print(I);
  display.println("A");
  display.print(SET);
  if (MODEstatus == 0) {
    display.print("A");
  } else if (MODEstatus == 1) {
    display.print("V");
  } else {
    display.print("R");
  }
  display.print(W);
  display.println("W");
  display.print("     ");
  display.print(T);
  display.println("C");
  display.display();
}

bool checkSOA(float V, float I) {
  if (V <= SOA1_V) {  //１つ目の領域
    if (I <= SOA1_I) {
      return true;
    } else {
      return false;
    }
  }
  if (V <= SOA2_V) {
    float SOA1a = (SOA2_I - SOA1_I) / (SOA2_V - SOA1_V);
    float SOA1b = SOA1_I - (SOA1a * SOA1_V);
    if (I <= (SOA1a * V + SOA1b)) {  //2つ目の領域
      return true;
    } else {
      return false;
    }
  }
  if (V <= SOA3_V) {
    float SOA2a = (SOA3_I - SOA2_I) / (SOA3_V - SOA2_V);
    float SOA2b = SOA2_I - (SOA2a * SOA2_V);
    if (I <= (SOA2a * V + SOA2b)) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

int count = 0;

void loop() {
  unsigned long Vsum = 0;
  unsigned long Isum = 0;
  unsigned long SETsum = 0;
  unsigned long Tsum = 0;
  for (int i = 0; i < averageN; i++) {
    Vsum += analogReadMilliVolts(Vmes);
    Isum += analogReadMilliVolts(Imes);
    SETsum += analogReadMilliVolts(SETmes);
    Tsum += analogReadMilliVolts(Tsens);
  }
  float V = aV * (Vsum / averageN) + bV;
  if (V < 0) {
    V = 0;
  }
  float I = (a * (Isum / averageN) + b) / R1;
  if (I < 0 || !ONOFFstatus) {
    I = 0;
  }
  float SET = a * (SETsum / averageN) + b;
  if (SETsum < 0) {
    SETsum = 0;
  }
  if (MODEstatus == 0) {  //Imode
    SET = SET / R1;
  } else if (MODEstatus == 1) {  //Rmode
    SET = R1 * V / SET;
  } else {  //Vmode
    SET = SET * (R4 + R5) / R5;
  }
  int T = 0.01 / (a * (Tsum / averageN) + b - 0.5);

  bool SOAover = false;
  if (ONOFFstatus) {
    if (!checkSOA(V, I)) {
      digitalWrite(ONOFF, LOW);
      ONOFFstatus = false;
      SOAover = true;
    }
  } else {
    if (MODEstatus == 0) {  //Imode
      SOAover = !checkSOA(V, SET);
    } else if (MODEstatus == 1) {  //Rmode
      SOAover = !checkSOA(V, V / SET);
    } else {  //Vmode
      SOAover = (SET < V);
    }
  }
  count++;
  if (count == 500 || SOAover) {  //about 500ms
    drawdisplay(V, I, T, SET, SOAover);
    count = 0;
  }
}
