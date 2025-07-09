# Bibliotecas Utilizadas no Projeto Cancela_Estacionamento

Este projeto utiliza diversas bibliotecas para garantir o funcionamento completo e robusto do sistema de controle de cancela de estacionamento.  
Abaixo estão listadas todas as bibliotecas necessárias, o motivo de uso de cada uma e em quais versões do código elas são utilizadas.

---

## 📚 Bibliotecas Externas

### 1. [NTPClient](https://github.com/arduino-libraries/NTPClient)
- **Autor:** Fabrice Weinberg  
- **Motivo:** Permite a sincronização de data e hora em tempo real via internet, fundamental para registrar corretamente os horários de entrada dos veículos e para o reset automático diário do contador.
- **Utilização:**  
  - **V2:** Sincronização de data/hora real.
  - **V3:** Sincronização de data/hora real.
  - **V4:** Sincronização de data/hora real.
  - **V5:** Sincronização de data/hora real.

---

### 2. [ESP32Servo](https://madhephaestus.github.io/ESP32Servo/annotated.html)
- **Autores:** Kevin Harrington e John K. Bennett  
- **Motivo:** Permite o controle preciso de servo motores com o ESP32, utilizado para abrir e fechar a cancela de forma automatizada.
- **Utilização:**  
  - **V3:** Controle da cancela por servo motor.
  - **V4:** Controle da cancela por servo motor.
  - **V5:** Controle da cancela por servo motor.

---

### 3. [Ultrasonic](https://github.com/ErickSimoes/Ultrasonic)
- **Autor:** Erick Simões  
- **Motivo:** Facilita a leitura de sensores ultrassônicos, essenciais para a detecção precisa de veículos na entrada e saída do estacionamento.
- **Utilização:**  
  - **V4:** Detecção de veículos por sensor ultrassônico.
  - **V5:** Dupla detecção de veículos (entrada e saída) por sensores ultrassônicos.

---

## 📦 Bibliotecas Padrão do Arduino/ESP32

Estas bibliotecas são mantidas pela equipe Arduino/ESP32 e já vêm integradas ao ambiente de desenvolvimento:

- **WiFi.h / WiFiUdp.h:**  
  - **Motivo:** Gerenciam a conexão Wi-Fi e comunicação UDP, necessárias para a sincronização NTP.
  - **Utilização:** V2, V3, V4, V5

- **EEPROM.h:**  
  - **Motivo:** Permite salvar dados de forma persistente, como o contador de veículos, mesmo após desligar o sistema.
  - **Utilização:** V2, V3, V4

- **time.h:**  
  - **Motivo:** Manipulação de datas e horários, usada em conjunto com o NTPClient.
  - **Utilização:** V2, V3, V4, V5

---

## 📑 Resumo de Utilização por Versão

| Biblioteca      | V1 | V2 | V3 | V4 | V5 |
|-----------------|----|----|----|----|----|
| NTPClient       |    | ✔️ | ✔️ | ✔️ | ✔️ |
| ESP32Servo      |    |    | ✔️ | ✔️ | ✔️ |
| Ultrasonic      |    |    |    | ✔️ | ✔️ |
| EEPROM          |    | ✔️ | ✔️ | ✔️ |    |
| WiFi / WiFiUdp  |    | ✔️ | ✔️ | ✔️ | ✔️ |
| time.h          |    | ✔️ | ✔️ | ✔️ | ✔️ |

---

## ℹ️ Observações

- As bibliotecas **NTPClient**, **ESP32Servo** e **Ultrasonic** devem ser instaladas manualmente via Gerenciador de Bibliotecas da Arduino IDE.
- As demais são padrão do ambiente ESP32/Arduino.
- Sempre confira a documentação oficial das bibliotecas para detalhes de instalação e uso.

---

## 👨‍💻 Créditos dos Autores das Bibliotecas

- **NTPClient:** Fabrice Weinberg – [GitHub](https://github.com/arduino-libraries/NTPClient)
- **ESP32Servo:** Kevin Harrington & John K. Bennett – [Documentação](https://madhephaestus.github.io/ESP32Servo/annotated.html)
- **Ultrasonic:** Erick Simões – [GitHub](https://github.com/ErickSimoes/Ultrasonic)

---