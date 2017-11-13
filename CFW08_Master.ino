#include <ModbusRtu.h>
#include <SoftwareSerial.h>

//Arrays de dados a receber
uint16_t au16dataRX[32];
//Arrays de dados a transmitir
uint16_t au16dataTX[16];
uint16_t au16OnOffTX[1];
uint16_t au16FrequencyTX[2];

uint8_t byteTX[32];
uint8_t u8state;

Modbus master(0, 1, 2);

//Funções Modbus
modbus_t readRegisters;
modbus_t writeRegisters;
modbus_t writeOnOff;
modbus_t writeFrquency;

//Porta Bluetooth
SoftwareSerial mySerial(10, 11); // RX, TX

unsigned long u32wait;

void setup() {

  //read registers
  readRegisters.u8id = 1;                 // slave address
  readRegisters.u8fct = 3;                // function code (registers read multiple  3)
  readRegisters.u16RegAdd = 3;            // start address in slave -  direccion de Inicio 0
  readRegisters.u16CoilsNo = 3;          // number of elements (coils or registers) to read  0 - 16
  readRegisters.au16reg = au16dataRX;       // pointer to a memory array in the Arduino - Almacenamiento en Array de memoria de arduino

  //write a multiple  register = function 16
  writeRegisters.u8id = 1;                 // slave address
  writeRegisters.u8fct = 16;               // function code (write multiple registers 16 )
  writeRegisters.u16RegAdd = 1;           // start address in slave  -  direccion de Inicio 10
  writeRegisters.u16CoilsNo = 1;          // number of elements (coils or registers) to read
  writeRegisters.au16reg = au16dataTX;

  //envio de frequencia para o inversor
  writeFrquency.u8id = 1;
  writeFrquency.u8fct = 6;
  writeFrquency.u16RegAdd = 5004;            //Variável básica V04 efetua o controle de velocidade (manual CFW08)
  writeFrquency.u16CoilsNo = 1;
  writeFrquency.au16reg = au16FrequencyTX;

  //Comando do inversor
  writeOnOff.u8id = 1;
  writeOnOff.u8fct = 6;
  writeOnOff.u16RegAdd = 5003;            //Variável básica V03 contém os bits de comando (manual CFW08)
  writeOnOff.u16CoilsNo = 1;
  writeOnOff.au16reg = au16OnOffTX;

  Serial.begin(9600);
  master.begin( 9600 );
  mySerial.begin(9600);
  master.setTimeOut( 2000 ); // if there is no answer in 2000 ms, roll over
  u32wait = millis() + 1000;
}

void loop() {
  switch ( u8state ) {
    case 0:
      if (millis() > u32wait)
        u8state++; // wait state
      break;
    case 1:
      master.query( readRegisters );
      u8state++;
      break;
    case 2:
      //au16OnOffTX[0] = 0xFF00;
      //master.query( writeOnOff );
      u8state++;
      break;
    case 3:
      //au16FrequencyTX[0] = 4500;
      //master.query( writeFrquency );
      u8state++;
      break;
    case 4:
      master.poll(); // check incoming messages
      if (master.getState() == COM_IDLE) {
        byteTX[0] = highByte(au16dataRX[0]);
        byteTX[1] = lowByte(au16dataRX[0]);
        byteTX[2] = highByte(au16dataRX[1]);
        byteTX[3] = lowByte(au16dataRX[1]);
        byteTX[4] = highByte(au16dataRX[2]);
        byteTX[5] = lowByte(au16dataRX[2]);

        /*Serial.println(au16dataRX[0]);
          Serial.println(au16dataRX[1]);
          Serial.println(au16dataRX[2]);
          Serial.println(au16dataRX[3]);
          Serial.println(au16dataRX[4]);
          Serial.println(au16dataRX[5]);
          Serial.println(au16dataRX[6]);*/

        if (!master.getTimeOutState()) {
          mySerial.write(byteTX[4]);
          mySerial.write(byteTX[5]);
          mySerial.write(byteTX[0]);
          mySerial.write(byteTX[1]);
          mySerial.write(byteTX[2]);
          mySerial.write(byteTX[3]);
        }
        else {
          mySerial.write("!");
          mySerial.write("!");
          mySerial.write("!");
          mySerial.write("!");
          mySerial.write("!");
        }
        u8state = 0;
        u32wait = millis() + 1000;
      }
      break;
  }

  if (mySerial.available() > 0) {
    String str;
    String command;
    while (mySerial.available() > 0) {
      int inChar = mySerial.read();
      delayMicroseconds(2000);
      str += String(inChar);
      command += String(char(inChar));
    }
    if (command.length() > 4) {
      if (command == "LIGAR")
        au16OnOffTX[0] = 0xF303;
      else if (command == "DESLIGAR")
        au16OnOffTX[0] = 0xF300;
      else if (command == "HHHHHHHH")
        au16OnOffTX[0] = 0xF404;
      else if (command == "AHAHAHAH")
        au16OnOffTX[0] = 0xF400;
      master.query( writeOnOff );
    } else {
      au16FrequencyTX[0] = str.toInt();
      master.query(writeFrquency);
    }
  }
  master.poll();
}
