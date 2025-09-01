/* Lecteur et Analyseur de TeleCarte Française v0.4 - par Orion_ [2018]  - orionsoft.free.fr */
/* Version nettoyée par Samy Rabih [2025 ]*/
/* autre source : matthieu.weber.free.fr/electronique/cartes_a_puces/index.html */
/* Assignement des Broches
 Arduino <-       Telecarte      -> Arduino
 VCC 5v <-  Vcc   1       5 GND  -> GND
 Pin 2  <- Write  2       6 VPP  -> VCC 3.3v
 Pin 3  <- Clock  3       7 Read -> Pin 5
 Pin 4  <- Reset  4       8 N/C
*/



const int R_W = 2;
const int CLK = 3;
const int RAZ = 4;
const int E_S = 5;

byte data[64];
int total_units;

byte SumBits(byte b)  // Compte le nombre de bit dans l'octet 'b'
{
  int i, sum = 0;

  for (i = 0; i < 8; i++, b >>= 1)
    sum += (b & 1);
  return (sum);
}

void PrintHex0(byte b) {
  if (b < 0x10)
    Serial.print("0");
  Serial.print(b, HEX);
}

void PrintCardType(byte type) {
  String msg = "[T";
  msg += type;
  msg += "G] ";
  Serial.print(msg);
}

void PrintFreeUnits(int units) {
  String msg = "Unitees: ";
  msg += total_units - units;
  msg += "/";
  msg += total_units;
  Serial.println(msg);
}

void PrintSerial(byte *ptr, byte nserial, bool firstPrint, bool lastPrint) {
  byte i, shift = 4;

  if (firstPrint)
    Serial.print("Num.Serie: ");
  for (i = 0; i != nserial; i++, shift ^= 4)
    Serial.print((ptr[i >> 1] >> shift) & 0xF, HEX);
  if (lastPrint)
    Serial.println();
}

void PrintCode(byte *ptr, byte n, bool firstPrint) {
  byte i, b, shift = 4;

  if (firstPrint)
    Serial.print("Code:");
  for (i = 0; i != n; i++, shift ^= 4) {
    b = (ptr[i >> 1] >> shift) & 0xF;
    if (b == 0xA)
      break;
    Serial.print(b, HEX);
  }
  if (!firstPrint)
    Serial.println();
}

void setup() {
  int i;
  byte j, sum;
  byte *ptr;

  Serial.begin(9600);
  while (!Serial)
    ;

  Serial.println("Lecteur et Analyseur");
  Serial.println(" Telecarte Francaise");
  Serial.println("  V.4  Orion_ [2018]");
  Serial.println("Lecture Telecarte...");
  delay(3000);

  pinMode(R_W, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(RAZ, OUTPUT);
  pinMode(E_S, INPUT);

  for (i = 0; i < 64; i++)
    data[i] = 0;

  digitalWrite(CLK, 0);

  digitalWrite(R_W, 0);  // Reset Counter
  digitalWrite(RAZ, 0);
  digitalWrite(CLK, 1);
  digitalWrite(CLK, 0);

  digitalWrite(RAZ, 1);  // Counter++
  for (i = 0; i < 512; i++) {
    data[i >> 3] |= digitalRead(E_S) << (7 - (i & 7));
    digitalWrite(CLK, 1);
    digitalWrite(CLK, 0);
  }

  // Affiche les données de la télécarte si c'est une T1G (256bits) ou T2G (512bits)
  ptr = data;
again:
  Tclear();
  for (j = 0; j < 4; j++) {
    Serial.print("$");
    PrintHex0(ptr - &data[0]);
    Serial.print(": ");
    for (i = 0; i < 8; i++)
      PrintHex0(*ptr++);
    Serial.println();
  }
  delay(2000);
  if ((data[0] == 0x81) && (ptr != &data[64]))
    goto again;

  // Analyse de la Carte
  Tclear();
  if (data[0] == 0x81)  // T2G/T3G
  {
    if ((data[6] & 0x0F) == 0x06)  // T3G
    {
      PrintCardType(3);
      if (data[13] != 0x36)
        PrintSerial(&data[5], 3, true, true);
      PrintCode(&data[10], 4, true);
      PrintCode(&data[40], 16, false);
      Serial.println("Numero service:");
      for (i = 12; i < 16; i++)
        PrintHex0(data[i]);
      Serial.print("00 ");
      switch (data[13]) {
        case 0x36: Serial.println("FranceTel"); break;
        case 0x09: Serial.println("Intercall"); break;
        case 0x01: Serial.println("Kertel"); break;
        default: Serial.println("Inconnu"); break;
      }
    } else {
      PrintCardType(2);
      switch (data[6] & 0x0F) {
        case 0: Serial.println("Standard"); break;
        case 9: Serial.println("Eurostar"); break;
        default: Serial.println("Inconnu"); break;
      }

      switch (data[7] & 0x0F) {
        case 1: total_units = 5; break;
        case 3: total_units = 25; break;
        case 5: total_units = 50; break;
        case 6: total_units = 50 + 5; break;
        case 12: total_units = 120; break;
        default: total_units = 0; break;
      }

      // Unités Restantes
      sum = SumBits(data[8]) * 512;
      sum += SumBits(data[9]) * 64;
      sum += (SumBits(data[10]) - 1) * 8;
      sum += (SumBits(data[11]) - 1) * 1;

      PrintSerial(&data[2], 9, true, true);
      PrintFreeUnits(sum);

      Serial.print("Certificat: ");
      PrintHex0(data[14]);
      PrintHex0(data[15]);
      Serial.println();
    }
  } else  // T1G
  {
    PrintCardType(1);
    Serial.print("Checksum ");
    for (i = 0; i < 3; i++) {
      sum = 0xE3;
      Serial.print(i + 1);
      for (j = 1; j < 4; j++)
        sum -= (SumBits(data[(i * 4) + j]) * 4);
      if (data[i * 4] != sum) {
        Serial.println();
        Serial.println();
        Serial.println("Checksum incorrect !");
        Serial.println("Format inconnu.");
        return;
      }
    }

    Serial.println(" OK");
    switch (data[11]) {
      case 5: total_units = 40; break;
      case 6: total_units = 50; break;
      case 19: total_units = 120; break;
      default: total_units = 0; break;
    }

    // Unités Restantes
    for (sum = 0, i = 96 / 8; i < 248 / 8; i++)
      sum += SumBits(data[i]);

    PrintSerial(&data[1], 6, true, false);
    PrintSerial(&data[5], 4, false, true);
    PrintFreeUnits(sum - 10);

    Serial.print("Voltage VPP: ");
    Serial.println((data[10] & 0xF0) ? "21v" : "25v");
  }
}

void loop() {
}
