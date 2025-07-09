# Cancela_Estacionamento
Projeto desenvolvido durante as aulas de Introdução á Engenharia e ao Metódo Cientifico no 1° Semestre de 2025, na Universide Federal de Itajubá.

> _Repositório criado para fins de arquivo. Replique por sua conta e risco._

---

## 📑 Sumário

- [Versionamento](#versionamento)
- [Contato](#-contato)

---

## Versionamento

1. **V1 – Estrutura Básica**
    - Primeira versão funcional do projeto.
    - Abertura e fechamento da cancela (simulados por LEDs).
    - Geração de ticket de entrada com data e hora (utiliza o horário de compilação do código).
    - Feedback detalhado via Serial Monitor.
    - Simulação da impressão do ticket por LED específico.
    - Contador de carros reiniciado automaticamente a cada 24h (baseado em millis).
    - Implementação de lógica anti-fraude simples.

2. **V2 – Persistência & Data Real**
    - Adiciona armazenamento do contador de carros na EEPROM.
    - Integração com NTP para obter data e hora reais.
    - Reset do contador ocorre ao detectar mudança de data.
    - Impressão do ticket inclui data/hora real e número sequencial.
    - Feedback serial aprimorado e funções mais segmentadas.

3. **V3 – Automação & Modularização**
    - Controle da cancela por servo motor.
    - Aviso sonoro antes do fechamento automático.
    - Funções mais detalhadas e modularizadas.
    - Controle de fluxo aprimorado, incluindo espera pela retirada do ticket.
    - Estrutura do código mais organizada.

4. **V4 – Detecção Avançada & Modo Manual**
    - Inclusão de sensor ultrassônico para detecção precisa de veículos.
    - Adição de modo manual para abertura/fechamento da cancela.
    - Lógica anti-fraude baseada em leitura do ultrassônico.
    - Feedback serial ainda mais detalhado, incluindo status da cancela.
    - Loop principal mais robusto, com checagem de sensores e modo manual.

5. **V5 – Dupla Detecção & Robustez**
    - Utilização de dois sensores ultrassônicos (entrada e saída) para maior precisão.
    - Funções segmentadas para cada etapa do fluxo (entrada, saída, manual, etc).
    - Reset automático do contador a cada 24h (baseado em millis).
    - Mensagens detalhadas, incluindo distâncias lidas pelos sensores.
    - Lógica anti-fraude aprimorada, impedindo passagem consecutiva sem registro.
    - Estrutura modular, preparada para futuras expansões.

---

## Contato

Para mais informações, entre em contato:
* Victor Augusto; 
* **VictorAugusto4096@outlook.com**;
