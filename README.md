# FIAP - Faculdade de Informática e Administração Paulista

<p align="center">
<a href= "https://www.fiap.com.br/"><img src="https://raw.githubusercontent.com/lfusca/templateFiap/main/assets/logo-fiap.png" alt="FIAP - Faculdade de Informática e Admnistração Paulista" border="0" width=40% height=40%></a>
</p>

<br>

## 👨‍🎓 Integrantes do Grupo

- RM559645 - [Edimilson Ribeiro](https://www.linkedin.com/in/edimilson-ribeiro/)
- RM560173 - [Gabriel Ribeiro](https://www.linkedin.com/in/ribeirogab/)
- RM559800 - [Jonas Felipe dos Santos Lima](https://www.linkedin.com/in/jonas-felipe-dos-santos-lima-b2346811b/)
- RM559926 - [Marcos Trazzini](https://www.linkedin.com/in/mstrazzini/)

## 👩‍🏫 Professores

- [Leonardo Ruiz Orabona](https://www.linkedin.com/in/leonardoorabona)

### Coordenador(a)

- [André Godoi](https://www.linkedin.com/in/profandregodoi/)

---

## 📌 Entregas do Projeto

Este projeto representa a **Global Solution FIAP 2025.1** para o curso de **Inteligência Artificial**, desenvolvendo um sistema completo de monitoramento cardíaco IoT que utiliza sensores embarcados, comunicação MQTT e dashboard web para visualização de dados vitais em tempo real.

---

## 🩺 CardioIA - Sistema de Monitoramento Cardíaco IoT

### 🎯 Objetivos

- **Monitorar sinais vitais** em tempo real (temperatura, batimentos cardíacos, umidade ambiente e movimento)
- **Processar dados no Edge** (ESP32) para reduzir latência e garantir resiliência offline
- **Transmitir dados via MQTT** para cloud computing com sincronização automática
- **Visualizar dados** em dashboard interativo (Node-RED)
- **Emitir alertas** automáticos quando valores críticos são detectados
- **Garantir resiliência** com buffer local de até 100 amostras durante falhas de conectividade

---

## 📁 Estrutura de Pastas/Arquivos

```
CARDIO_IOT_edge/
├── esp32_wokwi/                    # Código e configuração do ESP32
│   ├── sketch.ino                  # Código principal do ESP32
│   ├── diagram.json                # Diagrama do circuito Wokwi
│   └── libraries.txt               # Dependências de bibliotecas
│
├── node_red/                       # Configuração do Node-RED
│   └── flows_hivemq_bridge.json    # Flows do dashboard e bridge MQTT
├── prints/                         # evidências funcionamento
│   └── HiveMQ Cloud.png            # Funcinamento de mensagens no HiveMQ
│   └── Node-red.png                # Fluxo de funcionamento do Node-red
│   └── Dashboard.png               # Dashboard node-red com alerta
│   └── wokwi.png                   # serial monitor e diagram wokwi
│
├── LICENSE                         # Licença do projeto
└── README.md                       # Este arquivo
```

---

## 🛠️ Componentes do Sistema

### 🔌 Hardware (ESP32 + Sensores)

**Sensores Utilizados:**
- **DHT22**: Sensor de temperatura e umidade ambiente
- **MPU6050**: Acelerômetro/giroscópio para detecção de movimento e estimativa de BPM
- **LED Status (GPIO 2)**: Indicador de conexão WiFi
- **LED Alerta (GPIO 4)**: Indicador de alertas críticos
- **Botão WiFi (GPIO 25)**: Controle manual de conectividade

**Características do Firmware:**
- ✅ Buffer RAM para 100 amostras (resiliência offline)
- ✅ Transmissão MQTT para cloud (HiveMQ)
- ✅ Sincronização automática ao reconectar
- ✅ Alertas para valores críticos
- ✅ Controles via Serial Monitor
- ✅ Modo automático e manual de BPM

### 📡 Comunicação MQTT

**Broker:** HiveMQ Cloud / HiveMQ Public  
**Tópicos:**
- `cardioIA/vitals/data` - Dados de sensores
- `cardioIA/vitals/alert` - Alertas críticos

**Formato de Mensagem (JSON):**
```json
{
  "device_id": "CardioIA_ESP32_001",
  "timestamp": 1729732800,
  "temperature": 36.5,
  "humidity": 65.0,
  "bpm": 75,
  "movement": 0.8,
  "buffer_count": 0
}
```

### 📊 Dashboard (Node-RED)

**Recursos:**
- 📈 Gráficos de linha para temperatura, BPM e umidade
- 📏 Indicadores gauge para valores em tempo real
- 🔔 Sistema de notificações para alertas
- 📋 Histórico de alertas
- 🎨 Interface responsiva e intuitiva

**Acesso:** `http://localhost:1880/ui`

---

## 🔧 Como Executar

### 1️⃣ Configuração do Simulador Wokwi

#### Opção A: Wokwi Online
1. Acesse o projeto no Wokwi: [https://wokwi.com/projects/445648982526430209](https://wokwi.com/projects/445648982526430209)
2. Clique em **"Start Simulation"**
3. Abra o Serial Monitor para visualizar logs

#### Opção B: VS Code + Wokwi Extension
1. Instale a extensão **Wokwi Simulator** no VS Code
2. Abra a pasta `esp32_wokwi/`
3. Pressione `F1` → `Wokwi: Start Simulator`

### 2️⃣ Configuração do Broker MQTT

O projeto suporta dois brokers:

#### Broker Público (para testes no Wokwi):
```cpp
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
```

#### HiveMQ Cloud (para produção com ESP32 físico):
```cpp
const char* MQTT_BROKER = "fb72a7d252f04540981122bed00f24ae.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;  // Requer TLS
const char* MQTT_USER = "iatron";
const char* MQTT_PASSWORD = "Iatron2025";
```

> ⚠️ **Nota:** O Wokwi não suporta conexões TLS/SSL. Para ESP32 físico, use a porta 8883 com TLS habilitado.

### 3️⃣ Instalação do Node-RED

#### Passo 1: Instalar Node.js
Baixe e instale o Node.js: [https://nodejs.org/](https://nodejs.org/)

#### Passo 2: Instalar Node-RED
```powershell
npm install -g --unsafe-perm node-red
```

#### Passo 3: Instalar Dashboard
```powershell
npm install -g node-red-dashboard
```

#### Passo 4: Iniciar Node-RED
```powershell
node-red
```

Acesse: `http://localhost:1880/`

#### Passo 5: Importar Flows
1. No Node-RED, clique em **Menu (≡)** → **Import**
2. Selecione o arquivo `node_red/flows_hivemq_bridge.json`
3. Clique em **Deploy**

### 4️⃣ Acessar o Dashboard
Abra em seu navegador: `http://localhost:1880/ui`

---

## 💻 Tecnologias Utilizadas

### 🔧 Hardware
- **ESP32** - Microcontrolador com WiFi integrado
- **DHT22** - Sensor de temperatura e umidade
- **MPU6050** - Acelerômetro/giroscópio I2C

### 📚 Bibliotecas Arduino
- **WiFi.h** - Conectividade WiFi
- **PubSubClient.h** - Cliente MQTT
- **DHT.h** - Leitura do sensor DHT22
- **ArduinoJson.h** - Serialização/deserialização JSON
- **Wire.h** - Comunicação I2C

### 🌐 Software
- **Node-RED** - Orquestração e dashboard
- **Node-RED Dashboard** - Interface gráfica
- **HiveMQ** - Broker MQTT (Cloud e Public)
- **Wokwi** - Simulador de circuitos ESP32

### 🔄 Protocolos
- **MQTT** - Mensageria IoT
- **I2C** - Comunicação com sensores
- **JSON** - Formato de dados

---

## 📊 Funcionalidades do Dashboard

### 📈 Monitoramento em Tempo Real
- **Temperatura Corporal** (gauge + gráfico)
  - Faixa normal: 35°C - 38°C
  - Alerta abaixo de 35°C ou acima de 38°C
  
- **Batimentos Cardíacos (BPM)** (gauge + gráfico)
  - Faixa normal: 50 - 120 bpm
  - Alerta abaixo de 50 ou acima de 120 bpm

- **Umidade Ambiente** (gauge + gráfico)
  - Alerta acima de 80%

- **Nível de Movimento** (indicador)
  - Threshold: 1.5 G

### 🔔 Sistema de Alertas

**Tipos de Alerta:**
- 🚨 **CRITICAL** - Valores extremamente fora da faixa normal
- ⚠️ **WARNING** - Valores próximos aos limites

**Apresentação:**
- Toast notifications no canto da tela
- Lista de alertas com timestamp
- Formatação com cores e ícones

### 📋 Buffer Offline
- Exibição do número de amostras armazenadas localmente
- Sincronização automática ao recuperar conexão

---

## 🎮 Comandos do Serial Monitor

O sistema aceita comandos via Serial Monitor para controle e debug:

| Comando | Descrição |
|---------|-----------|
| `bpm=XX` | Define BPM manual (ex: `bpm=75`) |
| `auto` | Retorna ao modo automático de BPM |
| `wifi on` | Habilita WiFi |
| `wifi off` | Desabilita WiFi (modo offline) |
| `status` | Exibe status completo do sistema |
| `help` | Lista todos os comandos disponíveis |

**Exemplo de uso:**
```
> status
=== STATUS DO SISTEMA ===
WiFi: Conectado
MQTT: Conectado
Buffer: 0 amostras
BPM: 75 (automático)
Temperatura: 36.5°C
Umidade: 65.0%
Movimento: 0.8 G
```

---

## ⚙️ Configuração de Alertas

Para atualizar o formato dos alertas no Node-RED, consulte o arquivo **[ATUALIZAR_ALERTAS_NODERED.md](ATUALIZAR_ALERTAS_NODERED.md)** que contém instruções detalhadas para:

- Corrigir exibição de `[object Object]`
- Formatar alertas com HTML
- Personalizar cores e ícones
- Configurar notificações toast

---

## 🔐 Parâmetros de Segurança

### Limites de Alertas Críticos

```cpp
const float TEMP_MAX_ALERT = 38.0;      // Temperatura máxima: 38°C
const float TEMP_MIN_ALERT = 35.0;      // Temperatura mínima: 35°C
const int BPM_MAX_ALERT = 120;          // BPM máximo: 120
const int BPM_MIN_ALERT = 50;           // BPM mínimo: 50
const float HUMIDITY_MAX_ALERT = 80.0;  // Umidade máxima: 80%
const float MOVEMENT_THRESHOLD = 1.5;   // Movimento: 1.5 G
```

### Resiliência do Sistema

- **Buffer RAM:** 100 amostras (500 segundos de autonomia offline)
- **Intervalo de leitura:** 5 segundos
- **Tentativas de reconexão:** A cada 5 segundos
- **Sincronização automática:** Ao restabelecer conexão

---

## 🔄 Fluxo de Dados

```
┌─────────────┐
│   Sensores  │ DHT22 + MPU6050
└──────┬──────┘
       │
       ▼
┌─────────────┐
│    ESP32    │ Processamento Edge
│   (Buffer)  │ Detecção de alertas
└──────┬──────┘
       │ MQTT
       ▼
┌─────────────┐
│   HiveMQ    │ Broker MQTT
│   (Cloud)   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Node-RED   │ Dashboard + Alertas
│ (Dashboard) │ Visualização
└─────────────┘
```

---

## 🚀 Melhorias Futuras

### 📱 Aplicativo Mobile
- App nativo para iOS/Android
- Notificações push para alertas críticos
- Histórico de dados sincronizado

### 🤖 Inteligência Artificial
- Modelo de ML para predição de anomalias cardíacas
- Detecção de padrões anormais em BPM
- Alertas preditivos baseados em tendências

### ☁️ Cloud Computing
- Armazenamento em banco de dados (MongoDB/InfluxDB)
- API RESTful para integração com outros sistemas
- Autenticação e autorização de usuários

### 🔋 Gestão de Energia
- Modo deep sleep para economia de bateria
- Otimização de consumo energético
- Monitoramento de nível de bateria

### 📊 Analytics Avançado
- Relatórios diários/semanais/mensais
- Exportação de dados (CSV, PDF)
- Comparação de tendências históricas

---

## 🧪 Testes e Validação

### Teste de Conectividade
```cpp
// No Serial Monitor
> wifi off
[INFO] WiFi desconectado
> status
Buffer: 5 amostras (armazenadas localmente)
> wifi on
[INFO] Sincronizando 5 amostras...
[INFO] Sincronização completa!
```

### Teste de Alertas
```cpp
// No Serial Monitor
> bpm=130
[ALERT] BPM elevado: 130 (limite: 120)
```

### Teste de Sensores
```cpp
// Verificar leituras no Serial Monitor
Temp: 36.5°C | Humidity: 65.0% | BPM: 75 | Movement: 0.8G
```

---

## 📚 Referências e Links Úteis

- **Projeto Wokwi:** [https://wokwi.com/projects/445648982526430209](https://wokwi.com/projects/445648982526430209)
- **Repositório GitHub:** [https://github.com/FIAP-IA2024/CARDIO_IOT_edge](https://github.com/FIAP-IA2024/CARDIO_IOT_edge)
- **Documentação Node-RED:** [https://nodered.org/docs/](https://nodered.org/docs/)
- **HiveMQ Cloud:** [https://www.hivemq.com/mqtt-cloud-broker/](https://www.hivemq.com/mqtt-cloud-broker/)
- **ESP32 Reference:** [https://docs.espressif.com/](https://docs.espressif.com/)

---

## 🎥 Demonstração

> 📹 [Link para vídeo demonstrativo do projeto](#) *(a ser adicionado)*

---

## 📄 Licença

Este projeto segue o modelo de licença da FIAP e está licenciado sob **Attribution 4.0 International**.  
Para mais informações, consulte o arquivo [LICENSE](LICENSE).

---

## 🆘 Suporte e Contato

Para dúvidas, sugestões ou contribuições, entre em contato com a equipe:

- **Jonas Felipe** - RM559800
- **Gabriel Ribeiro** - RM560173
- **Marcos Trazzini** - RM559926
- **Edimilson Ribeiro** - RM559645

---

<div align="center">

**Desenvolvido com ❤️ pela equipe FIAP-IA2024**

![FIAP](https://img.shields.io/badge/FIAP-2025.1-red?style=for-the-badge)
![Global Solution](https://img.shields.io/badge/Global%20Solution-Intelig%C3%AAncia%20Artificial-blue?style=for-the-badge)

</div>
