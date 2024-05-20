#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

//ピン割り当て
#define Vmes 26
#define Imes 27
#define SETmes 28
#define Tsens 29
#define OLEDSDA 6
#define OLEDSCL 7
#define FANPWM 0
#define ONOFF 3
#define RIV 4
#define VIR 2
#define FANONOFF 1

//変更してください
#define Vmes_ref1 5.0     //1つ目のキャリブレーションに使用した電圧
#define Vmes_val1 743.0   //上の時の表示値
#define Vmes_ref2 18.0    //2つ目のキャリブレーションに使用した電圧
#define Vmes_val2 2627.0  //上の時の表示値
//以下は使用した抵抗値を入力してください
#define R1 0.5
#define R2 3300.0
#define R3 5100.0
#define R4 22000.0
#define R5 3000.0
#define R22 22000.0
#define R23 3000.0

//使用したトランジスタ(MOSFET)のSOAの情報を書いてください。

//TK15J50DのSOA情報
#define SOA1_V 25.0
#define SOA1_I 15.0
#define SOA2_V 50.0
#define SOA2_I 4.0
#define SOA3_V 500.0
#define SOA3_I 0.005


//ADCのサンプリング数
#define averageN 1000

//FAN関係
#define FANONTEMP 40.0   //この温度以上でONにする
#define FANSETTEMP 50.0  //PID制御での目標値
#define MINPWM 0.2     //PWMを最小にしたときの回転数の割合(0.2=20%)
#define KP 1000.0        //比例要素
#define KI 0.02        //積分要素
#define KD 1000        //微分要素

//OLED関係
#define SCREEN_ADDRESS 0x3C  //0x78のときは0x3C、0x7Aのときは0x3A
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels

#define IMODE 0
#define RMODE 1
#define VMODE 2

bool ONOFFstatus = false;
int MODEstatus = IMODE;
bool Fsetup = false;

const float aV = float((Vmes_ref2 - Vmes_ref1) / (Vmes_val2 - Vmes_val1));
const float bV = float(Vmes_ref1 - (aV * Vmes_val1));
//const float a = aV * (R23 / (R22 + R23));
//const float b = bV * (R23 / (R22 + R23));
const float a = 3.3 / 4095.0;
const float b = 0;

const float SOA1a = static_cast<float>((SOA2_I - SOA1_I) / (SOA2_V - SOA1_V));
const float SOA1b = static_cast<float>(SOA1_I - (SOA1a * SOA1_V));
const float SOA2a = static_cast<float>((SOA3_I - SOA2_I) / (SOA3_V - SOA2_V));
const float SOA2b = static_cast<float>(SOA2_I - (SOA2a * SOA2_V));

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
  digitalWrite(RIV, LOW);  //IorV mode → IMODE
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

int digits(int num) {
  if (num < 10) {
    return 1;
  } else if (num < 100) {
    return 2;
  } else if (num < 1000) {
    return 3;
  }
  return 4;
}

bool blink = false;
void drawdisplay(float V, float I, int T, float SET, bool SOAover, int fanper) {
  float W = V * I;
  char buff[50];
  sprintf(buff, "V:%2.1f I:%2.1f W:%3.1f T:%d MODE:%d SET:%2.1f", V, I, W, T, MODEstatus, SET);
  //Serial.println(buff);
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
  display.print(fanper);
  display.print("% ");
  for (int i = 0; i < 3 - digits(fanper); i++) {
    display.print(" ");
  }
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
  delay(5);
  unsigned long sum = 0;
  for (int i = 0; i < samplingN; i++) {
    sum += analogRead(pinnum);
  }
  float result = sum / samplingN;
  return result;
}

bool fanonhold = false;
bool fanonoff(int temp) {
  if (temp >= FANONTEMP) {  //閾値よりも高い時
    if (ONOFFstatus) {      //ONの時
      fanonhold = true;
      return true;
    } else {  //OFFの時
      fanonhold = false;
      return true;
    }
  } else {               //閾値よりも低い時
    if (!ONOFFstatus) {  //OFFの時
      return false;
    } else if (fanonhold) {
      return true;
    }
  }
  return false;
}

double integral = 0.0;
float lastError = 0.0;
unsigned long lastTime = 0;
int fanpid(float nowtemp, int settemp) {
  unsigned long now = millis();
  float timeChange = static_cast<float>(now - lastTime);
  float error = settemp - nowtemp;
  integral += (error * timeChange);
  float derivative = (error - lastError) / timeChange;
  float output = static_cast<float>((KP * error) + (KI * integral) + (KD * derivative));
  /*Serial.print(output);
  Serial.print(" ");
  Serial.print((float)KP * error);
  Serial.print(" ");
  Serial.print((float)KI * integral);
  Serial.print(" ");
  Serial.print((float)KD * derivative);
  Serial.print(" ");*/
  output = 0 - output;
  if (output > 4095) output = 4095;
  else if (output < MINPWM * 4095) output = MINPWM * 4095;
  lastError = error;
  lastTime = now;
  if (integral > 100000) integral = 100000;
  else if (integral < -100000) integral = -100000;
  //Serial.println((int)output);
  return (int)output;
}

int count = 500;
bool SOAover = true;
float V = 0;
float I = 0;
float SET = 0;
float T = 0;
void loop() {
  V = static_cast<float>((aV * mespin(Vmes, averageN)) + bV);
  if (V < 0) V = 0;
  I = static_cast<float>((a * mespin(Imes, averageN) + b) / R1);
  if (I < 0 || !ONOFFstatus) I = 0;

  SET = (a * mespin(SETmes, averageN)) + b;
  if (SET < 0) SET = 0;
  switch (MODEstatus) {
    case IMODE:
      SET = static_cast<float>(SET / R1);
      break;
    case RMODE:
      SET = static_cast<float>(R1 * V / SET);
      break;
    case VMODE:
      SET = static_cast<float>(SET * (R4 + R5) / R5);
      break;
  }
  T = 100 * (a * mespin(Tsens, averageN) + b - 0.5);

  if (ONOFFstatus) {  //ON
    if (!checkSOA(V, I)) {
      digitalWrite(ONOFF, LOW);
      ONOFFstatus = false;
      SOAover = true;
    } else {
      SOAover = false;
    }
  } else {  //OFF
    switch (MODEstatus) {
      case IMODE:
        SOAover = !checkSOA(V, SET);
        break;
      case RMODE:
        SOAover = !checkSOA(V, V / SET);
        break;
      case VMODE:
        SOAover = (SET < V);
        break;
    }
  }

  int fanpwm = 0;
  if (fanonoff(T)) {
    digitalWrite(FANONOFF, HIGH);
    //analogWrite(FANPWM, 4095);  //FAN 100%
    fanpwm = fanpid(T, FANSETTEMP);
    analogWrite(FANPWM, fanpwm);
  } else {
    digitalWrite(FANONOFF, LOW);
    //analogWrite(FANPWM, 0);  //FAN 0%
    integral = 0;
    lastError = 0;
    lastTime = 0;
  }

  count++;
  if (count >= 1 || SOAover) {  //about 500ms
    drawdisplay(V, I, (int)T, SET, SOAover, 100.0 * fanpwm / 4095.0);
    count = 0;
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
          switch (MODEstatus) {
            case IMODE:
              MODEstatus = RMODE;
              digitalWrite(RIV, HIGH);
              digitalWrite(VIR, LOW);
              break;
            case RMODE:
              MODEstatus = VMODE;
              digitalWrite(RIV, LOW);
              digitalWrite(VIR, HIGH);
              break;
            case VMODE:
              MODEstatus = IMODE;
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
