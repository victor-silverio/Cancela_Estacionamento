# Cancela_Estacionamento

# Cancela_Estacionamento - V4

Sistema de Controle para Cancela de Estacionamento  
Versão 4 – Detecção Avançada & Modo Manual

---

## 📋 Descrição

Esta versão do projeto implementa um sistema de controle de acesso veicular ainda mais robusto e seguro.  
Utiliza ESP32, servo motor, sensor ultrassônico, sensores digitais, LEDs, buzzer e botão físico.  
Inclui modo manual, lógica anti-fraude aprimorada, persistência do contador de veículos na EEPROM e sincronização de data/hora via NTP.

---

## 🚗 Funcionalidades

- **Emissão de ticket simulada** via Serial Monitor, com numeração sequencial e data/hora reais.
- **Abertura e fechamento automáticos da cancela** por servo motor e LEDs.
- **Detecção precisa de veículos** por sensor ultrassônico.
- **Modo manual** para abertura/fechamento da cancela via chave física.
- **Sincronização de data/hora** via NTP (ou data de compilação, caso offline).
- **Persistência do contador de veículos** na EEPROM.
- **Reset automático diário** do contador ao detectar mudança de data.
- **Timeout de segurança** para evitar cancela aberta indefinidamente.
- **Sistema anti-fraude:** impede passagem não autorizada ou tentativa de carona.
- **Aviso sonoro** antes do fechamento automático da cancela.
- **Feedback detalhado** via Serial Monitor.
- **Mensagens de status** periódicas.

---

## 🛠️ Hardware Utilizado

- ESP32
- 1x Servo Motor SG90
- 1x Sensor Ultrassônico (entrada)
- 1x Botão para emissão de ticket
- 1x Chave para modo manual
- 1x Sensor digital de saída
- 3x LEDs (Verde, Vermelho, Amarelo)
- 1x Buzzer

---

## ⚡ Mapeamento de Pinos

| Função                        | Pino ESP32 |
|-------------------------------|------------|
| Servo Motor                   | 22         |
| Botão Ticket                  | 15         |
| Chave Modo Manual             | 21         |
| Sensor Ultrassônico (Trig)    | 16         |
| Sensor Ultrassônico (Echo)    | 17         |
| Sensor Saída (digital)        | 18         |
| LED Verde (Cancela Aberta)    | 27         |
| LED Vermelho (Cancela Fechada)| 26         |
| LED Amarelo (Impressão Ticket)| 13         |
| Buzzer                        | 12         |

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
8. **Contador de veículos** é incrementado e salvo na EEPROM.
9. **Reset diário automático** do contador ao detectar mudança de data.
10. **Modo manual:** permite abrir/fechar a cancela via chave física.
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