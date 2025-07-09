# Cancela_Estacionamento - V3

Sistema de Controle para Cancela de Estacionamento  
Versão 3 – Automação & Modularização

---

## 📋 Descrição

Esta versão do projeto traz maior automação e modularização ao controle de acesso de veículos em estacionamentos.  
Utiliza ESP32, servo motor, sensores de presença, LEDs, buzzer e botão físico, além de integração com NTP para data/hora real e persistência do contador de veículos na EEPROM.

---

## 🚗 Funcionalidades

- **Emissão de ticket simulada** via Serial Monitor, com numeração sequencial e data/hora reais.
- **Abertura e fechamento automáticos da cancela** por servo motor e LEDs.
- **Sincronização de data/hora** via NTP (ou data de compilação, caso offline).
- **Persistência do contador de veículos** na EEPROM.
- **Reset automático diário** do contador ao detectar mudança de data.
- **Timeout de segurança** para evitar cancela aberta indefinidamente.
- **Sistema anti-fraude:** impede passagem não autorizada ou tentativa de carona.
- **Aviso sonoro** antes do fechamento automático da cancela.
- **Feedback detalhado** via Serial Monitor.
- **Estrutura modular** e funções segmentadas para cada etapa do fluxo.

---

## 🛠️ Hardware Utilizado

- ESP32
- 1x Servo Motor SG90
- 1x Botão para emissão de ticket
- 3x Sensores de presença (entrada, saída, retirada de ticket)
- 3x LEDs (Verde, Vermelho, Amarelo)
- 1x Buzzer

---

## ⚡ Mapeamento de Pinos

| Função                        | Pino ESP32 |
|-------------------------------|------------|
| Servo Motor                   | 22         |
| Botão Ticket                  | 25         |
| Sensor Entrada                | 33         |
| Sensor Saída                  | 34         |
| Sensor Retirada de Ticket     | 11         |
| LED Verde (Cancela Aberta)    | 27         |
| LED Vermelho (Cancela Fechada)| 26         |
| LED Amarelo (Impressão Ticket)| 14         |
| Buzzer                        | 12         |

---

## 🌐 Requisitos de Rede

- Rede Wi-Fi disponível (preencher SSID e senha no código).
- Acesso à internet para sincronização NTP (opcional, mas recomendado).

---

## ▶️ Como Funciona

1. **Veículo se posiciona** no sensor de entrada.
2. **Usuário pressiona o botão** para solicitar entrada.
3. **Ticket é "impresso"** (mensagem no Serial Monitor com data/hora real).
4. **Sistema aguarda a retirada do ticket** (sensor dedicado).
5. **Cancela abre** (servo motor e LED verde).
6. **Veículo passa pelo sensor de entrada e depois pelo de saída**.
7. **Sistema monitora tentativas de fraude** (carona ou passagem irregular).
8. **Cancela fecha** (servo motor e LED vermelho) após passagem.
9. **Contador de veículos** é incrementado e salvo na EEPROM.
10. **Reset diário automático** do contador ao detectar mudança de data.
11. **Mensagens de status** são exibidas periodicamente no Serial Monitor.

---

## 📄 Observações

- Caso a conexão Wi-Fi/NTP falhe, o sistema utiliza a data/hora de compilação.
- O sistema é totalmente simulado via Serial Monitor e LEDs.
- Para uso real, adapte os pinos e sensores conforme o hardware disponível.

---

## 👨‍💻 Autor

Desenvolvido para fins acadêmicos.  
Para dúvidas ou sugestões, entre em contato com o responsável pelo