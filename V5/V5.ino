/**
 * @file sistema_cancela_estacionamento.ino
 * @brief Sistema de controle de cancela de estacionamento para ESP32.
 * @author Contribuidores do Projeto Acadêmico (UNIFEI)
 * @date Julho de 2025
 *
 * @details
 * Este projeto, desenvolvido originalmente para fins acadêmicos, gerencia a
 * entrada de veículos em um estacionamento.
 *
 * Funcionalidades Principais:
 * - Abertura e fechamento da cancela via servo motor.
 * - Emissão de um ticket simulado (via Monitor Serial) com data, hora e
 * numeração sequencial.
 * - Sincronização de tempo via NTP para registros precisos.
 * - Indicadores visuais (LEDs) para status de cancela aberta, fechada e
 * impressão de ticket.
 * - Operação manual por meio de uma alavanca de controle.
 *
 * Mecanismos de Segurança:
 * - Timeout: A cancela fecha automaticamente se um veículo não passar pelos
 * sensores dentro de um tempo limite.
 * - Anti-Fraude: Impede que um segundo veículo passe aproveitando a cancela
 * aberta pelo veículo anterior.
 * - Contador Diário: A numeração de veículos é reiniciada a cada 24 horas.
 */

//==============================================================================
// INCLUSÃO DE BIBLIOTECAS
//==============================================================================
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include <Ultrasonic.h>
#include <time.h>

//==============================================================================
// CONFIGURAÇÕES DE REDE E TEMPO
//==============================================================================
// --- Credenciais de Rede ---
const char *SSID_REDE = "COLOQUE O SSID AQUI";     // Nome da sua rede Wi-Fi (SSID)
const char *SENHA_REDE = "COLOQUE A SENHA AQUI"; // Senha da sua rede Wi-Fi


// Cliente NTP para sincronização de tempo (UTC-3)
WiFiUDP ntpUDP;
NTPClient clienteNTP(ntpUDP, "pool.ntp.org", -3 * 3600);

//==============================================================================
// MAPEAMENTO DE HARDWARE (PINOS)
//==============================================================================
// --- Sensores e Atuadores ---
const byte pinoServoCancela = 21;
const byte pinoBotaoEmissaoTicket = 25;
const byte pinoAlavancaManual = 33;
const byte pinoBuzzer = 12;

// --- LEDs Indicadores ---
const byte pinoLedVerde = 27;     // Indica cancela aberta
const byte pinoLedVermelho = 26;  // Indica cancela fechada
const byte pinoLedAmarelo = 14;   // Indica impressão do ticket

// --- Pinos Sensor Ultrassônico 1 (Entrada) ---
const byte pinoTrigSensorEntrada = 16;
const byte pinoEchoSensorEntrada = 17;

// --- Pinos Sensor Ultrassônico 2 (Saída/Segurança) ---
const byte pinoTrigSensorSaida = 22;
const byte pinoEchoSensorSaida = 23;

//==============================================================================
// CONSTANTES DE OPERAÇÃO
//==============================================================================
// --- Parâmetros dos Sensores ---
const int DISTANCIA_DETECCAO_CM = 50; // Distância para considerar um veículo presente

// --- Parâmetros do Servo Motor (Cancela) ---
const int POSICAO_SERVO_ABERTA = 500;
const int POSICAO_SERVO_FECHADA = 1495;
const int FREQUENCIA_SERVO_HZ = 50;

// --- Intervalos de Tempo (em milissegundos) ---
const unsigned long INTERVALO_RESET_CONTADOR = 24UL * 60UL * 60UL * 1000UL; // 24 horas
const unsigned long TIMEOUT_PASSAGEM_VEICULO = 30000; // 30 segundos
const unsigned long DURACAO_SIMULACAO_IMPRESSAO = 3000;
const unsigned long ATRASO_FECHAMENTO_APOS_SAIDA = 2000; // 2 segundos
const unsigned long INTERVALO_MENSAGEM_STATUS = 5000;  // 5 segundos

// --- Parâmetros do Buzzer de Aviso ---
const int DURACAO_AVISO_SONORO_S = 5;
const unsigned long PERIODO_PISCA_BUZZER_MS = 500;

//==============================================================================
// OBJETOS E VARIÁVEIS GLOBAIS
//==============================================================================
Servo servoCancela;
Ultrasonic sensorEntrada(pinoTrigSensorEntrada, pinoEchoSensorEntrada);
Ultrasonic sensorSaida(pinoTrigSensorSaida, pinoEchoSensorSaida);

// --- Variáveis de Estado ---
bool cancelaAberta = false;
unsigned int contadorVeiculos = 0;

// --- Variáveis de Controle de Tempo ---
unsigned long tempoUltimoResetContador = 0;
unsigned long tempoUltimaMensagemStatus = 0;

//==============================================================================
// FUNÇÃO DE CONFIGURAÇÃO INICIAL (SETUP)
//==============================================================================
void setup() {
    Serial.begin(115200);

    // Configuração dos pinos
    pinMode(pinoBotaoEmissaoTicket, INPUT);
    pinMode(pinoAlavancaManual, INPUT);
    pinMode(pinoLedVerde, OUTPUT);
    pinMode(pinoLedVermelho, OUTPUT);
    pinMode(pinoLedAmarelo, OUTPUT);
    pinMode(pinoBuzzer, OUTPUT);

    // Conectar ao Wi-Fi
    Serial.print("\nConectando ao Wi-Fi ");
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Conectado!");

    // Iniciar cliente NTP
    clienteNTP.begin();

    // Configurar e inicializar o servo motor
    servoCancela.setPeriodHertz(FREQUENCIA_SERVO_HZ);
    servoCancela.attach(pinoServoCancela, POSICAO_SERVO_ABERTA, POSICAO_SERVO_FECHADA);
    
    // Inicia o sistema com a cancela fechada
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedVermelho, HIGH);
    servoCancela.writeMicroseconds(POSICAO_SERVO_FECHADA);
    cancelaAberta = false;
    
    Serial.println("\nSistema de Controle de Cancela Iniciado.");
    Serial.println("Aguardando veículos...");
}

//==============================================================================
// FUNÇÕES AUXILIARES
//==============================================================================

/**
 * @brief Obtém e formata a data e hora atuais do servidor NTP.
 * @param[out] data String para armazenar a data formatada (dd/mm/aaaa).
 * @param[out] hora String para armazenar a hora formatada (hh:mm:ss).
 */
void obterDataHora(String &data, String &hora) {
    clienteNTP.update();
    time_t tempoEpoch = clienteNTP.getEpochTime();
    struct tm* infoTempo = localtime(&tempoEpoch);

    char bufferData[11];
    char bufferHora[9];
    strftime(bufferData, sizeof(bufferData), "%d/%m/%Y", infoTempo);
    strftime(bufferHora, sizeof(bufferHora), "%H:%M:%S", infoTempo);

    data = String(bufferData);
    hora = String(bufferHora);
}

/**
 * @brief Fecha a cancela e atualiza os indicadores de status.
 */
void fecharCancela() {
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedVermelho, HIGH);
    servoCancela.writeMicroseconds(POSICAO_SERVO_FECHADA);
    cancelaAberta = false;
    Serial.println("-> Cancela fechada.\n");
}

/**
 * @brief Abre a cancela e atualiza os indicadores de status.
 */
void abrirCancela() {
    Serial.println("Abrindo cancela...");
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVermelho, LOW);
    digitalWrite(pinoLedVerde, HIGH);
    servoCancela.writeMicroseconds(POSICAO_SERVO_ABERTA);
    cancelaAberta = true;
    Serial.println("-> Cancela aberta.\n");
}

/**
 * @brief Ativa um aviso sonoro antes de fechar a cancela.
 */
void emitirAvisoSonoroEFechar() {
    Serial.println("AVISO: A cancela fechará em " + String(DURACAO_AVISO_SONORO_S) + " segundos.");
    for (int i = 0; i < DURACAO_AVISO_SONORO_S * 2; i++) {
        digitalWrite(pinoBuzzer, HIGH);
        delay(PERIODO_PISCA_BUZZER_MS / 2);
        digitalWrite(pinoBuzzer, LOW);
        delay(PERIODO_PISCA_BUZZER_MS / 2);
    }
    fecharCancela();
}

/**
 * @brief Simula a impressão de um ticket de entrada no Monitor Serial.
 */
void simularImpressaoTicket() {
    String dataAtual, horaAtual;
    obterDataHora(dataAtual, horaAtual);

    Serial.println("\nImprimindo ticket...");
    digitalWrite(pinoLedAmarelo, HIGH);
    delay(DURACAO_SIMULACAO_IMPRESSAO / 2);

    Serial.println("| ==================== |");
    Serial.println("|  Ticket de Entrada   |");
    Serial.println("| Data: " + dataAtual);
    Serial.println("| Hora: " + horaAtual);
    Serial.println("| Veículo Nº: " + String(contadorVeiculos + 1));
    Serial.println("| ==================== |\n");

    delay(DURACAO_SIMULACAO_IMPRESSAO / 2);
    digitalWrite(pinoLedAmarelo, LOW);
    Serial.println("Ticket impresso. Retire o ticket e avance.");
}

/**
 * @brief Monitora a passagem do veículo pela entrada, aplicando um timeout.
 * @return true se o veículo passou, false se o tempo expirou (timeout).
 */
bool monitorarPassagemEntrada() {
    Serial.println("Aguardando veículo passar pelo sensor de entrada...");
    unsigned long tempoInicio = millis();
    
    while (sensorEntrada.read() <= DISTANCIA_DETECCAO_CM) {
        if (millis() - tempoInicio > TIMEOUT_PASSAGEM_VEICULO) {
            Serial.println("ERRO: Tempo limite de passagem na entrada excedido!");
            emitirAvisoSonoroEFechar();
            return false; // Falha por timeout
        }
        delay(100); // Pequeno delay para não sobrecarregar o processador
    }
    Serial.println("Veículo liberou o sensor de entrada.");
    return true; // Sucesso
}

/**
 * @brief Monitora a saída do veículo e implementa a lógica anti-fraude.
 * @return true se a passagem foi concluída com sucesso, false caso contrário.
 */
bool monitorarSaidaEAntifraude() {
    Serial.println("Aguardando veículo passar pelo sensor de saída...");
    unsigned long tempoInicio = millis();

    // Espera o veículo chegar e sair da zona do sensor de saída
    while (sensorSaida.read() > DISTANCIA_DETECCAO_CM) {
         // Lógica Anti-Fraude: verifica se outro carro se aproxima da entrada
        if (sensorEntrada.read() <= DISTANCIA_DETECCAO_CM) {
            Serial.println("ALERTA: Segundo veículo detectado na entrada! Fechando cancela imediatamente.");
            fecharCancela();
            return false; // Falha por fraude
        }
        
        if (millis() - tempoInicio > TIMEOUT_PASSAGEM_VEICULO) {
            Serial.println("ERRO: Tempo limite de passagem na saída excedido!");
            emitirAvisoSonoroEFechar();
            return false; // Falha por timeout
        }
        delay(100);
    }

    Serial.println("Veículo detectado no sensor de saída. Passagem completa.");
    delay(ATRASO_FECHAMENTO_APOS_SAIDA);
    fecharCancela();
    return true; // Sucesso
}

/**
 * @brief Gerencia o processo completo de entrada de um veículo.
 */
void iniciarProcessoEntradaVeiculo() {
    Serial.println("\n--- INÍCIO DO PROCESSO DE ENTRADA ---");
    Serial.println("Botão pressionado por veículo na entrada.");

    simularImpressaoTicket();
    abrirCancela();

    if (monitorarPassagemEntrada()) {
        if (monitorarSaidaEAntifraude()) {
            contadorVeiculos++;
            Serial.println("\nProcesso concluído com sucesso. Veículo registrado.");
            Serial.println("Total de veículos hoje: " + String(contadorVeiculos));
        } else {
            Serial.println("\nFalha no processo (anti-fraude ou timeout na saída). Veículo não foi registrado.");
        }
    } else {
        Serial.println("\nFalha no processo (timeout na entrada). Veículo não foi registrado.");
    }
    
    Serial.println("\nSistema pronto para o próximo veículo.");
}

/**
 * @brief Controla a cancela manualmente através de uma alavanca.
 * A cancela permanece aberta enquanto a alavanca estiver ativada.
 */
void operarModoManual() {
    Serial.println("\n--- MODO MANUAL ATIVADO ---");
    abrirCancela();

    unsigned long tempoMsgManual = millis();
    while (digitalRead(pinoAlavancaManual) == HIGH) {
        if (millis() - tempoMsgManual > INTERVALO_MENSAGEM_STATUS) {
            Serial.println("A cancela está em modo de operação manual.");
            tempoMsgManual = millis();
        }
        delay(100);
    }

    Serial.println("--- MODO MANUAL DESATIVADO ---");
    fecharCancela();
}

/**
 * @brief Exibe mensagens de status periodicamente no Monitor Serial.
 * @param veiculoPresente True se um veículo está no sensor de entrada.
 * @param botaoPressionado True se o botão de ticket foi pressionado.
 */
void exibirStatusPeriodico(bool veiculoPresente, bool botaoPressionado) {
    if (millis() - tempoUltimaMensagemStatus > INTERVALO_MENSAGEM_STATUS) {
        if (veiculoPresente && !botaoPressionado) {
            Serial.println("Status: Veículo posicionado, aguardando acionamento do botão.");
        } else if (!veiculoPresente && !botaoPressionado) {
            Serial.println("Status: Sistema ocioso, aguardando veículo.");
        } else if (!veiculoPresente && botaoPressionado) {
             Serial.println("Status: Botão acionado sem veículo na posição.");
        }
        // Se veiculoPresente e botaoPressionado, a função principal já está tratando.
        tempoUltimaMensagemStatus = millis();
    }
}

//==============================================================================
// FUNÇÃO PRINCIPAL (LOOP)
//==============================================================================
void loop() {
    // Verifica se a alavanca de modo manual foi acionada
    if (digitalRead(pinoAlavancaManual) == HIGH) {
        operarModoManual();
        return; // Retorna para reiniciar o loop após o modo manual
    }

    // Verifica se o contador de veículos deve ser reiniciado (a cada 24h)
    if (millis() - tempoUltimoResetContador >= INTERVALO_RESET_CONTADOR) {
        contadorVeiculos = 0;
        tempoUltimoResetContador = millis();
        Serial.println("\nINFO: Contador de veículos foi reiniciado (ciclo de 24h).");
    }

    // Leitura dos sensores de entrada
    bool veiculoPresenteNaEntrada = (sensorEntrada.read() <= DISTANCIA_DETECCAO_CM);
    bool botaoPressionado = (digitalRead(pinoBotaoEmissaoTicket) == HIGH);

    // Lógica principal de operação
    if (veiculoPresenteNaEntrada && botaoPressionado && !cancelaAberta) {
        iniciarProcessoEntradaVeiculo();
    } else {
        exibirStatusPeriodico(veiculoPresenteNaEntrada, botaoPressionado);
    }
}