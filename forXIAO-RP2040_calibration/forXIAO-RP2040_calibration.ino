//IVR Electronic Load
//ESP32のADCをキャリブレーションする用のスケッチ
//設計した電圧範囲の最大値の0.9倍の値の電圧を加えた時と、0.3倍の値の電圧を加えた時の出力される値をメモしてください。

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

#define averageN 1000

void pinbegin() {
  analogReadResolution(12);
  pinMode(Vmes, INPUT);
  pinMode(Imes, INPUT);
  pinMode(SETmes, INPUT);
  pinMode(Tsens, INPUT);
  pinMode(VIR, OUTPUT);
  digitalWrite(VIR, LOW); //IorR mode
  pinMode(ONOFF, OUTPUT);
  digitalWrite(ONOFF, LOW); //OFF
  pinMode(RIV, OUTPUT);
  digitalWrite(RIV, LOW); //IorV mode → Imode
  pinMode(FANPWM, OUTPUT);
  analogWriteFreq(25000);
  analogWriteRange(4096);
  analogWrite(FANPWM, 0); //FAN 0%
  pinMode(FANONOFF, OUTPUT);
  digitalWrite(FANONOFF, LOW); //FAN OFF
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
    Vsum += analogRead(Vmes);
  }
  Serial.print("Vmes:");
  Serial.println(Vsum / averageN);
}
