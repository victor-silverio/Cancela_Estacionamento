# Cancela_Estacionamento - V2

Sistema de Controle para Cancela de Estacionamento  
Versão 2 – Persistência & Data Real

---

## 📋 Descrição

Esta versão aprimorada do projeto implementa um sistema automatizado para controle de acesso a estacionamentos, utilizando ESP32, sensores infravermelhos, LEDs, buzzer e botão físico.  
Agora, conta com persistência do contador de veículos na EEPROM e sincronização de data/hora via NTP, garantindo registros precisos e reset automático diário.

---

## 🚗 Funcionalidades

- **Emissão de ticket simulada** via Serial Monitor, com numeração sequencial e data/hora reais.
- **Abertura e fechamento da cancela** (simulados por LEDs).
- **Sincronização de data/hora** via NTP (ou data de compilação, caso offline).
- **Persistência do contador de veículos** na EEPROM.
- **Reset automático diário** do contador ao detectar mudança de data.
- **Timeout de segurança** para evitar cancela aberta indefinidamente.
- **Sistema anti-fraude:** impede passagem não autorizada ou tentativa de carona.
- **Feedback detalhado** via Serial Monitor.

---

## 🛠️ Hardware Utilizado

- ESP32
- 1x Botão
- 2x Sensores Infravermelhos (entrada e saída)
- 3x LEDs (Verde, Vermelho, Feedback)
- 1x Buzzer

---

## ⚡ Mapeamento de Pinos

| Função                | Pino ESP32 |
|-----------------------|------------|
| Botão                 | 16         |
| Sensor Entrada        | 21         |
| Sensor Saída          | 18         |
| LED Verde (Cancela)   | 27         |
| LED Vermelho (Cancela)| 26         |
| LED Feedback          | 13         |
| Buzzer                | 12         |

---

## 🌐 Requisitos de Rede

- Rede Wi-Fi disponível (preencher SSID e senha no código).
- Acesso à internet para sincronização NTP (opcional, mas recomendado).

---

## ▶️ Como Funciona

1. **Veículo se posiciona** no sensor de entrada.
2. **Usuário pressiona o botão** para solicitar entrada.
3. **Ticket é "impresso"** (mensagem no Serial Monitor com data/hora real).
4. **Cancela abre** (LED verde aceso).
5. **Veículo passa pelo sensor de entrada e depois pelo de saída**.
6. **Sistema monitora tentativas de fraude** (carona ou passagem irregular).
7. **Cancela fecha** (LED vermelho aceso) após passagem.
8. **Contador de veículos** é incrementado e salvo na EEPROM.
9. **Reset diário automático** do contador ao detectar mudança de data.
10. **Mensagens de status** são exibidas periodicamente no Serial Monitor.

---

## 📄 Observações

- Caso a conexão Wi-Fi/NTP falhe, o sistema utiliza a data/hora de compilação.
- O sistema é totalmente simulado via Serial Monitor e LEDs.
- Para uso real, adapte os pinos e sensores conforme o hardware disponível.

---

## 👨‍💻 Autor

Desenvolvido para fins acadêmicos.  
Para dúvidas ou sugestões, entre em contato com o responsável pelo projeto.