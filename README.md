# IRV_Electronic_Load
[IRV Electronic Load](https://unagidojyou.com/2024/04-07/irv_electronicload/)のソフトウェアです。
# 各フォルダ
## forXIAO-RP2040
XIAO向けのソフトウェアです。キャリブレーションを行った後に各種値を入力してからインストールしてください。
## forXIAO-RP2040_calibration
XIAO向けのキャリブレーションソフトです。キャリブレーションの方法は下の「キャリブレーションにつて」の章を読んでください。

# インストール時に変更が必要な部分
## キャリブレーション
次の章で書くのでここでは省略します。
## 抵抗値の変更
```
#define R1 0.47
#define R2 3300.0
#define R3 5100.0
#define R4 22000.0
#define R5 3000.0
#define R22 22000.0
#define R23 3000.0
```
実際に使用した抵抗値を入力してください。
## SOAの入力
```
#define SOA1_V 25.0
#define SOA1_I 15.0
#define SOA2_V 50.0
#define SOA2_I 4.0
#define SOA3_V 500.0
#define SOA3_I 0.005
```
使用したトランジスタ、MOSFETのSOA情報を入力してください。<br>
SOA1は、一番左(電流が最も小さい)の頂点です。そのため、SOA1_Iはトランジスタの最大電流となるはずです。SOA2はその次の頂点(無い場合もあり)で傾きが変化しているところです。SOA3は一番右(電流が最も大きい)の頂点です。そのため、SOA3_Vはトランジスタの最大電圧となるはずです。

# キャリブレーションについて
抵抗値の誤差や、使用するマイコンボードの基準電圧のズレを解消するために、キャリブレーションが必要です。<br>
キャリブレーション用のファイルをインストールし、その後、電子負荷本体に設計した電圧の0.3倍と0.9倍の電圧を印加(近すぎない任意の2電圧)、その時の表示値をめもしてください。<br>
その後、ソフトフェアの
```
#define Vmes_ref1 5.0
#define Vmes_val1 743
#define Vmes_ref2 18.0
#define Vmes_val2 2627
```
の部分に、キャリブレーション時の電圧(Vmes_ref1,Vmes_ref2)と、表示値(Vmes_val1,Vmes_val2)を入力してください。

