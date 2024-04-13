//IVR Electronic Load
//ESP32のADCをキャリブレーションする用のスケッチ
//設計した電圧範囲の最大値の0.9倍の値の電圧を加えた時と、0.3倍の値の電圧を加えた時の出力される値をメモしてください。

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

#define averageN 1000

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

void setup() {
  // put your setup code here, to run once:
  pinbegin();
  Serial.begin(115200);
}

void loop() {
  delay(1000);
  unsigned long Vsum = 0;
  for (int i = 0; i < averageN; i++) {
    Vsum += analogReadMilliVolts(Vmes);
  }
  Serial.print("Vmes:");
  Serial.println(Vsum / averageN);
}
