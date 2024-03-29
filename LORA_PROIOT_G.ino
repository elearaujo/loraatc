/*Créditos:
Este sketch foi inicialmente elaborado por Pedro Bertoleti, e está disponível em https://github.com/phfbertoleti/dummy_esp32_lorawan/blob/master/src/dummy_esp32_lorawan.ino
Gerador de valor aleatório com duas casas (de 00 a 99), para o correto funcionamento na plataforma da PROIoT, observe as seguintes considerações:
 
Plataforma  https://app.proiot.com.br/devices

Em INFORMAÇÕES DO DISPOSITIVO, utilize os seguintes valores:

Ativação de segurança: ABP
Criptografia de segurança: NS
Banda: AU915-928A
Classe: A
Tamanho do contador: 2 
Validação de contadores: desabilitada
DEVICE EUI: Gerar aleatoriamente o valor 
APPLICATION SESSION KEY: Gerar aleatoriamente o valor 
NETWORK SESSION ENCRYPION KEY: Gerar aleatoriamente o valor 

Nos parametros de dispositivo 
Upload Speed 115200   <-------------------------------IMPORTANTE!!!
Definir Flash Frequence em 40 Mhz   <-------------------------------IMPORTANTE!!!
ESP32 DevKit   <-------------------------------IMPORTANTE!!!

*/





/*Bibliotecas - Utilizada a biblioteca MCCI LoRaWAN LMIC library versão 2.3.2*/
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>


/* Definições gerais */
#define BAUDRATE_SERIAL_DEBUG   115200
/* Definições do rádio LoRa (SX1276) */
#define GANHO_LORA_DBM          20 //dBm
/*Pinout - Heltec Lora V1*/
#define RADIO_RESET_PORT        14
#define RADIO_MOSI_PORT         27
#define RADIO_MISO_PORT         19
#define RADIO_SCLK_PORT         5
#define RADIO_NSS_PORT          18
#define RADIO_DIO_0_PORT        26
#define RADIO_DIO_1_PORT        33
#define RADIO_DIO_2_PORT        32
/* Constantes do rádio LoRa: GPIOs utilizados para comunicação com rádio SX1276 */
const lmic_pinmap lmic_pins = {
  .nss = RADIO_NSS_PORT,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = RADIO_RESET_PORT,
  .dio = {RADIO_DIO_0_PORT, RADIO_DIO_1_PORT, LMIC_UNUSED_PIN},
};
/* Constantes do LoraWAN - com letras maiusculas*/
/* - Chaves (Network Keys) */
static const PROGMEM u1_t NWKSKEY[16] =  { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A }; //coloque aqui sua network session key
/* - Chaves (Application Keys) */
static const u1_t PROGMEM APPSKEY[16] =  { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A }; //coloque aqui sua application session key
/* - Device Address - com letras minusculas*/
static const u4_t DEVADDR = 0x12ab34cd;
/* - Tempo entre envios de pacotes LoRa */
const unsigned TX_INTERVAL = 300; //1800s = 30 minutos 
/* Variáveis e objetos globais */
static osjob_t sendjob; //objeto para job de envio de dados via ABP
/* Callbacks para uso cpm OTAA apenas (por este projeto usar ABP, isso, eles estão vazios) */
void os_getArtEui (u1_t* buf) 
{/* Não utilizado neste projeto */  }
void os_getDevEui (u1_t* buf) 
{/* Não utilizado neste projeto */  }
void os_getDevKey (u1_t* buf) 
{/* Não utilizado neste projeto */  }

/* Callback de evento: todo evento do LoRaAN irá chamar essa callback, de forma que seja possível saber o status da comunicação com o gateway LoRaWAN. */
void onEvent (ev_t ev) /*--------------------------------------------------------------------------------------------------------- VOID--- */
{
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.println(ev);
    
    switch(ev) 
    {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println (millis());
            Serial.println(F("EV_TXCOMPLETE (incluindo espera pelas janelas de recepção)"));

            /* Verifica se ack foi recebido do gateway */
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Ack recebido"));

            /* Verifica se foram recebidos dados do gateway */  
            if (LMIC.dataLen) 
            {
                Serial.println(F("Recebidos "));
                Serial.println(LMIC.dataLen);
                Serial.println(F(" bytes (payload) do gateway"));
              
                /* Como houve recepção de dados do gateway, os coloca
                   em um array para uso futuro. */
                if (LMIC.dataLen == 1) 
                {
                    uint8_t dados_recebidos = LMIC.frame[LMIC.dataBeg + 0];
                    Serial.print(F("Dados recebidos: "));
                    Serial.write(dados_recebidos);
                }

                /* Aguarda 100ms para verificar novos eventos */
                delay(100);
             
            }
            
            /* Agenda próxima transmissão de dados ao gateway, informando daqui quanto tempo deve ocorrer (TX_INTERVAL) e qual função chamar para transmitir (do_send).
               Dessa forma, os eventos serão gerados de forma periódica. */
               
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;

        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            Serial.println (millis());
            Serial.println(LMIC.freq);
            break;
        default:
            Serial.print(F("Evento desconhecido: "));
            Serial.println((unsigned) ev);
            break;
    }
}

/* Função para envio de dados ao gateway LoRaWAN */
void do_send(osjob_t* j) /*--------------------------------------------------------------------------------------------------------- VOID--- */
{
    static uint8_t mydata[5];
    char mydata_str[5]={0};
    int numero_aleatorio = (int)random(100);

    /* Formata dado a ser enviado (número aleatório de 0 a 99) */   
    sprintf(mydata_str, "01%02d", numero_aleatorio);  
    memcpy(mydata, (uint8_t *)&mydata_str, strlen(mydata_str)); 
    
    /* Verifica se já há um envio sendo feito.
       Em caso positivo, o envio atual é suspenso. */
    if (LMIC.opmode & OP_TXRXPEND) 
    {
        Serial.println(F("OP_TXRXPEND: ha um envio ja pendente, portanto o atual envio nao sera feito"));
    } 
    else 
    {
        /* Aqui, o envio pode ser feito. */
        /* O pacote LoRaWAN é montado e o coloca na fila de envio. */
        LMIC_setTxData2(4, mydata, sizeof(mydata), 0);
        Serial.println(F("Pacote LoRaWAN na fila de envio."));       
    }
}

void setup() /*--------------------------------------------------------------------------------------------------------- VOID--- */
{
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    int b;
    
    /* Inicializa serial de debug */
    Serial.begin(BAUDRATE_SERIAL_DEBUG);

    /* Inicializa comunicação SPI com rádio LoRa */
    SPI.begin(RADIO_SCLK_PORT, RADIO_MISO_PORT, RADIO_MOSI_PORT);

    /* Inicializa stack LoRaWAN */
    os_init();
    LMIC_reset();

    /* Inicializa chaves usadas na comunicação ABP */
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);

    /* Faz inicializações de rádio pertinentes a região do 
       gateway LoRaWAN (ATC / Everynet Brasil) */
    for (b=0; b<8; ++b) 
        LMIC_disableSubBand(b);

    LMIC_enableChannel(0); // 915.2 MHz
    LMIC_enableChannel(1); // 915.4 MHz
    LMIC_enableChannel(2); // 915.6 MHz
    LMIC_enableChannel(3); // 915.8 MHz
    LMIC_enableChannel(4); // 916.0 MHz
    LMIC_enableChannel(5); // 916.2 MHz
    LMIC_enableChannel(6); // 916.4 MHz
    LMIC_enableChannel(7); // 916.6 MHz

    LMIC_setAdrMode(0);
    LMIC_setLinkCheckMode(0);

    /* Data rate para janela de recepção RX2 */
    LMIC.dn2Dr = DR_SF12CR;

    /* Configura data rate de transmissão e ganho do rádio
       LoRa (dBm) na transmissão */
    LMIC_setDrTxpow(DR_SF12, GANHO_LORA_DBM);

    /* Inicializa geração de número aleatório */
    randomSeed(0);

    /* Força primeiro envio de pacote LoRaWAN */
    do_send(&sendjob);
}

void loop()   /*--------------------------------------------------------------------------------------------------------- VOID--- */
{
    os_runloop_once();    
}
