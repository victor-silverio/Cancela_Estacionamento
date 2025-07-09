# Cancela_Estacionamento - V1

Sistema de Controle para Cancela de Estacionamento  
Versão 1 – Estrutura Básica

---

## 📋 Descrição

Projeto desenvolvido para gerenciar a entrada de veículos em um estacionamento, simulando abertura/fechamento de cancela, emissão de ticket e controle de fluxo.  
Esta versão utiliza ESP32, sensores infravermelhos, LEDs, buzzer e um botão físico.

---

## 🚗 Funcionalidades

- **Simulação de impressão de ticket de entrada** (via Serial Monitor)
- **Abertura e fechamento da cancela** (simulados por LEDs)
- **Timeout de segurança** para evitar cancela aberta indefinidamente
- **Sistema anti-carona (tailgating)**: fecha a cancela se outro veículo tentar entrar junto
- **Contagem diária de veículos** com reset automático a cada 24h (baseado em millis)
- **Feedback detalhado** via Serial Monitor

---

## 🛠️ Hardware Utilizado

- ESP32
- 1x Botão
- 2x Sensores Infravermelhos (entrada e saída)
- 3x LEDs (Verde, Vermelho, Status)
- 1x Buzzer

---

## ⚡ Mapeamento de Pinos

| Função                | Pino ESP32 |
|-----------------------|------------|
| Botão                 | 25         |
| Sensor Entrada        | 33         |
| Sensor Saída          | 34         |
| LED Verde (Cancela)   | 27         |
| LED Vermelho (Cancela)| 26         |
| LED Status            | 14         |
| Buzzer                | 12         |

---

## ▶️ Como Funciona

1. **Veículo se posiciona** no sensor de entrada.
2. **Usuário pressiona o botão** para solicitar entrada.
3. **Ticket é "impresso"** (mensagem no Serial Monitor).
4. **Cancela abre** (LED verde aceso).
5. **Veículo passa pelo sensor de entrada**.
6. **Sistema monitora saída** e verifica tentativas de fraude.
7. **Cancela fecha** (LED vermelho aceso) após passagem.
8. **Contador de veículos** é incrementado.
9. **Mensagens de status** são exibidas periodicamente no Serial Monitor.

---

## 📄 Observações

- O horário impresso no ticket é o da compilação do código (__DATE__ e __TIME__).
- Para data/hora em tempo real, seria necessário um módulo RTC ou integração NTP.
- O sistema é totalmente simulado via Serial Monitor e LEDs.

---

## 👨‍💻 Autor

Desenvolvido para fins acadêmicos.  
Para dúvidas ou sugestões, entre em contato com o responsável pelo