/**
 * @file V2.ino
 * @brief Sistema de Controle de Cancela de Estacionamento.
 * * Este código implementa um sistema automatizado para controle de acesso a um 
 * estacionamento, utilizando um ESP32.
 * * Funcionalidades:
 * - Emissão de ticket simulada via Monitor Serial com numeração sequencial.
 * - Controle de abertura e fechamento de cancela com sinalização por LEDs.
 * - Sincronização de data e hora via NTP para registros precisos.
 * - Mecanismo de segurança com timeout para evitar que a cancela fique aberta.
 * - Sistema anti-fraude para impedir a passagem de veículos não autorizados.
 * - Contador de veículos com reset automático diário e persistência em memória (EEPROM).
 * * @note Este código foi desenvolvido como parte de um projeto acadêmico e
 * adaptado para uma versão profissional.
 */

// Bibliotecas necessárias
#include <NTPClient.h>      // Cliente NTP para sincronização de tempo
#include <WiFi.h>           // Gerenciamento de conexão Wi-Fi
#include <WiFiUdp.h>        // Protocolo UDP para a comunicação NTP
#include <time.h>           // Manipulação de funções de tempo padrão
#include <EEPROM.h>         // Acesso à memória não-volátil para persistência de dados

// --- Configurações de Persistência ---
#define TAMANHO_EEPROM  4   // Espaço necessário para armazenar um 'int' (4 bytes)
#define ENDERECO_CONTADOR 0 // Endereço na EEPROM para salvar o contador de veículos

// --- Credenciais de Rede ---
const char *SSID_REDE = "COLOQUE O SSID AQUI";     // Nome da sua rede Wi-Fi (SSID)
const char *SENHA_REDE = "COLOQUE A SENHA AQUI"; // Senha da sua rede Wi-Fi

// --- Configurações de Tempo e NTP ---
WiFiUDP ntpUDP;
// Cliente NTP configurado para o fuso horário de Brasília (UTC-3)
NTPClient clienteNtp(ntpUDP, "pool.ntp.org", -3 * 3600); 

// --- Variáveis de Estado e Controle de Tempo ---
String dataAtualFormatada;
String horaAtualFormatada;
String ultimaDataResetContador = "";
bool primeiraExecucao = true;

// --- Mapeamento de Pinos (Hardware) ---
const byte pinoBotaoEmissao    = 16; // Botão que o motorista pressiona para solicitar o ticket
const byte pinoSensorEntrada   = 21; // Sensor que detecta o veículo na entrada
const byte pinoSensorSaida     = 18; // Sensor que detecta o veículo na saída
const byte pinoLedVerde        = 27; // LED indicador de cancela aberta
const byte pinoLedVermelho     = 26; // LED indicador de cancela fechada
const byte pinoLedFeedback     = 13; // LED de feedback visual (ex: imprimindo ticket)
const byte pinoBuzzer          = 12; // Buzzer para alertas sonoros

// --- Constantes de Temporização (em milissegundos) ---
const unsigned long TIMEOUT_OPERACAO     = 30000; // Timeout geral para a passagem do veículo (30s)
const unsigned long DURACAO_IMPRESSAO    = 3000;  // Duração da simulação de impressão do ticket (3s)
const unsigned long ESPERA_PARA_FECHAR   = 2000;  // Tempo de espera antes de fechar a cancela após passagem (2s)
const unsigned long TEMPO_MOVIMENTO_CANCELA = 500;   // Duração da animação de abertura/fechamento (0,5s)
const unsigned long INTERVALO_MSG_STATUS = 5000;  // Intervalo entre mensagens de status no modo ocioso (5s)

// --- Parâmetros do Buzzer de Alerta ---
const int REPETICOES_AVISO_BUZZER = 5;      // Número de bips antes de um fechamento forçado
const unsigned long DURACAO_BIP_BUZZER = 500;   // Duração de cada bip (0,5s)

// --- Variáveis Globais de Controle ---
unsigned int contadorVeiculos = 0;
bool estadoCancelaAberta = false;
unsigned long timestampUltimaMsg = 0;

/**
 * @brief Atualiza as variáveis globais de data e hora.
 * Tenta sincronizar com o servidor NTP. Se falhar, usa data/hora de compilação.
 */
void sincronizarRelogio() {
    time_t tempoEpoch;
    bool sucessoNtp = false;

    if (WiFi.status() == WL_CONNECTED) {
        if (clienteNtp.update()) {
            tempoEpoch = clienteNtp.getEpochTime();
            sucessoNtp = true;
        }
    }

    if (!sucessoNtp) { // Fallback para data/hora de compilação se NTP falhar
        struct tm tm_comp;
        strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &tm_comp);
        tempoEpoch = mktime(&tm_comp);
    }

    struct tm *infoTempo = localtime(&tempoEpoch);
    char bufferData[11];
    char bufferHora[9];
    strftime(bufferData, sizeof(bufferData), "%d/%m/%Y", infoTempo);
    strftime(bufferHora, sizeof(bufferHora), "%H:%M:%S", infoTempo);
    
    dataAtualFormatada = String(bufferData);
    horaAtualFormatada = String(bufferHora);

    // Na primeira execução, define a data de referência para o reset diário
    if (primeiraExecucao) {
        ultimaDataResetContador = dataAtualFormatada;
        primeiraExecucao = false;
    }
}

/**
 * @brief Fecha a cancela e atualiza os indicadores visuais e de estado.
 */
void fecharCancela() {
    digitalWrite(pinoLedFeedback, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedVermelho, HIGH);
    Serial.println("-> Cancela fechada.\n");
    delay(TEMPO_MOVIMENTO_CANCELA); // Simula o tempo de movimento físico
    estadoCancelaAberta = false;
}

/**
 * @brief Emite um alerta sonoro antes de forçar o fechamento da cancela.
 */
void acionarAvisoSonoroFechamento() {
    Serial.println("AVISO: A cancela será fechada em " + String(REPETICOES_AVISO_BUZZER) + " segundos!\n");
    for (int i = 0; i < REPETICOES_AVISO_BUZZER; i++) {
        digitalWrite(pinoBuzzer, HIGH);
        delay(DURACAO_BIP_BUZZER);
        digitalWrite(pinoBuzzer, LOW);
        delay(DURACAO_BIP_BUZZER);
    }
    fecharCancela();
}

/**
 * @brief Simula a impressão do ticket, exibindo os detalhes no Monitor Serial.
 */
void emitirTicket() {
    sincronizarRelogio(); // Garante que a hora está atualizada para o ticket
    Serial.println("\nImprimindo ticket...");
    digitalWrite(pinoLedFeedback, HIGH);
    
    Serial.println("| ==================== |");
    Serial.println("|   Ticket de Entrada    |");
    Serial.println("| Data: " + dataAtualFormatada);
    Serial.println("| Hora: " + horaAtualFormatada);
    Serial.println("| Veículo N°: " + String(contadorVeiculos + 1)); // Mostra o próximo número
    Serial.println("| ==================== |\n");
    
    delay(DURACAO_IMPRESSAO); // Simula o tempo total de impressão
    
    digitalWrite(pinoLedFeedback, LOW);
    Serial.println("-> Ticket impresso. Retire o ticket e avance.");
}

/**
 * @brief Abre a cancela e atualiza os indicadores visuais e de estado.
 */
void abrirCancela() {
    Serial.println("-> Abrindo a cancela...");
    digitalWrite(pinoLedVermelho, LOW);
    digitalWrite(pinoLedVerde, HIGH);
    delay(TEMPO_MOVIMENTO_CANCELA); // Simula o tempo de movimento físico
    Serial.println("-> Cancela aberta.\n");
    estadoCancelaAberta = true;
}

/**
 * @brief Salva o valor atual do contador de veículos na EEPROM.
 */
void salvarContador() {
    EEPROM.put(ENDERECO_CONTADOR, contadorVeiculos);
    EEPROM.commit();
}

/**
 * @brief Função de configuração inicial do sistema.
 */
void setup() {
    Serial.begin(115200);
    EEPROM.begin(TAMANHO_EEPROM);

    // Carrega o último valor do contador salvo na EEPROM
    EEPROM.get(ENDERECO_CONTADOR, contadorVeiculos);

    // Configuração dos pinos
    pinMode(pinoBotaoEmissao, INPUT);
    pinMode(pinoSensorEntrada, INPUT);
    pinMode(pinoSensorSaida, INPUT);
    pinMode(pinoLedVerde, OUTPUT);
    pinMode(pinoLedVermelho, OUTPUT);
    pinMode(pinoLedFeedback, OUTPUT);
    pinMode(pinoBuzzer, OUTPUT);

    // Define o estado inicial dos atuadores
    digitalWrite(pinoLedFeedback, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedVermelho, HIGH); // Inicia com a cancela fechada

    Serial.println("\n--- Sistema de Controle de Cancela Iniciado ---");
    Serial.println("Contador de veículos carregado: " + String(contadorVeiculos));

    // Inicia a conexão com a rede Wi-Fi
    Serial.print("Conectando à rede Wi-Fi ");
    WiFi.begin(SSID_REDE, SENHA_REDE);
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) { // Timeout de ~10s
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado com sucesso!");
        Serial.print("Endereço IP: ");
        Serial.println(WiFi.localIP());
        clienteNtp.begin();
    } else {
        Serial.println("\nFalha ao conectar ao WiFi. Usando data/hora de compilação.");
    }
    
    sincronizarRelogio(); // Sincronização inicial
    Serial.println("Sistema pronto.\n");
}

/**
 * @brief Loop principal que gerencia o estado do sistema.
 */
void loop() {
    sincronizarRelogio();

    // --- Reset Diário do Contador ---
    if (dataAtualFormatada != ultimaDataResetContador) {
        Serial.println("\n--- DETECTADA MUDANÇA DE DIA ---");
        Serial.println("Resetando contador de veículos para o novo dia.");
        contadorVeiculos = 0;
        salvarContador();
        ultimaDataResetContador = dataAtualFormatada;
        Serial.println("Contador zerado. Sistema pronto.\n");
    }

    // --- Lógica de Operação da Cancela ---
    bool sensorEntradaAtivado = digitalRead(pinoSensorEntrada);
    bool botaoPressionado = digitalRead(pinoBotaoEmissao);

    // Condição de início: carro posicionado E botão pressionado E cancela fechada
    if (sensorEntradaAtivado && botaoPressionado && !estadoCancelaAberta) {
        emitirTicket();
        abrirCancela();

        unsigned long timestampInicioPassagem = millis();
        bool passouPelaEntrada = false;
        bool operacaoCancelada = false;

        // Loop não-bloqueante para gerenciar a passagem do veículo
        while (estadoCancelaAberta) {
            // 1. Verificação de Timeout
            if (millis() - timestampInicioPassagem > TIMEOUT_OPERACAO) {
                Serial.println("ERRO: Timeout! O veículo demorou demais para passar.");
                acionarAvisoSonoroFechamento(); // Fecha a cancela com aviso
                operacaoCancelada = true;
                break; // Sai do laço de gerenciamento
            }

            // 2. Lógica de Passagem
            // Se o veículo já liberou o sensor de entrada, aguarda ele acionar o de saída
            if (passouPelaEntrada) {
                // Sistema anti-fraude: se outro carro chegar na entrada enquanto o primeiro sai
                if (digitalRead(pinoSensorEntrada)) {
                    Serial.println("ALERTA: Tentativa de fraude detectada! Fechando a cancela.");
                    fecharCancela(); // Fechamento imediato
                    operacaoCancelada = true;
                    break;
                }
                // Passagem normal: veículo aciona o sensor de saída
                if (digitalRead(pinoSensorSaida)) {
                    Serial.println("Veículo passando pelo sensor de saída.");
                    delay(ESPERA_PARA_FECHAR); // Aguarda um instante para garantir a passagem completa
                    fecharCancela();
                    break; // Sucesso, sai do laço
                }
            } 
            // Se o veículo ainda está no sensor de entrada
            else if (digitalRead(pinoSensorEntrada) == LOW) {
                Serial.println("Veículo liberou o sensor de entrada. Aguardando saída...");
                passouPelaEntrada = true;
            }
            
            // Pequeno delay para não sobrecarregar o loop
            delay(50);
        }

        // 3. Atualização do Contador Pós-Operação
        if (!operacaoCancelada) {
            contadorVeiculos++;
            salvarContador();
            Serial.println("Operação bem-sucedida! Veículo N°" + String(contadorVeiculos) + " registrado.");
            Serial.println("Sistema pronto para o próximo veículo.\n");
        } else {
            Serial.println("Operação cancelada. O contador de veículos não foi incrementado.");
            Serial.println("Sistema pronto para o próximo veículo.\n");
        }
    }
    // --- Mensagens de Status em Modo Ocioso ---
    else if (!estadoCancelaAberta && (millis() - timestampUltimaMsg > INTERVALO_MSG_STATUS)) {
        if (sensorEntradaAtivado && !botaoPressionado) {
            Serial.println("Status: Veículo posicionado. Aguardando acionamento do botão.");
        } else if (!sensorEntradaAtivado && botaoPressionado) {
            Serial.println("Status: Botão acionado sem veículo na posição.");
        } else {
            Serial.println("Status: Sistema ocioso, aguardando veículo.");
        }
        timestampUltimaMsg = millis();
    }
}