/**
 * @file V3.ino
 * @brief Sistema de Controle de Cancela de Estacionamento com ESP32.
 *
 * @details Este projeto foi desenvolvido para fins acadêmicos e demonstra o controle
 * de uma cancela de estacionamento. O sistema gerencia a entrada de veículos,
 * simula a impressão de um ticket com data, hora e número sequencial, e
 * implementa mecanismos de segurança como anti-fraude e timeout de operação.
 *
 * @note O código está configurado para ser compilado e executado na Arduino IDE.
 *
 * Hardware Utilizado:
 * - ESP32
 * - Servo Motor SG90
 * - Sensores de Presença (simulados por botões/chaves)
 * - LEDs (Verde, Vermelho, Amarelo)
 * - Botão para Emissão de Ticket
 * - Buzzer
 */

//======================================================================================
// Bibliotecas
//======================================================================================
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include <EEPROM.h>
#include <time.h>

//======================================================================================
// Definições de Pinos
//======================================================================================
const byte PINO_SERVO = 22;
const byte PINO_BOTAO_TICKET = 25;
const byte PINO_SENSOR_ENTRADA = 33;
const byte PINO_SENSOR_SAIDA = 34;
const byte PINO_SENSOR_RETIRADA_TICKET = 11;
const byte PINO_LED_VERDE = 27;     // Indica cancela aberta
const byte PINO_LED_VERMELHO = 26;  // Indica cancela fechada
const byte PINO_LED_AMARELO = 14;   // Indica impressão de ticket
const byte PINO_BUZZER = 12;

//======================================================================================
// Constantes de Configuração
//======================================================================================
// --- Rede Wi-Fi para Sincronização de Horário (NTP) ---
// --- Credenciais de Rede ---
const char *SSID_REDE = "COLOQUE O SSID AQUI";     // Nome da sua rede Wi-Fi (SSID)
const char *SENHA_REDE = "COLOQUE A SENHA AQUI"; // Senha da sua rede Wi-Fi

// --- Servo Motor ---
const int SERVO_POSICAO_ABERTA = 500;
const int SERVO_POSICAO_FECHADA = 1495;

// --- Temporizadores (em milissegundos) ---
const unsigned long TIMEOUT_OPERACAO = 30000;      // 30 segundos para timeout de passagem
const unsigned long DURACAO_IMPRESSAO = 3000;      // 3 segundos para simular impressão
const unsigned long INTERVALO_MENSAGEM_STATUS = 5000; // 5 segundos entre mensagens de status
const unsigned long DELAY_MOVIMENTO_CANCELA = 500; // 0.5 segundos de delay após mover a cancela
const unsigned long DELAY_POS_SAIDA = 2000;        // 2 segundos para fechar após carro sair

// --- Buzzer ---
const int BEEP_AVISO_TIMEOUT = 5;
const unsigned long DURACAO_BEEP = 200;

// --- EEPROM ---
#define EEPROM_SIZE 4       // Espaço para armazenar um unsigned int
#define ENDERECO_CONTADOR 0 // Endereço do contador na EEPROM

//======================================================================================
// Variáveis Globais
//======================================================================================
// --- Objetos ---
Servo servoCancela;
WiFiUDP ntpUDP;
NTPClient clienteNTP(ntpUDP, "pool.ntp.org", -3 * 3600); // UTC-3

// --- Controle de Estado ---
unsigned int contadorVeiculos = 0;
bool estadoCancelaAberta = false;
String ultimaDataReset = "";

// --- Controle de Tempo ---
unsigned long ultimoTempoStatus = 0;

//======================================================================================
// Protótipos das Funções
//======================================================================================
void conectarWiFi();
String obterDataFormatada();
String obterHoraFormatada();
void atualizarRelogio();
void fecharCancela();
void emitirAvisoFechamento();
void simularImpressaoTicket();
void abrirCancela();
bool monitorarPassagemEntrada();
bool monitorarPassagemSaida();
void aguardarRetiradaTicket();
void gerenciarStatusOcioso();
void verificarResetDiarioContador();

//======================================================================================
// Configuração Inicial (setup)
//======================================================================================
void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);

    // Configuração dos pinos
    pinMode(PINO_BOTAO_TICKET, INPUT);
    pinMode(PINO_SENSOR_ENTRADA, INPUT);
    pinMode(PINO_SENSOR_SAIDA, INPUT);
    pinMode(PINO_SENSOR_RETIRADA_TICKET, INPUT);
    pinMode(PINO_LED_VERDE, OUTPUT);
    pinMode(PINO_LED_VERMELHO, OUTPUT);
    pinMode(PINO_LED_AMARELO, OUTPUT);
    pinMode(PINO_BUZZER, OUTPUT);

    // Inicializa o servo e define a posição inicial
    servoCancela.setPeriodHertz(50);
    servoCancela.attach(PINO_SERVO, SERVO_POSICAO_ABERTA, SERVO_POSICAO_FECHADA);
    
    // Conecta ao Wi-Fi e sincroniza o tempo
    conectarWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        clienteNTP.begin();
    }
    atualizarRelogio(); // Primeira atualização para definir a data de reset

    // Carrega o contador de veículos da memória
    EEPROM.get(ENDERECO_CONTADOR, contadorVeiculos);

    // Garante que o estado inicial seja "fechado"
    fecharCancela();
    Serial.println("\n=============================================");
    Serial.println("Sistema de Controle de Cancela Iniciado.");
    Serial.println("=============================================\n");
}

//======================================================================================
// Loop Principal
//======================================================================================
void loop() {
    atualizarRelogio();
    verificarResetDiarioContador();

    bool sensorEntradaAtivado = digitalRead(PINO_SENSOR_ENTRADA);
    bool botaoTicketPressionado = digitalRead(PINO_BOTAO_TICKET);

    if (sensorEntradaAtivado && botaoTicketPressionado) {
        Serial.println("[INFO] Veículo detectado e botão pressionado. Iniciando operação.");
        
        simularImpressaoTicket();
        aguardarRetiradaTicket();
        abrirCancela();

        bool timeoutEntrada = monitorarPassagemEntrada();
        if (timeoutEntrada) {
            Serial.println("[AVISO] Operação cancelada por timeout na entrada.");
        } else {
            bool timeoutSaida = monitorarPassagemSaida();
            if (timeoutSaida) {
                Serial.println("[AVISO] Operação cancelada por timeout na saída.");
            } else {
                // Sucesso: incrementa o contador e salva na EEPROM
                contadorVeiculos++;
                EEPROM.put(ENDERECO_CONTADOR, contadorVeiculos);
                EEPROM.commit();
                Serial.println("[SUCESSO] Veículo registrado. Total do dia: " + String(contadorVeiculos));
                Serial.println("\nSistema pronto para o próximo veículo.");
            }
        }
    } else {
        gerenciarStatusOcioso();
    }
}

//======================================================================================
// Implementação das Funções
//======================================================================================

/**
 * @brief Tenta conectar à rede Wi-Fi com um timeout.
 */
void conectarWiFi() {
    Serial.print("Conectando ao Wi-Fi ");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long inicioTentativa = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - inicioTentativa < 15000) { // Timeout de 15s
        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
    } else {
        Serial.println("\nFalha ao conectar ao Wi-Fi. Usando tempo de compilação.");
    }
}

/**
 * @brief Obtém a data formatada (DD/MM/AAAA).
 * @return String com a data.
 */
String obterDataFormatada() {
    if (WiFi.status() != WL_CONNECTED) {
        return __DATE__; // Fallback para data de compilação
    }
    time_t tempoEpoch = clienteNTP.getEpochTime();
    struct tm *infoTempo = localtime(&tempoEpoch);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", infoTempo);
    return String(buffer);
}

/**
 * @brief Obtém a hora formatada (HH:MM:SS).
 * @return String com a hora.
 */
String obterHoraFormatada() {
    if (WiFi.status() != WL_CONNECTED) {
        return __TIME__; // Fallback para hora de compilação
    }
    return clienteNTP.getFormattedTime();
}

/**
 * @brief Atualiza o cliente NTP e define a data inicial para o reset diário.
 */
void atualizarRelogio() {
    if (WiFi.status() == WL_CONNECTED) {
        clienteNTP.update();
    }
    // Define a data para o primeiro reset diário, se ainda não foi definida
    if (ultimaDataReset == "") {
        ultimaDataReset = obterDataFormatada();
    }
}

/**
 * @brief Fecha a cancela, atualiza os LEDs e o estado do sistema.
 */
void fecharCancela() {
    digitalWrite(PINO_LED_AMARELO, LOW);
    digitalWrite(PINO_LED_VERDE, LOW);
    digitalWrite(PINO_LED_VERMELHO, HIGH);
    servoCancela.writeMicroseconds(SERVO_POSICAO_FECHADA);
    estadoCancelaAberta = false;
    Serial.println("[AÇÃO] Cancela Fechada.");
    delay(DELAY_MOVIMENTO_CANCELA);
}

/**
 * @brief Emite um aviso sonoro antes de fechar a cancela automaticamente.
 */
void emitirAvisoFechamento() {
    Serial.println("[ALERTA] A cancela será fechada em " + String(BEEP_AVISO_TIMEOUT) + " segundos!");
    for (int i = 0; i < BEEP_AVISO_TIMEOUT; i++) {
        digitalWrite(PINO_BUZZER, HIGH);
        delay(DURACAO_BEEP);
        digitalWrite(PINO_BUZZER, LOW);
        delay(1000 - DURACAO_BEEP); // Espera 1 segundo total por beep
    }
    fecharCancela();
}

/**
 * @brief Simula a impressão de um ticket, exibindo informações no monitor serial.
 */
void simularImpressaoTicket() {
    Serial.println("\nImprimindo ticket...");
    digitalWrite(PINO_LED_AMARELO, HIGH);
    delay(DURACAO_IMPRESSAO / 2); // Metade do tempo antes de mostrar o ticket

    Serial.println("| ==================== |");
    Serial.println("|   Ticket de Entrada  ");
    Serial.println("| Data: " + obterDataFormatada());
    Serial.println("| Hora: " + obterHoraFormatada());
    Serial.println("| Veículo N°: " + String(contadorVeiculos + 1)); // Mostra o próximo número
    Serial.println("| ==================== |\n");
    
    delay(DURACAO_IMPRESSAO / 2); // Restante do tempo
    digitalWrite(PINO_LED_AMARELO, LOW);
    Serial.println("Ticket impresso. Retire-o para liberar a passagem.");
}

/**
 * @brief Abre a cancela, atualiza os LEDs e o estado do sistema.
 */
void abrirCancela() {
    Serial.println("[AÇÃO] Abrindo a cancela...");
    digitalWrite(PINO_LED_VERMELHO, LOW);
    digitalWrite(PINO_LED_VERDE, HIGH);
    servoCancela.writeMicroseconds(SERVO_POSICAO_ABERTA);
    estadoCancelaAberta = true;
    Serial.println("[AÇÃO] Cancela Aberta. Prossiga.");
    delay(DELAY_MOVIMENTO_CANCELA);
}

/**
 * @brief Aguarda a retirada do ticket (sensor correspondente ser desativado).
 */
void aguardarRetiradaTicket() {
    unsigned long tempoMsg = millis();
    // Assume-se que o sensor em HIGH significa "ticket não retirado"
    while (digitalRead(PINO_SENSOR_RETIRADA_TICKET) == HIGH) {
        if (millis() - tempoMsg > INTERVALO_MENSAGEM_STATUS) {
            Serial.println("[AGUARDANDO] Por favor, retire o ticket.");
            tempoMsg = millis();
        }
        delay(10); // Pequeno delay para não sobrecarregar o loop
    }
    Serial.println("[INFO] Ticket retirado.");
}


/**
 * @brief Monitora a passagem do veículo pela entrada, com timeout.
 * @return Retorna 'true' se ocorreu timeout, 'false' caso contrário.
 */
bool monitorarPassagemEntrada() {
    unsigned long tempoInicio = millis();
    while (digitalRead(PINO_SENSOR_ENTRADA)) {
        if (millis() - tempoInicio > TIMEOUT_OPERACAO) {
            emitirAvisoFechamento();
            return true; // Timeout
        }
        delay(10);
    }
    Serial.println("[INFO] Veículo liberou o sensor de entrada.");
    return false; // Sucesso
}

/**
 * @brief Monitora a saída do veículo, implementando anti-fraude e timeout.
 * @return Retorna 'true' se ocorreu timeout, 'false' caso contrário.
 */
bool monitorarPassagemSaida() {
    unsigned long tempoInicio = millis();
    
    // Aguarda o veículo atingir o sensor de saída
    while (!digitalRead(PINO_SENSOR_SAIDA)) {
        // Mecanismo anti-fraude: se outro carro entrar, fecha imediatamente
        if (digitalRead(PINO_SENSOR_ENTRADA)) {
            Serial.println("[ALERTA] Fraude detectada! Veículo não autorizado tentou entrar. Fechando cancela.");
            fecharCancela();
            return true; // Considera como falha na operação
        }
        // Timeout: se o carro demorar muito para passar completamente
        if (millis() - tempoInicio > TIMEOUT_OPERACAO) {
            emitirAvisoFechamento();
            return true; // Timeout
        }
        delay(10);
    }

    // Veículo passou pelo sensor de saída
    Serial.println("[INFO] Veículo detectado no sensor de saída. Fechando em " + String(DELAY_POS_SAIDA / 1000) + "s.");
    delay(DELAY_POS_SAIDA);
    fecharCancela();
    return false; // Sucesso
}

/**
 * @brief Gerencia e exibe mensagens de status quando o sistema está ocioso.
 */
void gerenciarStatusOcioso() {
    if (millis() - ultimoTempoStatus > INTERVALO_MENSAGEM_STATUS) {
        bool sensorEntrada = digitalRead(PINO_SENSOR_ENTRADA);
        bool botaoPressionado = digitalRead(PINO_BOTAO_TICKET);
        String statusCancela = estadoCancelaAberta ? "Aberta" : "Fechada";

        if (sensorEntrada && !botaoPressionado) {
            Serial.println("[STATUS] Veículo posicionado. Aguardando acionamento do botão.");
        } else if (!sensorEntrada && botaoPressionado) {
            Serial.println("[STATUS] Botão acionado sem veículo na entrada.");
        } else {
            Serial.println("[STATUS] Sistema ocioso. Aguardando veículo...");
        }
        Serial.println("           (Status da Cancela: " + statusCancela + ")");
        ultimoTempoStatus = millis();
    }
}

/**
 * @brief Verifica se a data mudou para resetar o contador diário de veículos.
 */
void verificarResetDiarioContador() {
    String dataAtual = obterDataFormatada();
    if (dataAtual != ultimaDataReset && ultimaDataReset != "") {
        Serial.println("\n[INFO] Novo dia detectado! Resetando contador de veículos.");
        contadorVeiculos = 0;
        EEPROM.put(ENDERECO_CONTADOR, 0);
        EEPROM.commit();
        ultimaDataReset = dataAtual;
    }
}