#include <HardwareSerial.h>
char data_in;

HardwareSerial sim(2);
int PWRKEY = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // sim.begin(921600UL, SERIAL_8N1, 16, 17);
  sim.begin(115200, SERIAL_8N1, 16, 17); //RX TX
  // pinMode(35, OUTPUT);
  // digitalWrite(35, HIGH);
  // pinMode(PWRKEY, OUTPUT);

  // digitalWrite(PWRKEY, HIGH);
  // delay(500);
  // digitalWrite(PWRKEY, LOW);
  // delay(600);
  // digitalWrite(PWRKEY, HIGH);

}

void loop() {
  // put your main code here, to run repeatedly:
  // delay(500);
  // sim.print("AT+GMI=?");
  // while(sim.available()){
  //   data_in = sim.read();
  //   Serial.write(data_in);
  // }
  if (sim.available()){
    Serial.write(sim.read());
  }

  if (Serial.available()){
    while(Serial.available()){
      sim.write(Serial.read());
    }
    sim.println();
  }
}

