/**
 * @file V1.ino
 * @brief Sistema de Controle de Cancela de Estacionamento.
 * 
 * Sistema de Controle para Cancela de Estacionamento
 *
 * Este projeto foi desenvolvido originalmente para fins acadêmicos. O código gerencia
 * o processo de entrada de um veículo em um estacionamento, controlando a cancela,
 * sensores e fornecendo feedback via monitor serial.
 *
 * Funcionalidades:
 * - Simulação de impressão de ticket de entrada.
 * - Controle de abertura e fechamento da cancela com sinalização por LEDs.
 * - Timeout de segurança para evitar que a cancela fique aberta indefinidamente.
 * - Sistema anti-carona (tailgating) para maior segurança.
 * - Contagem diária de veículos com reset automático.
 *
 * Hardware: ESP32, 1x Botão, 2x Sensores Infravermelhos, 3x LEDs, 1x Buzzer.
 */

// --- Mapeamento de Pinos ---
const int PINO_BOTAO = 25;
const int PINO_SENSOR_ENTRADA = 33;
const int PINO_SENSOR_SAIDA = 34;
const int PINO_LED_VERDE = 27;
const int PINO_LED_VERMELHO = 26;
const int PINO_LED_STATUS = 14;
const int PINO_BUZZER = 12;

// --- Constantes de Tempo (em milissegundos) ---
const unsigned long ATRASO_OPERACAO_CANCELA_MS = 500;
const unsigned long ATRASO_IMPRESSAO_TICKET_MS = 1500;
const unsigned long ATRASO_POS_SAIDA_MS = 2000;
const unsigned long TIMEOUT_ENTRADA_MS = 30000;
const unsigned long TIMEOUT_SAIDA_MS = 30000;
const unsigned long INTERVALO_STATUS_MS = 5000;
const unsigned long INTERVALO_RESET_CONTADOR_MS = 24UL * 60UL * 60UL * 1000UL; // 24 horas

// --- Constantes do Buzzer ---
const int DURACAO_ALERTA_SEGUNDOS = 5;
const unsigned long INTERVALO_BEEP_BUZZER_MS = 500;

// --- Variáveis Globais de Estado ---
unsigned int contagemVeiculos = 0;
unsigned long ultimoResetContador = 0;
unsigned long ultimoUpdateStatus = 0;
bool cancelaAberta = false;

/**
 * @brief Fecha a cancela, acende o LED vermelho e atualiza o estado do sistema.
 */
void fecharCancela() {
    digitalWrite(PINO_LED_STATUS, LOW);
    digitalWrite(PINO_LED_VERDE, LOW);
    digitalWrite(PINO_LED_VERMELHO, HIGH);
    cancelaAberta = false;
    Serial.println("STATUS: Cancela fechada.\n");
    delay(ATRASO_OPERACAO_CANCELA_MS);
}

/**
 * @brief Ativa um alarme sonoro por um período e, em seguida, fecha a cancela.
 * Utilizado em situações de timeout ou alertas de segurança.
 */
void acionarAlertaEFechar() {
    Serial.print("ALERTA: A cancela sera fechada em ");
    Serial.print(DURACAO_ALERTA_SEGUNDOS);
    Serial.println(" segundos.\n");

    for (int i = 0; i < DURACAO_ALERTA_SEGUNDOS; i++) {
        digitalWrite(PINO_BUZZER, HIGH);
        delay(INTERVALO_BEEP_BUZZER_MS / 2); // Bips mais curtos
        digitalWrite(PINO_BUZZER, LOW);
        delay(INTERVALO_BEEP_BUZZER_MS / 2);
    }
    fecharCancela();
}

/**
 * @brief Abre a cancela, acende o LED verde e atualiza o estado do sistema.
 */
void abrirCancela() {
    Serial.println("INFO: Abrindo a cancela...");
    digitalWrite(PINO_LED_STATUS, LOW);
    digitalWrite(PINO_LED_VERMELHO, LOW);
    digitalWrite(PINO_LED_VERDE, HIGH);
    cancelaAberta = true;
    Serial.println("STATUS: Cancela aberta.\n");
    delay(ATRASO_OPERACAO_CANCELA_MS);
}

/**
 * @brief Simula a impressão de um ticket de entrada no Monitor Serial.
 * O LED de status pisca durante a simulação.
 * @note  __DATE__ e __TIME__ são definidos no momento da compilação, não em tempo real.
 * Para tempo real, um módulo RTC (Real-Time Clock) seria necessário.
 */
void simularImpressaoTicket() {
    Serial.println("INFO: Imprimindo ticket...\n");
    digitalWrite(PINO_LED_STATUS, HIGH);
    delay(ATRASO_IMPRESSAO_TICKET_MS);

    Serial.println("| ======================== |");
    Serial.println("|    TICKET DE ENTRADA     |");
    Serial.println("| ======================== |");
    Serial.print("| Data: ");
    Serial.println(__DATE__); // Data da compilação do código
    Serial.print("| Hora: ");
    Serial.println(__TIME__); // Hora da compilação do código
    Serial.print("| Veiculo No: ");
    Serial.println(contagemVeiculos + 1);
    Serial.println("| ======================== |\n");

    delay(ATRASO_IMPRESSAO_TICKET_MS);
    digitalWrite(PINO_LED_STATUS, LOW);
    Serial.println("INFO: Ticket impresso.\n");
}

/**
 * @brief Gerencia a passagem do veículo pelo sensor de entrada.
 * @return Retorna 'true' se o veículo passar corretamente, 'false' se ocorrer timeout.
 */
bool gerenciarEntradaVeiculo() {
    unsigned long tempoInicioEntrada = millis();
    Serial.println("INFO: Aguardando veiculo passar pelo sensor de entrada...");

    while (digitalRead(PINO_SENSOR_ENTRADA)) {
        if (millis() - tempoInicioEntrada >= TIMEOUT_ENTRADA_MS) {
            Serial.println("ERRO: Timeout na entrada. Cancelando operacao.");
            acionarAlertaEFechar();
            return false;
        }
        delay(50); // Pequeno delay para evitar sobrecarga de loop
    }
    Serial.println("INFO: Veiculo liberou o sensor de entrada.\n");
    return true;
}

/**
 * @brief Gerencia a passagem do veículo pelo sensor de saída e verifica fraudes.
 * @return Retorna 'true' se o veículo sair corretamente, 'false' se houver timeout ou fraude.
 */
bool gerenciarSaidaVeiculo() {
    unsigned long tempoInicioSaida = millis();
    Serial.println("INFO: Aguardando veiculo passar pelo sensor de saida...");

    while (!digitalRead(PINO_SENSOR_SAIDA)) {
        // Checagem anti-carona: se outro carro chegar na entrada enquanto o primeiro sai
        if (digitalRead(PINO_SENSOR_ENTRADA)) {
            Serial.println("ALERTA: Segundo veiculo detectado na entrada! Fechamento imediato!");
            fecharCancela();
            return false;
        }

        if (millis() - tempoInicioSaida >= TIMEOUT_SAIDA_MS) {
            Serial.println("ERRO: Timeout na saida. Cancelando operacao.");
            acionarAlertaEFechar();
            return false;
        }
        delay(50);
    }

    Serial.println("INFO: Veiculo detectado no sensor de saida. Fechando em instantes.");
    delay(ATRASO_POS_SAIDA_MS);
    fecharCancela();
    return true;
}

/**
 * @brief Exibe mensagens de status no monitor serial quando o sistema está ocioso.
 */
void exibirStatusOcioso() {
    bool sensorEntradaAtivo = digitalRead(PINO_SENSOR_ENTRADA);
    bool botaoPressionado = digitalRead(PINO_BOTAO);

    if (!sensorEntradaAtivo && botaoPressionado) {
        Serial.println("AVISO: Botao pressionado sem veiculo no sensor.");
    } else if (sensorEntradaAtivo && !botaoPressionado) {
        Serial.println("STATUS: Veiculo detectado. Aguardando acionamento do botao...");
    } else {
        Serial.println("STATUS: Sistema ocioso. Aguardando veiculo.");
    }
    
    Serial.print("--> Status da Cancela: ");
    Serial.println(cancelaAberta ? "Aberta" : "Fechada");
    Serial.print("--> Veiculos hoje: ");
    Serial.println(contagemVeiculos);
    Serial.println();
    
    ultimoUpdateStatus = millis();
}

/**
 * @brief Função de configuração inicial do Arduino.
 */
void setup() {
    Serial.begin(115200);

    pinMode(PINO_BOTAO, INPUT);
    pinMode(PINO_SENSOR_ENTRADA, INPUT);
    pinMode(PINO_SENSOR_SAIDA, INPUT);
    pinMode(PINO_LED_VERDE, OUTPUT);
    pinMode(PINO_LED_VERMELHO, OUTPUT);
    pinMode(PINO_LED_STATUS, OUTPUT);
    pinMode(PINO_BUZZER, OUTPUT);

    ultimoResetContador = millis();
    fecharCancela(); // Garante que o estado inicial seja "cancela fechada"

    Serial.println("=================================================");
    Serial.println("Sistema de Controle de Estacionamento Iniciado");
    Serial.println("=================================================\n");
}

/**
 * @brief Loop principal do programa.
 */
void loop() {
    unsigned long tempoAtual = millis();

    // Verifica se é hora de resetar o contador diário de veículos
    if (tempoAtual - ultimoResetContador >= INTERVALO_RESET_CONTADOR_MS) {
        contagemVeiculos = 0;
        ultimoResetContador = tempoAtual;
        Serial.println("INFO: O contador de veiculos foi zerado para o novo dia.");
    }

    // Condição para iniciar o ciclo de entrada: Veículo posicionado E botão pressionado
    if (digitalRead(PINO_SENSOR_ENTRADA) && digitalRead(PINO_BOTAO)) {
        Serial.println("\n--- INICIO DA SEQUENCIA DE ENTRADA ---");
        
        simularImpressaoTicket();
        abrirCancela();

        bool entradaOK = gerenciarEntradaVeiculo();
        bool saidaOK = false;

        if (entradaOK) {
            saidaOK = gerenciarSaidaVeiculo();
        }

        if (entradaOK && saidaOK) {
            contagemVeiculos++;
            Serial.println("SUCESSO: Veiculo processado e registrado.");
            Serial.print("Total de veiculos hoje: ");
            Serial.println(contagemVeiculos);
        } else {
            Serial.println("FALHA: Operacao cancelada ou falhou. Veiculo nao registrado.");
        }
        Serial.println("--- FIM DA SEQUENCIA --- \n\nSistema pronto para o proximo veiculo.\n");
        
        // Evita mensagem de status imediata após uma operação
        ultimoUpdateStatus = millis(); 
    }
    // Lógica para exibir mensagens de status periodicamente se o sistema estiver ocioso
    else if (tempoAtual - ultimoUpdateStatus >= INTERVALO_STATUS_MS) {
        exibirStatusOcioso();
    }
}