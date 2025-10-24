/*
 * CardioIA - Sistema de Monitoramento Cardíaco IoT
 * 
 * Projeto: Cap 1 - CardioIA Conectada: IoT e Visualização de Dados para a Saúde Digital
 * Instituição: FIAP - Inteligência Artificial 2024
 * Plataforma: ESP32 (Wokwi Simulator)
 * 
 * DESCRIÇÃO:
 * Sistema de monitoramento de sinais vitais com Edge Computing e Cloud Computing.
 * 
 * SENSORES:
 * - DHT22: Temperatura e umidade (obrigatório)
 * - MPU6050: Acelerômetro para detecção de movimento e BPM
 * 
 * FUNCIONALIDADES:
 * - Buffer RAM (100 amostras) para resiliência offline
 * - Transmissão MQTT para cloud
 * - Sincronização automática ao reconectar
 * - Alertas para valores críticos
 * - Controles via Serial Monitor (bpm=XX, auto, wifi on/off, status, help)
 * - Botão físico para toggle WiFi
 * 
 * AUTORES:
 *  - RM559645 - Edimilson Ribeiro da Silva
 *  - RM560173 - Gabriel Ribeiro
 *  - RM560461 - Jonas Felipe dos Santos Lima
 *  - RM559926 - Marcos Trazzini
 * 
 * WOKWI: https://wokwi.com/projects/445648982526430209
 * GITHUB: https://github.com/FIAP-IA2024/CARDIO_IOT_edge
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ========================================================================
// CONFIGURAÇÕES DE REDE E MQTT
// ========================================================================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Configurações do broker MQTT (HiveMQ Cloud)
// NOTA: Wokwi não suporta TLS/SSL. Para testes no Wokwi, usar broker público sem TLS.
// Para produção no ESP32 físico, use porta 8883 com TLS.

// Opção 1: HiveMQ Cloud com credenciais (requer TLS - não funciona no Wokwi)
// const char* MQTT_BROKER = "fb72a7d252f04540981122bed00f24ae.s1.eu.hivemq.cloud";
// const int MQTT_PORT = 8883;
// const char* MQTT_USER = "iatron";
// const char* MQTT_PASSWORD = "Iatron2025";

// Opção 2: Broker público HiveMQ (sem TLS - funciona no Wokwi)
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASSWORD = "";

const char* MQTT_CLIENT_ID = "CardioIA_ESP32_001";
const char* MQTT_TOPIC_DATA = "cardioIA/vitals/data";
const char* MQTT_TOPIC_ALERT = "cardioIA/vitals/alert";

// ========================================================================
// CONFIGURAÇÕES DE HARDWARE
// ========================================================================
#define DHT_PIN 15           // Pino do sensor DHT22
#define DHT_TYPE DHT22       // Tipo do sensor
#define MPU6050_ADDR 0x68    // Endereço I2C do MPU6050
#define WIFI_BUTTON_PIN 25   // Botão para conectar/desconectar WiFi
#define LED_STATUS_PIN 2     // LED de status (built-in)
#define LED_ALERT_PIN 4      // LED de alerta

// ========================================================================
// PARÂMETROS DO SISTEMA
// ========================================================================
const unsigned long SENSOR_READ_INTERVAL = 5000;  // Intervalo de leitura: 5 segundos
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;  // Tentativa reconexão MQTT: 5s
const int MAX_OFFLINE_SAMPLES = 100;  // Máximo de amostras em buffer RAM

// Limites para alertas (valores críticos)
const float TEMP_MAX_ALERT = 38.0;    // Temperatura máxima: 38°C
const float TEMP_MIN_ALERT = 35.0;    // Temperatura mínima: 35°C
const int BPM_MAX_ALERT = 120;        // BPM máximo: 120
const int BPM_MIN_ALERT = 50;         // BPM mínimo: 50
const float HUMIDITY_MAX_ALERT = 80.0;  // Umidade máxima: 80%
const float MOVEMENT_THRESHOLD = 1.5;  // Limite de movimento para detecção de atividade

// ========================================================================
// OBJETOS GLOBAIS
// ========================================================================
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ========================================================================
// VARIÁVEIS DE ESTADO
// ========================================================================
bool wifiEnabled = true;      // WiFi habilitado (pode ser controlado)
bool wifiConnected = false;
bool mqttConnected = false;

unsigned long lastSensorRead = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastMovement = 0;
int currentBPM = 70;  // BPM inicial
int manualBPM = -1;   // BPM definido manualmente (-1 = automático)

// Dados dos sensores
float temperature = 37.0;  // Temperatura inicial
float humidity = 65.0;     // Umidade inicial
float accelX = 0.0, accelY = 0.0, accelZ = 0.0;
float movementIntensity = 0.0;
unsigned long sampleTimestamp = 0;

// Modo de teste automático (simula aumento/diminuição gradual)
bool autoTestMode = true;     // Modo teste ativo por padrão
bool increasingValues = true; // true = subindo, false = descendo
float tempTarget = TEMP_MAX_ALERT + 0.5;  // Alvo: 38.5°C (acima do limite)
float humidTarget = HUMIDITY_MAX_ALERT + 5.0;  // Alvo: 85% (acima do limite)
int bpmTarget = BPM_MAX_ALERT + 5;  // Alvo: 125 bpm (acima do limite)

// Controle do botão WiFi
bool lastWifiButtonState = HIGH;
unsigned long lastWifiButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Buffer para comandos Serial
String serialBuffer = "";

// Buffer RAM para dados offline (quando SD Card não disponível)
String offlineDataBuffer[100];  // Buffer para 100 amostras em RAM
int bufferWriteIndex = 0;
int bufferReadIndex = 0;
int bufferCount = 0;

// ========================================================================
// PROTÓTIPOS DE FUNÇÕES
// ========================================================================
void setupWiFi();
void setupMQTT();
void setupSensors();
void setupMPU6050();
void reconnectWiFi();
void reconnectMQTT();
void readSensors();
void readMPU6050();
void calculateBPMFromMovement();
void checkWiFiButton();
void processSerialCommands();
void showHelp();
void saveDataToRAM(String data);
void syncOfflineData();
void sendDataToMQTT(String jsonData);
void checkAlerts();
String createJsonData();
void blinkLED(int pin, int times, int delayMs);

// ========================================================================
// SETUP - INICIALIZAÇÃO DO SISTEMA
// ========================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("  CardioIA - Sistema de Monitoramento");
  Serial.println("  Edge Computing + IoT Cloud");
  Serial.println("========================================\n");

  // Configuração dos pinos
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);
  pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP);
  
  // Teste inicial dos LEDs
  blinkLED(LED_STATUS_PIN, 3, 200);
  
  // Inicializar subsistemas
  setupSensors();
  setupMPU6050();
  setupWiFi();
  setupMQTT();
  
  Serial.println("\n[SISTEMA] Inicialização completa!");
  Serial.println("[SISTEMA] Modo de operação: Edge Computing ativo");
  Serial.println("\n[INFO] COMANDOS DISPONÍVEIS:");
  Serial.println("  - Digite 'bpm=XX' para definir BPM manualmente (ex: bpm=75)");
  Serial.println("  - Digite 'auto' para voltar ao modo automático");
  Serial.println("  - Digite 'help' para ver todos os comandos");
  Serial.println("  - Pressione o botão azul para conectar/desconectar WiFi");
  Serial.println("========================================\n");
}

// ========================================================================
// LOOP PRINCIPAL
// ========================================================================
void loop() {
  unsigned long currentMillis = millis();
  
  // Verificar botão WiFi
  checkWiFiButton();
  
  // Processar comandos Serial
  processSerialCommands();
  
  // Verificar conexão WiFi (apenas se habilitado)
  if (wifiEnabled) {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    // Reconectar MQTT se necessário
    if (wifiConnected && !mqttClient.connected()) {
      if (currentMillis - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
        reconnectMQTT();
        lastMqttReconnect = currentMillis;
      }
    }
    
    // Manter conexão MQTT
    if (mqttClient.connected()) {
      mqttClient.loop();
      mqttConnected = true;
      digitalWrite(LED_STATUS_PIN, HIGH);
    } else {
      mqttConnected = false;
      digitalWrite(LED_STATUS_PIN, LOW);
    }
  } else {
    wifiConnected = false;
    mqttConnected = false;
    digitalWrite(LED_STATUS_PIN, LOW);
  }
  
  // Ler sensor de movimento
  readMPU6050();
  
  // Calcular BPM baseado em movimento (se não for manual)
  if (manualBPM == -1) {
    calculateBPMFromMovement();
  } else {
    currentBPM = manualBPM;
  }
  
  // Leitura periódica dos sensores
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    
    readSensors();
    String jsonData = createJsonData();
    
    // Verificar alertas
    checkAlerts();
    
    // Decidir armazenar ou enviar
    if (mqttConnected) {
      Serial.println("[ONLINE] Enviando dados via MQTT...");
      sendDataToMQTT(jsonData);
      
      // Sincronizar dados offline pendentes (buffer RAM)
      if (bufferCount > 0) {
        Serial.println("[SYNC] Sincronizando dados offline do buffer RAM...");
        syncOfflineData();
      }
    } else {
      Serial.println("[OFFLINE] WiFi/MQTT desconectado. Armazenando em RAM...");
      saveDataToRAM(jsonData);
    }
    
    // Exibir resumo
    Serial.println("\n--- Resumo da Leitura ---");
    Serial.printf("Temperatura: %.1f °C\n", temperature);
    Serial.printf("Umidade: %.1f %%\n", humidity);
    Serial.printf("BPM: %d %s\n", currentBPM, manualBPM != -1 ? "(MANUAL)" : "(AUTO)");
    Serial.printf("Movimento: %.2f g\n", movementIntensity);
    Serial.printf("WiFi: %s\n", wifiEnabled ? (wifiConnected ? "CONECTADO" : "DESCONECTADO") : "DESABILITADO");
    Serial.printf("Status: %s\n", mqttConnected ? "ONLINE" : "OFFLINE");
    Serial.printf("Buffer RAM: %d/%d amostras\n", bufferCount, MAX_OFFLINE_SAMPLES);
    Serial.println("-------------------------\n");
  }
  
  delay(10);  // Small delay to prevent WDT reset
}

// ========================================================================
// VERIFICAR BOTÃO WiFi
// ========================================================================
void checkWiFiButton() {
  bool buttonState = digitalRead(WIFI_BUTTON_PIN);
  unsigned long currentTime = millis();
  
  // Detectar borda de descida com debounce
  if (buttonState == LOW && lastWifiButtonState == HIGH) {
    if (currentTime - lastWifiButtonPress > DEBOUNCE_DELAY) {
      wifiEnabled = !wifiEnabled;
      
      if (wifiEnabled) {
        Serial.println("\n[BOTÃO] WiFi HABILITADO - Tentando conectar...");
        setupWiFi();
      } else {
        Serial.println("\n[BOTÃO] WiFi DESABILITADO - Modo offline forçado");
        WiFi.disconnect(true);
        wifiConnected = false;
        mqttConnected = false;
      }
      
      lastWifiButtonPress = currentTime;
      blinkLED(LED_STATUS_PIN, 3, 100);
    }
  }
  
  lastWifiButtonState = buttonState;
}

// ========================================================================
// PROCESSAR COMANDOS SERIAL
// ========================================================================
void processSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        serialBuffer.trim();
        serialBuffer.toLowerCase();
        
        // Comando: definir BPM manualmente
        if (serialBuffer.startsWith("bpm=")) {
          int newBPM = serialBuffer.substring(4).toInt();
          if (newBPM >= 30 && newBPM <= 200) {
            manualBPM = newBPM;
            currentBPM = newBPM;
            Serial.printf("[SERIAL] BPM definido manualmente: %d\n", newBPM);
          } else {
            Serial.println("[SERIAL] ERRO: BPM deve estar entre 30 e 200");
          }
        }
        // Comando: modo automático
        else if (serialBuffer == "auto") {
          manualBPM = -1;
          Serial.println("[SERIAL] Modo automático ativado (BPM baseado em movimento)");
        }
        // Comando: ajuda
        else if (serialBuffer == "help") {
          showHelp();
        }
        // Comando: status
        else if (serialBuffer == "status") {
          Serial.println("\n========== STATUS DO SISTEMA ==========");
          Serial.printf("WiFi: %s\n", wifiEnabled ? "Habilitado" : "Desabilitado");
          Serial.printf("Conectado: %s\n", wifiConnected ? "Sim" : "Não");
          Serial.printf("MQTT: %s\n", mqttConnected ? "Conectado" : "Desconectado");
          Serial.printf("BPM Atual: %d %s\n", currentBPM, manualBPM != -1 ? "(Manual)" : "(Auto)");
          Serial.printf("Buffer RAM: %d/%d amostras\n", bufferCount, MAX_OFFLINE_SAMPLES);
          Serial.println("======================================\n");
        }
        // Comando: conectar WiFi
        else if (serialBuffer == "wifi on") {
          wifiEnabled = true;
          Serial.println("[SERIAL] WiFi habilitado via comando");
          setupWiFi();
        }
        // Comando: desconectar WiFi
        else if (serialBuffer == "wifi off") {
          wifiEnabled = false;
          WiFi.disconnect(true);
          Serial.println("[SERIAL] WiFi desabilitado via comando");
        }
        // Comando desconhecido
        else {
          Serial.println("[SERIAL] Comando desconhecido. Digite 'help' para ver comandos.");
        }
        
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}

// ========================================================================
// MOSTRAR AJUDA
// ========================================================================
void showHelp() {
  Serial.println("\n========== COMANDOS DISPONÍVEIS ==========");
  Serial.println("bpm=XX     - Definir BPM manualmente (ex: bpm=75)");
  Serial.println("auto       - Voltar ao modo automático de BPM");
  Serial.println("wifi on    - Habilitar WiFi");
  Serial.println("wifi off   - Desabilitar WiFi");
  Serial.println("status     - Mostrar status do sistema");
  Serial.println("help       - Mostrar esta ajuda");
  Serial.println("\nNOTA: Você também pode usar o botão azul");
  Serial.println("      para conectar/desconectar WiFi fisicamente");
  Serial.println("=========================================\n");
}

// ========================================================================
// CONFIGURAÇÃO DO WiFi
// ========================================================================
void setupWiFi() {
  if (!wifiEnabled) {
    Serial.println("[WiFi] WiFi está desabilitado.");
    return;
  }
  
  Serial.println("[WiFi] Configurando WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("[WiFi] Conectando");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Conectado com sucesso!");
    Serial.print("[WiFi] Endereço IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\n[WiFi] Falha na conexão. Modo offline ativo.");
    wifiConnected = false;
  }
}

// ========================================================================
// CONFIGURAÇÃO DO MQTT
// ========================================================================
void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  Serial.println("[MQTT] Broker configurado: " + String(MQTT_BROKER));
}

// ========================================================================
// RECONEXÃO MQTT
// ========================================================================
void reconnectMQTT() {
  if (!wifiConnected || !wifiEnabled) return;
  
  Serial.print("[MQTT] Tentando conectar ao broker...");
  
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println(" Conectado!");
    mqttConnected = true;
    blinkLED(LED_STATUS_PIN, 2, 100);
  } else {
    Serial.print(" Falhou. Código de erro: ");
    Serial.println(mqttClient.state());
    mqttConnected = false;
  }
}



// ========================================================================
// CONFIGURAÇÃO DOS SENSORES
// ========================================================================
void setupSensors() {
  Serial.println("[SENSOR] Inicializando DHT22...");
  dht.begin();
  delay(2000);  // DHT22 precisa de tempo para estabilizar
  
  // Teste de leitura
  float testTemp = dht.readTemperature();
  if (isnan(testTemp)) {
    Serial.println("[SENSOR] AVISO: DHT22 pode não estar conectado corretamente.");
  } else {
    Serial.println("[SENSOR] DHT22 inicializado com sucesso!");
  }
}

// ========================================================================
// CONFIGURAÇÃO DO MPU6050
// ========================================================================
void setupMPU6050() {
  Serial.println("[SENSOR] Inicializando MPU6050...");
  
  Wire.begin();
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // Wake up MPU6050
  Wire.endTransmission(true);
  
  Serial.println("[SENSOR] MPU6050 inicializado com sucesso!");
}

// ========================================================================
// LEITURA DOS SENSORES
// ========================================================================
void readSensors() {
  // Modo de teste automático: simula aumento/diminuição gradual dos valores
  if (manualBPM == -1 && autoTestMode) {
    // Simular mudança gradual dos valores
    if (increasingValues) {
      // FASE 1: SUBINDO até limites máximos (alertas críticos)
      temperature += 0.08;  // Aumenta ~0.4°C por minuto (5s * 12 = 1min)
      humidity += 0.15;     // Aumenta ~0.75% por minuto
      currentBPM += 1;      // Aumenta ~5 bpm por minuto
      
      // Verificar se atingiu os alvos máximos
      if (temperature >= tempTarget && humidity >= humidTarget && currentBPM >= bpmTarget) {
        increasingValues = false;  // Começar a descer
        tempTarget = TEMP_MIN_ALERT - 0.5;  // Novo alvo: 34.5°C (abaixo do limite)
        humidTarget = 40.0;  // Novo alvo: 40% (valor normal baixo)
        bpmTarget = BPM_MIN_ALERT - 5;  // Novo alvo: 45 bpm (abaixo do limite)
        Serial.println("\n[AUTO-TEST] Alvos MÁXIMOS atingidos! Iniciando DESCIDA...\n");
      }
    } else {
      // FASE 2: DESCENDO até limites mínimos (alertas críticos)
      temperature -= 0.08;  // Diminui ~0.4°C por minuto
      humidity -= 0.15;     // Diminui ~0.75% por minuto
      currentBPM -= 1;      // Diminui ~5 bpm por minuto
      
      // Verificar se atingiu os alvos mínimos
      if (temperature <= tempTarget && humidity <= humidTarget && currentBPM <= bpmTarget) {
        increasingValues = true;  // Voltar a subir
        tempTarget = TEMP_MAX_ALERT + 0.5;  // Voltar ao alvo máximo: 38.5°C
        humidTarget = HUMIDITY_MAX_ALERT + 5.0;  // Voltar ao alvo máximo: 85%
        bpmTarget = BPM_MAX_ALERT + 5;  // Voltar ao alvo máximo: 125 bpm
        Serial.println("\n[AUTO-TEST] Alvos MÍNIMOS atingidos! Iniciando SUBIDA...\n");
      }
    }
    
    // Garantir limites físicos realistas
    temperature = constrain(temperature, 30.0, 42.0);
    humidity = constrain(humidity, 30.0, 90.0);
    currentBPM = constrain(currentBPM, 40, 150);
    
  } else {
    // Modo normal: ler DHT22
    float dhtTemp = dht.readTemperature();
    float dhtHum = dht.readHumidity();
    
    // Verificar se as leituras são válidas
    if (!isnan(dhtTemp) && !isnan(dhtHum)) {
      temperature = dhtTemp;
      humidity = dhtHum;
    } else {
      Serial.println("[SENSOR] AVISO: Usando valores simulados (DHT22 não respondeu)");
    }
  }
  
  // Timestamp da amostra
  sampleTimestamp = millis();
  
  // Log detalhado
  Serial.printf("[SENSOR] Temp: %.1f°C | Umid: %.1f%% | BPM: %d | Mov: %.2fg", 
                temperature, humidity, currentBPM, movementIntensity);
  if (autoTestMode && manualBPM == -1) {
    Serial.printf(" [AUTO-TEST: %s]\n", increasingValues ? "SUBINDO ↑" : "DESCENDO ↓");
  } else {
    Serial.println();
  }
}

// ========================================================================
// LEITURA DO MPU6050
// ========================================================================
void readMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);  // Starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true);
  
  int16_t rawX = Wire.read() << 8 | Wire.read();
  int16_t rawY = Wire.read() << 8 | Wire.read();
  int16_t rawZ = Wire.read() << 8 | Wire.read();
  
  // Converter para g (aceleração gravitacional)
  accelX = rawX / 16384.0;
  accelY = rawY / 16384.0;
  accelZ = rawZ / 16384.0;
  
  // Calcular magnitude do movimento (excluindo gravidade)
  movementIntensity = sqrt(accelX * accelX + accelY * accelY + (accelZ - 1) * (accelZ - 1));
}

// ========================================================================
// CALCULAR BPM BASEADO EM MOVIMENTO
// ========================================================================
void calculateBPMFromMovement() {
  // Detectar movimento significativo
  if (movementIntensity > MOVEMENT_THRESHOLD) {
    lastMovement = millis();
    // Simular aumento de BPM com atividade física
    // BPM aumenta com intensidade do movimento
    int targetBPM = 70 + (int)(movementIntensity * 30);
    targetBPM = constrain(targetBPM, 60, 150);
    
    // Suavizar transição
    if (currentBPM < targetBPM) {
      currentBPM = min(currentBPM + 2, targetBPM);
    } else if (currentBPM > targetBPM) {
      currentBPM = max(currentBPM - 2, targetBPM);
    }
  } else {
    // Sem movimento, BPM retorna gradualmente ao repouso
    if (millis() - lastMovement > 10000) {  // 10 segundos sem movimento
      if (currentBPM > 70) {
        currentBPM = max(currentBPM - 1, 70);
      }
    }
  }
  
  // Se não houver movimento por muito tempo, simular BPM de repouso
  if (movementIntensity < 0.1 && currentBPM == 0) {
    currentBPM = random(60, 80);  // BPM de repouso
  }
}

// ========================================================================
// CRIAR JSON COM DADOS DOS SENSORES
// ========================================================================
String createJsonData() {
  StaticJsonDocument<300> doc;
  
  doc["timestamp"] = sampleTimestamp;
  doc["temperature"] = round(temperature * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["bpm"] = currentBPM;
  doc["movement"] = round(movementIntensity * 100) / 100.0;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["status"] = mqttConnected ? "online" : "offline";
  doc["bpm_mode"] = manualBPM != -1 ? "manual" : "auto";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}

// ========================================================================
// SALVAR DADOS EM BUFFER RAM
// ========================================================================
void saveDataToRAM(String data) {
  if (bufferCount < MAX_OFFLINE_SAMPLES) {
    offlineDataBuffer[bufferWriteIndex] = data;
    bufferWriteIndex = (bufferWriteIndex + 1) % MAX_OFFLINE_SAMPLES;
    bufferCount++;
    Serial.printf("[RAM] Dados armazenados em buffer RAM (%d/%d)\n", bufferCount, MAX_OFFLINE_SAMPLES);
  } else {
    // Buffer cheio, sobrescrever dados mais antigos (circular)
    offlineDataBuffer[bufferWriteIndex] = data;
    bufferWriteIndex = (bufferWriteIndex + 1) % MAX_OFFLINE_SAMPLES;
    bufferReadIndex = (bufferReadIndex + 1) % MAX_OFFLINE_SAMPLES;
    Serial.println("[RAM] Buffer RAM cheio, sobrescrevendo dados antigos");
  }
}

// ========================================================================
// SINCRONIZAR DADOS OFFLINE (BUFFER RAM)
// ========================================================================
void syncOfflineData() {
  if (bufferCount == 0) {
    return;  // Nada para sincronizar
  }
  
  Serial.printf("[SYNC] Sincronizando %d amostras do buffer RAM...\n", bufferCount);
  int totalSynced = 0;
  
  while (bufferCount > 0 && mqttClient.connected()) {
    String jsonData = offlineDataBuffer[bufferReadIndex];
    
    if (mqttClient.publish(MQTT_TOPIC_DATA, jsonData.c_str())) {
      totalSynced++;
      bufferReadIndex = (bufferReadIndex + 1) % MAX_OFFLINE_SAMPLES;
      bufferCount--;
    } else {
      Serial.println("[SYNC] Erro ao enviar dados, abortando...");
      break;
    }
    
    delay(100);  // Pequeno delay entre envios
  }
  
  // Limpar buffer RAM após sincronização bem-sucedida
  if (bufferCount == 0) {
    bufferWriteIndex = 0;
    bufferReadIndex = 0;
    Serial.println("[SYNC] Buffer RAM limpo!");
  }
  
  Serial.printf("[SYNC] Sincronização concluída! %d amostras enviadas.\n", totalSynced);
}

// ========================================================================
// ENVIAR DADOS VIA MQTT
// ========================================================================
void sendDataToMQTT(String jsonData) {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Não conectado. Dados não enviados.");
    return;
  }
  
  Serial.println("[MQTT] Publicando dados:");
  Serial.println(jsonData);
  
  bool success = mqttClient.publish(MQTT_TOPIC_DATA, jsonData.c_str());
  
  if (success) {
    Serial.println("[MQTT] Dados publicados com sucesso!");
  } else {
    Serial.println("[MQTT] ERRO: Falha ao publicar dados!");
  }
}

// ========================================================================
// VERIFICAR ALERTAS
// ========================================================================
void checkAlerts() {
  bool alertTriggered = false;
  String alertType = "";
  String alertMessage = "";
  String severity = "warning";
  
  // Verificar temperatura
  if (temperature > TEMP_MAX_ALERT) {
    alertType = "temperatura_alta";
    alertMessage = "Temperatura elevada: " + String(temperature, 1) + "°C (limite: " + String(TEMP_MAX_ALERT, 1) + "°C)";
    severity = "critical";
    alertTriggered = true;
  } else if (temperature < TEMP_MIN_ALERT) {
    alertType = "temperatura_baixa";
    alertMessage = "Temperatura baixa: " + String(temperature, 1) + "°C (limite: " + String(TEMP_MIN_ALERT, 1) + "°C)";
    severity = "critical";
    alertTriggered = true;
  }
  
  // Verificar BPM
  if (currentBPM > BPM_MAX_ALERT) {
    if (alertMessage.length() > 0) alertMessage += " | ";
    alertType += (alertType.length() > 0 ? "_e_" : "") + String("bpm_alto");
    alertMessage += "BPM elevado: " + String(currentBPM) + " bpm (limite: " + String(BPM_MAX_ALERT) + ")";
    severity = "critical";
    alertTriggered = true;
  } else if (currentBPM < BPM_MIN_ALERT && currentBPM > 0) {
    if (alertMessage.length() > 0) alertMessage += " | ";
    alertType += (alertType.length() > 0 ? "_e_" : "") + String("bpm_baixo");
    alertMessage += "BPM baixo: " + String(currentBPM) + " bpm (limite: " + String(BPM_MIN_ALERT) + ")";
    severity = "critical";
    alertTriggered = true;
  }
  
  // Verificar umidade
  if (humidity > HUMIDITY_MAX_ALERT) {
    if (alertMessage.length() > 0) alertMessage += " | ";
    alertType += (alertType.length() > 0 ? "_e_" : "") + String("umidade_alta");
    alertMessage += "Umidade elevada: " + String(humidity, 1) + "% (limite: " + String(HUMIDITY_MAX_ALERT, 1) + "%)";
    severity = "warning";
    alertTriggered = true;
  }
  
  // Acionar LED de alerta e enviar via MQTT
  if (alertTriggered) {
    // Piscar LED vermelho
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_ALERT_PIN, HIGH);
      delay(150);
      digitalWrite(LED_ALERT_PIN, LOW);
      delay(150);
    }
    
    // Log no Serial
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║           ⚠️  ALERTA CRÍTICO  ⚠️           ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.println("║ " + alertMessage);
    Serial.println("╚════════════════════════════════════════════╝\n");
    
    // Enviar alerta via MQTT
    if (mqttClient.connected()) {
      StaticJsonDocument<400> alertDoc;
      alertDoc["timestamp"] = millis();
      alertDoc["device_id"] = MQTT_CLIENT_ID;
      alertDoc["type"] = alertType;
      alertDoc["message"] = alertMessage;
      alertDoc["severity"] = severity;
      alertDoc["temperature"] = round(temperature * 10) / 10.0;
      alertDoc["humidity"] = round(humidity * 10) / 10.0;
      alertDoc["bpm"] = currentBPM;
      alertDoc["movement"] = round(movementIntensity * 100) / 100.0;
      
      String alertJson;
      serializeJson(alertDoc, alertJson);
      
      mqttClient.publish(MQTT_TOPIC_ALERT, alertJson.c_str());
      Serial.println("[MQTT] Alerta publicado em: " + String(MQTT_TOPIC_ALERT));
    }
  } else {
    digitalWrite(LED_ALERT_PIN, LOW);
  }
}

// ========================================================================
// PISCAR LED
// ========================================================================
void blinkLED(int pin, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
  }
}

// ========================================================================
// FIM DO CÓDIGO
// ========================================================================
