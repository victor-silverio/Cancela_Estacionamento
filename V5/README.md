# Cancela_Estacionamento - V5

Sistema de Controle para Cancela de Estacionamento  
Versão 5 – Dupla Detecção & Robustez

---

## 📋 Descrição

Esta versão representa o estágio mais avançado do sistema de controle de cancela para estacionamentos.  
Utiliza ESP32, dois sensores ultrassônicos (entrada e saída), servo motor, LEDs, buzzer, botão físico e alavanca manual.  
Inclui lógica anti-fraude aprimorada, operação manual, contador diário automático e sincronização de data/hora via NTP.

---

## 🚗 Funcionalidades

- **Emissão de ticket simulada** via Serial Monitor, com numeração sequencial e data/hora reais.
- **Abertura e fechamento automáticos da cancela** por servo motor e LEDs.
- **Dupla detecção de veículos** por sensores ultrassônicos (entrada e saída).
- **Modo manual** para abertura/fechamento da cancela via alavanca.
- **Sincronização de data/hora** via NTP (ou data de compilação, caso offline).
- **Contador diário de veículos** com reset automático a cada 24h.
- **Timeout de segurança** para evitar cancela aberta indefinidamente.
- **Sistema anti-fraude:** impede passagem não autorizada ou tentativa de carona.
- **Aviso sonoro** antes do fechamento automático da cancela.
- **Feedback detalhado** via Serial Monitor.
- **Mensagens de status** periódicas.

---

## 🛠️ Hardware Utilizado

- ESP32
- 1x Servo Motor SG90
- 2x Sensores Ultrassônicos (entrada e saída)
- 1x Botão para emissão de ticket
- 1x Alavanca para modo manual
- 3x LEDs (Verde, Vermelho, Amarelo)
- 1x Buzzer

---

## ⚡ Mapeamento de Pinos

| Função                        | Pino ESP32 |
|-------------------------------|------------|
| Servo Motor                   | 21         |
| Botão Ticket                  | 25         |
| Alavanca Modo Manual          | 33         |
| Buzzer                        | 12         |
| LED Verde (Cancela Aberta)    | 27         |
| LED Vermelho (Cancela Fechada)| 26         |
| LED Amarelo (Impressão Ticket)| 14         |
| Sensor Ultrassônico Entrada (Trig) | 16    |
| Sensor Ultrassônico Entrada (Echo) | 17    |
| Sensor Ultrassônico Saída (Trig)   | 22    |
| Sensor Ultrassônico Saída (Echo)   | 23    |

---

## 🌐 Requisitos de Rede

- Rede Wi-Fi disponível (preencher SSID e senha no código).
- Acesso à internet para sincronização NTP (opcional, mas recomendado).

---

## ▶️ Como Funciona

1. **Veículo se posiciona** no sensor ultrassônico de entrada.
2. **Usuário pressiona o botão** para solicitar entrada.
3. **Ticket é "impresso"** (mensagem no Serial Monitor com data/hora real).
4. **Cancela abre** (servo motor e LED verde).
5. **Sistema monitora a passagem do veículo** pela entrada e saída, com timeout de segurança.
6. **Sistema anti-fraude:** fecha a cancela se outro veículo tentar entrar antes do anterior sair.
7. **Cancela fecha** (servo motor e LED vermelho) após passagem.
8. **Contador de veículos** é incrementado e reiniciado automaticamente a cada 24h.
9. **Modo manual:** permite abrir/fechar a cancela via alavanca física.
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