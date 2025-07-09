/**
 * @file V4.ino
 * @brief Sistema de controle automatizado para cancela de estacionamento.
 * @author Victor Augusto
 * @date Julho 2025
 *
 * @note Este código foi desenvolvido como parte de um projeto acadêmico e
 * adaptado para uma versão profissional. O sistema gerencia a entrada de
 * veículos, emissão de tickets, e possui mecanismos de segurança como
 * timeout e anti-fraude.
 */

// =============================================================================
// Bibliotecas
// =============================================================================
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include <Ultrasonic.h>
#include <EEPROM.h>
#include <time.h>

// =============================================================================
// Configurações de Rede e Tempo
// =============================================================================
// --- Credenciais de Rede ---
const char *SSID_REDE = "COLOQUE O SSID AQUI";     // Nome da sua rede Wi-Fi (SSID)
const char *SENHA_REDE = "COLOQUE A SENHA AQUI"; // Senha da sua rede Wi-Fi

WiFiUDP ntpUDP;
NTPClient clienteNTP(ntpUDP, "pool.ntp.org", -3 * 3600); // Fuso UTC-3

// =============================================================================
// Definições de Hardware e Pinos
// =============================================================================
// --- Pinos de Entrada ---
const byte pinoBotaoTicket      = 15;
const byte pinoChaveManual      = 21;
const byte pinoSensorSaida      = 18;
const byte pinoTrigSensorEntrada = 16;
const byte pinoEchoSensorEntrada = 17;

// --- Pinos de Saída ---
const byte pinoLedVerde         = 27; // Indica cancela aberta
const byte pinoLedVermelho      = 26; // Indica cancela fechada
const byte pinoLedAmarelo       = 13; // Indica emissão de ticket
const byte pinoBuzzer           = 12;
const byte pinoServo            = 22;

// =============================================================================
// Constantes do Sistema
// =============================================================================
// --- Servo Motor ---
const int SERVO_POSICAO_ABERTA   = 500;
const int SERVO_POSICAO_FECHADA  = 1495;

// --- Sensor Ultrassônico ---
const int DISTANCIA_PRESENCA_VEICULO_CM = 5;  // Distância para detectar um veículo na entrada
const int DISTANCIA_SAIDA_VEICULO_CM    = 15; // Distância para confirmar que o veículo saiu da entrada

// --- Intervalos de Tempo (em milissegundos) ---
const unsigned long TIMEOUT_OPERACAO     = 30000; // Timeout para entrada e saída (30s)
const unsigned long DURACAO_EMISSAO_TICKET = 1500;  // Duração da simulação de impressão (usado 2x = 3s)
const unsigned long ATRASO_FECHAMENTO    = 2000;  // Atraso para fechar após detecção na saída (2s)
const unsigned long DURACAO_MOV_CANCELA  = 500;   // Atraso para simular movimento da cancela (0.5s)
const unsigned long INTERVALO_MSG_STATUS = 2000;  // Intervalo para exibir mensagens de status (2s)

// --- Buzzer ---
const int QTD_AVISOS_SONOROS = 5;
const unsigned long DURACAO_BEEP_BUZZER = 500;

// --- EEPROM ---
#define EEPROM_TAMANHO 4   // Espaço para um inteiro (4 bytes)
#define ENDERECO_CONTADOR 0 // Endereço de memória para o contador de veículos

// =============================================================================
// Variáveis Globais de Controle
// =============================================================================
Servo servoCancela;
Ultrasonic sensorUltrassonico(pinoTrigSensorEntrada, pinoEchoSensorEntrada);

// --- Controle de Estado ---
bool estadoCancelaAberta = false;
unsigned int contadorVeiculosDia = 0;

// --- Controle de Tempo e Data ---
String dataAtual;
String horaAtual;
String ultimaDataResetContador = "";
unsigned long ultimoTempoMsgStatus = 0; // Temporizador para mensagens de status

// --- Flags de Operação ---
bool timeoutEntradaOcorrido = false;
bool timeoutSaidaOcorrido = false;
bool primeiraExecucao = true;

// =============================================================================
// Funções Auxiliares
// =============================================================================

/**
 * @brief Sincroniza as variáveis globais de data e hora.
 * Utiliza o cliente NTP se houver conexão Wi-Fi. Caso contrário,
 * utiliza a data e hora de compilação do código como fallback.
 */
void sincronizarDataHora() {
    time_t tempoEpoch;
    bool sucesso = false;

    if (WiFi.status() == WL_CONNECTED) {
        if (clienteNTP.update()) {
            tempoEpoch = clienteNTP.getEpochTime();
            sucesso = true;
        }
    }

    if (!sucesso) { // Fallback para tempo de compilação
        struct tm tm_compilacao = {0};
        strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm_compilacao);
        tempoEpoch = mktime(&tm_compilacao);
    }

    struct tm* infoTempo = localtime(&tempoEpoch);
    char bufferData[11];
    char bufferHora[9];
    strftime(bufferData, sizeof(bufferData), "%d/%m/%Y", infoTempo);
    strftime(bufferHora, sizeof(bufferHora), "%H:%M:%S", infoTempo);

    dataAtual = String(bufferData);
    horaAtual = String(bufferHora);

    // Na primeira execução, define a data do último reset
    if (primeiraExecucao) {
        ultimaDataResetContador = dataAtual;
        primeiraExecucao = false;
    }
}

/**
 * @brief Fecha a cancela, atualiza os LEDs e o estado do sistema.
 */
void fecharCancela() {
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedVermelho, HIGH);
    servoCancela.writeMicroseconds(SERVO_POSICAO_FECHADA);
    estadoCancelaAberta = false;
    Serial.println("-> Cancela fechada.\n");
    delay(DURACAO_MOV_CANCELA);
}

/**
 * @brief Emite um aviso sonoro por um período definido antes de fechar a cancela.
 */
void emitirAvisoSonoroFechamento() {
    Serial.println("AVISO: A cancela fechará em " + String(QTD_AVISOS_SONOROS) + " segundos!\n");
    for (int i = 0; i < QTD_AVISOS_SONOROS; i++) {
        digitalWrite(pinoBuzzer, HIGH);
        delay(DURACAO_BEEP_BUZZER / 2);
        digitalWrite(pinoBuzzer, LOW);
        delay(DURACAO_BEEP_BUZZER / 2);
    }
    fecharCancela();
}

/**
 * @brief Abre a cancela, atualiza os LEDs e o estado do sistema.
 */
void abrirCancela() {
    Serial.println("Abrindo cancela...");
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVermelho, LOW);
    digitalWrite(pinoLedVerde, HIGH);
    servoCancela.writeMicroseconds(SERVO_POSICAO_ABERTA);
    estadoCancelaAberta = true;
    Serial.println("-> Cancela aberta.\n");
    delay(DURACAO_MOV_CANCELA);
}

/**
 * @brief Simula a impressão de um ticket, exibindo os dados no monitor serial.
 */
void emitirTicket() {
    sincronizarDataHora();
    Serial.println("Emitindo ticket...");
    digitalWrite(pinoLedAmarelo, HIGH);
    delay(DURACAO_EMISSAO_TICKET);

    contadorVeiculosDia++;
    Serial.println("| ==================== |");
    Serial.println("|   TICKET DE ENTRADA  |");
    Serial.println("| Data: " + dataAtual);
    Serial.println("| Hora: " + horaAtual);
    Serial.println("| Veículo Nº: " + String(contadorVeiculosDia));
    Serial.println("| ==================== |\n");

    delay(DURACAO_EMISSAO_TICKET);
    digitalWrite(pinoLedAmarelo, LOW);
    Serial.println("-> Ticket emitido.\n");
}

/**
 * @brief Gerencia a passagem do veículo pela entrada, monitorando o tempo limite.
 */
void gerenciarSegurancaEntrada() {
    timeoutEntradaOcorrido = false;
    unsigned long tempoInicioEspera = millis();
    unsigned long ultimoTempoMsg = 0;

    // Aguarda o veículo sair da área do sensor de entrada
    while (sensorUltrassonico.read() <= DISTANCIA_PRESENCA_VEICULO_CM) {
        if (millis() - tempoInicioEspera > TIMEOUT_OPERACAO) {
            Serial.println("TIMEOUT: Veículo demorou para avançar. Cancelando operação.\n");
            emitirAvisoSonoroFechamento();
            timeoutEntradaOcorrido = true;
            return;
        }
        if (millis() - ultimoTempoMsg > INTERVALO_MSG_STATUS) {
            Serial.println("Aguardando veículo avançar...");
            ultimoTempoMsg = millis();
        }
        delay(10); // Pequeno delay para não sobrecarregar o loop
    }
    Serial.println("Veículo liberou a área de entrada.\n");
}

/**
 * @brief Gerencia a passagem do veículo pela saída, com segurança anti-fraude.
 */
void gerenciarSegurancaSaida() {
    // Se a operação já foi cancelada na entrada, não faz nada
    if (timeoutEntradaOcorrido) return;

    timeoutSaidaOcorrido = false;
    unsigned long tempoInicioEspera = millis();
    unsigned long ultimoTempoMsg = 0;

    // Aguarda o veículo ser detectado pelo sensor de saída
    while (digitalRead(pinoSensorSaida) == LOW) {
        if (millis() - tempoInicioEspera > TIMEOUT_OPERACAO) {
            Serial.println("TIMEOUT: Veículo não alcançou a saída. Cancelando operação.\n");
            emitirAvisoSonoroFechamento();
            timeoutSaidaOcorrido = true;
            return;
        }
        // Sistema Anti-Fraude: Se outro carro se posiciona na entrada enquanto o primeiro ainda não saiu
        if (sensorUltrassonico.read() <= DISTANCIA_PRESENCA_VEICULO_CM) {
            Serial.println("ANTI-FRAUDE: Segundo veículo detectado na entrada. Fechando imediatamente!\n");
            fecharCancela();
            timeoutSaidaOcorrido = true; // Considera como falha para não contar o carro
            return;
        }
        if (millis() - ultimoTempoMsg > INTERVALO_MSG_STATUS) {
            Serial.println("Aguardando veículo passar pelo sensor de saída...");
            ultimoTempoMsg = millis();
        }
        delay(10);
    }
    
    // Se chegou aqui, o carro foi detectado na saída
    Serial.println("Veículo detectado na saída. Fechando a cancela em " + String(ATRASO_FECHAMENTO / 1000) + " segundos.\n");
    delay(ATRASO_FECHAMENTO);
    fecharCancela();
}

/**
 * @brief Ativa o modo de controle manual da cancela através de uma chave.
 */
void gerenciarModoManual() {
    bool jaAbriu = false;
    while (digitalRead(pinoChaveManual) == HIGH) {
        if (!jaAbriu) {
            Serial.println("MODO MANUAL ATIVADO. Abrindo cancela.");
            abrirCancela();
            jaAbriu = true;
        }
        if (millis() - ultimoTempoMsgStatus > INTERVALO_MSG_STATUS) {
            Serial.println("Sistema em modo de operação manual...");
            ultimoTempoMsgStatus = millis();
        }
        delay(10);
    }
    // Ao sair do loop (chave desativada)
    Serial.println("MODO MANUAL DESATIVADO. Fechando cancela.");
    fecharCancela();
}


// =============================================================================
// Função de Configuração Inicial (Setup)
// =============================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\nIniciando Sistema de Controle de Cancela...");

    // --- Inicialização da EEPROM ---
    EEPROM.begin(EEPROM_TAMANHO);
    EEPROM.get(ENDERECO_CONTADOR, contadorVehiclesToday); // Recupera o valor salvo

    // --- Configuração dos Pinos ---
    pinMode(pinoBotaoTicket, INPUT);
    pinMode(pinoChaveManual, INPUT);
    pinMode(pinoSensorSaida, INPUT);
    pinMode(pinoLedVerde, OUTPUT);
    pinMode(pinoLedVermelho, OUTPUT);
    pinMode(pinoLedAmarelo, OUTPUT);
    pinMode(pinoBuzzer, OUTPUT);

    // --- Conexão Wi-Fi ---
    Serial.print("Conectando ao Wi-Fi ");
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado com sucesso!");
        clienteNTP.begin();
    } else {
        Serial.println("\nFalha ao conectar. Usando tempo de compilação.");
    }
    
    sincronizarDataHora(); // Primeira sincronização de hora

    // --- Configuração do Servo Motor ---
    servoCancela.setPeriodHertz(50);
    servoCancela.attach(pinoServo, SERVO_POSICAO_ABERTA, SERVO_POSICAO_FECHADA);
    
    // --- Estado Inicial do Sistema ---
    Serial.println("Sistema iniciado. Cancela em posição inicial (fechada).");
    fecharCancela();
}

// =============================================================================
// Loop Principal de Execução
// =============================================================================
void loop() {
    sincronizarDataHora();

    // --- Verificação de Reset Diário do Contador ---
    if (dataAtual != ultimaDataResetContador) {
        Serial.println("\nNovo dia detectado! Resetando contador de veículos.");
        contadorVeiculosDia = 0;
        EEPROM.put(ENDERECO_CONTADOR, 0);
        EEPROM.commit();
        ultimaDataResetContador = dataAtual;
    }

    // --- Verificação do Modo Manual ---
    if (digitalRead(pinoChaveManual) == HIGH) {
        gerenciarModoManual();
        return; // Retorna para reiniciar o loop após o modo manual
    }

    bool veiculoPresente = (sensorUltrassonico.read() <= DISTANCIA_PRESENCA_VEICULO_CM);
    bool botaoPressionado = (digitalRead(pinoBotaoTicket) == HIGH);

    // --- Fluxo Principal de Entrada de Veículo ---
    if (veiculoPresente && botaoPressionado) {
        Serial.println("Veículo na posição e botão pressionado. Iniciando processo de entrada.");
        
        emitirTicket();
        abrirCancela();
        gerenciarSegurancaEntrada();
        gerenciarSegurancaSaida();

        // Se a operação foi cancelada por timeout, reverte o contador
        if (timeoutEntradaOcorrido || timeoutSaidaOcorrido) {
            Serial.println("Operação cancelada por timeout. O veículo não será registrado.");
            if (contadorVeiculosDia > 0) {
                contadorVeiculosDia--;
            }
        } else {
            // Salva o novo total de veículos na EEPROM
            EEPROM.put(ENDERECO_CONTADOR, contadorVeiculosDia);
            EEPROM.commit();
            Serial.println("Veículo registrado com sucesso.");
            Serial.println("Sistema pronto para o próximo veículo.\n");
        }
        // Reseta as flags de timeout para o próximo ciclo
        timeoutEntradaOcorrido = false;
        timeoutSaidaOcorrido = false;
    }
    // --- Lógica para Mensagens de Status (não bloqueante) ---
    else if (millis() - ultimoTempoMsgStatus > INTERVALO_MSG_STATUS) {
        if (botaoPressionado && !veiculoPresente) {
            Serial.println("Status: Botão pressionado sem veículo na posição.");
        } else if (veiculoPresente && !botaoPressionado) {
            Serial.println("Status: Veículo posicionado, aguardando acionamento do botão.");
        } else {
            Serial.println("Status: Sistema ocioso, aguardando veículos.");
        }
        Serial.println("Cancela: " + String(estadoCancelaAberta ? "Aberta" : "Fechada") + " | Veículos Hoje: " + String(contadorVeiculosDia) + "\n");
        ultimoTempoMsgStatus = millis();
    }
}