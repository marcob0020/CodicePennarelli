#include <Arduino.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include "funzioni.h"

//Ethernet :50,51,52,10
//SD shield: 50,51,52,4
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
String Tag = "";
//String codAuto = "FA19BC2E";        //TODO: aggiungere al database
String codM;

bool flag = false;
pin pulsAnn = 26;   // pulsante annulla
pin pulsConf = 27;  // pulsante conferma

//costruttore: Pennarello(String ccolore,pin cporta,pin cmotore,pin cled,pin cvuoto,int cNumServo,pin csens)

Pennarello rosso("rosso",5,6,7,8,0,A1);
Pennarello verde("verde",11,12,13,14,180,A2);
Pennarello nero("nero",16,17,18,19,180,A3);
Pennarello blu("blu",22,23,24,25,0,A4);
Pennarello* pennarelli[]={&rosso,&verde,&nero,&blu};
String prof;

EthernetClient client;

void setup(){
    SD.begin(4);
    pinMode(pulsAnn, INPUT);
    pinMode(pulsConf, INPUT);
    rosso.initPin();
    verde.initPin();
    nero.initPin();
    blu.initPin();
    Serial.begin(9600);
    Wire.begin();       //prima aveva 2 come parametro
    //Wire.onRequest(requestEvent);  //Non usiamo più I2C perchè c'è un solo Arduino
    nfc.begin();
    codM=Pennarelli::getCodM();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata){
        while (1); // halt
    }
    String mac=Pennarelli::getMAC();
    String ip=Pennarelli::getIP();
    Pennarelli::useEthernet();
    Ethernet.begin(mac,ip);
    delay(1000);
    Pennarelli::sendStartRequest(codM,client);
    nfc.setPassiveActivationRetries(255);
    nfc.SAMConfig();
}

void loop(){
    boolean success;
    for (auto& p:pennarelli){
        p->okMotore();
    }
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    if (success){
        Tag = "";
        for (uint8_t i = 0; i < uidLength; i++){
            Tag += String (uid[i], HEX);
        }
        Tag.toUpperCase();
        Serial.println(Tag);
        if (Loop::tesseraValida(Tag)){
            flag = true;
            prof = Tag;
        }
    }
    Loop::checkPennarelliAggiunti(pennarelli,codM,e);
    Loop::checkBuffers(e,codM);
    Loop::aggiornaListaTessere(e,codM);
    requestEvent();
}

void requestEvent(){
    if (flag){
        while (flag){
            if (digitalRead(pulsAnn) == HIGH){    //Pulsante annulla
                flag = false;
            }
            if (digitalRead(pulsConf)== HIGH){
                String newFile;
                for (auto& penn:pennarelli){
                    if (penn->isSelezionato()) {
                        penn->writeporta();
                        flag = false;
                    }
                    newFile+=String(penn->getCount())
                }
                if (Pennarelli::updateValue(value)){
                    Pennarelli::httpRequest(Pennarelli::fileRequest(codM),e,FILE);
                }
            }
            for (auto& penn:pennarelli){
                if (penn->checkporta()){
                    /*penn->writeporta();
                    Loop::notificaTake(prof,codM,penn->getColore());*/
                    selezionati[nSelezionati++]=penn;
                    penn->seleziona();
                }
            }
            /*for (auto penn:pennarelli){
                if (penn->checkreset()){
                    penn->writereset();
                    break;
                }
            }*/
        }
    }
    for (auto penn:pennarelli){
        if (penn->checkcountdown()){
            penn->writecountdown();
        }
    }
}
