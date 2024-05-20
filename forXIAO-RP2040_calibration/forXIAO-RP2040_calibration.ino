//IVR Electronic Load
//XIAOのADCをキャリブレーションする用のスケッチ
//設計した電圧範囲の最大値の0.9倍の値の電圧を加えた時と、0.3倍の値の電圧を加えた時の出力される値をメモしてください。

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
