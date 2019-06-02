//
// Created by marco on 22/05/2019.
//
/* File da creare sulla SD:
 * -MAC.txt : contiene il codice MAC dell' Ethernet Shield, scritto tutto di seguito senza :
 * -IP.txt : contiene l'ip dell'Ethernet Shield, scritto con i .
 * -codM.txt : contiene il codice della macchinetta. Il codice può essere visualizzato dal sito
 * -listaTessere.txt : contiene tutti i codici delle tessere. NON MODIFICARE NULLA
 * -checksum.txt : md5 dei codici delle tessere. NON MODIFICARE NULLA
 * -numero.txt : contiene il numero dei pennarelli presenti al momento. Scrivere il numero manualmente solo all'inizio. RVNB
 */
#ifndef PENNARELLI_FUNZIONI_HPP
#define PENNARELLI_FUNZIONI_HPP
#include <Arduino.h>
#include <SD.h>
#include <Ethernet.h>
#include <Servo.h>
#define ETHERNET_PORT 10
#define SD_PORT 4
//TODO: scrivere valori veri
#define SERVER "100.200.100.1"
#define Q4 100
#define Q3 200
#define Q2 300
#define Q1 400
#define Q0 500
#define MARGINE 10
#define CICLI_START 50      //TODO: mettere un valore sensato (più è alto meno fa richieste)
typedef uint8_t pin
namespace  Pennarelli {
    void useSD() {
        digitalWrite(ETHERNET_PORT, HIGH);
        digitalWrite(SD_PORT, LOW);  //attivata SD=LOW
    }

    void useEthernet() {
        digitalWrite(SD_PORT, HIGH);  //disattivata SD=HIGH
        digitalWrite(ETHERNET_PORT, LOW);
    }

    enum Fmode {
        READ, WRITE, APPEND
    };

    File openFile(String name, Fmode m = READ) {
        switch (m) {
            case READ:
                return SD.open(name);
            case WRITE:
                return SD.open(name, FILE_WRITE);
            case APPEND:
                return SD.open(name, FILE_APPEND);
        }
    }

    char readChar(File f) {
        return f.read();
    }

    String readContent(File f){
        String s;
        while (f.available()){
            s+=f.read();
        }
        return s;
    }

    String readLine(File f) {
        String r = "";
        while (f.available()) {
            char c = f.read();
            if (c == '\n') break;
            r += c;
        }
        return r;
    }
    void write(File f, String value) {
        f.println(value);
    }
    void emptyFile(String name){
        openFile(name,WRITE).close();
    }
    enum requestType{FILE,TAKE};
    bool httpRequest(String request,EthernetClient client,requestType r,bool salvaRichieste=true){
        useEthernet();
        e.stop();
        if (e.connect(SERVER,80)){
            client.println(request);
            client.println("Host: "+SERVER);
            client.println("User-Agent: arduino-ethernet");
            client.println("Connection: close");
            client.println();
            return true;
        }else{
            if (salvaRichieste) {
                useSD();
                switch (r) {
                    case FILE: {
                        emptyFile("fileBuffer.txt");
                        auto f = openFile("fileBuffer.txt", WRITE);
                        write(f, "1");
                        f.close();
                    }
                        break;
                    case TAKE: {
                        auto f = openFile("takeBuffer.txt", APPEND);
                        write(f, request + "\n");
                        f.close();
                    }
                }
            }
            return false;
        }
    }
    String fileRequest(String codM){
        useSD();
        auto f=openFile("numero.txt");
        String value=String()+readChar(f)+':'+readChar(f)+':'+readChar(f)+':'+readChar(f);
        String request="GET /pennarelli.php?action=file&codM="+codM+"&value="+value+" HTTP/1.1";
        f.close();
        return  request;
    }
    String takeRequest(String codM,String codProf,String colore){
        String request="GET /pennarelli.php?action=take&codM="+codM+"&coloreP="+colore+"&prof="+codProf+" HTTP/1.1";
        return request;
    }
    String startRequest(String codM,String checksum){
        String request="GET /pennarelli.php?action=start&codM="+codM+"&md5="+checksum+" HTTP/1.1";
        return request;
    }
    int hexToInt(char h){
        if (h>='0' && h<='9') return int(h-'0');
        else if (h>='A' && h<='F') return 10+int(h-'A');
        else return 10+int(h-'a');
    }
    String getCodM(){
        useSD();
        auto f=openFile("codM.txt");
        String r=readLine(f);
        f.close();
        return r;
    }
    byte* getMAC(){
        useSD();
        String s;
        byte r[6];
        auto f=openFile("MAC.txt");
        for (int i=0;i!=6;i++){
            s=readChar(f);
            s+=readChar(f);
            r[i]=byte(hexToInt(s[0])*16+hexToInt(s[1]));
        }
        f.close();
        return r;
    }
    byte* getIP(){
        useSD();
        String s;
        byte r[4];
        auto f=openFile("IP.txt");
        for (int i=0;f.available() && i!=4;i++){
            char c='\0';
            while (c!='.' && f.available()){
                c=readChar();
                s+=c;
            }
            r[i]=s.toInt();
            s="";
        }
        f.close();
        return r;
    }
    void sendStartRequest(String codM,EthernetClient e){
        useSD();
        auto f=openFile("checksum.txt");
        String checksum=readLine(f);
        f.close();
        String r=startRequest(codM,checksum);
        useEthernet();
        e.stop();
        if (e.connect(SERVER,80)){
            client.println(r);
            client.println("Host: "+SERVER);
            client.println("User-Agent: arduino-ethernet");
            client.println("Connection: close");
            client.println();
        }else{
            Serial.println("Connessione non disponibile. La lista delle tessere non è stata aggiornata");
        }
        delay(50);
        Serial.println("Creazione lista utenti");
        String checksum2;
        while{
            char c=e.read();
            if (c=='\n')break;
            else checksum2+=c;
        }
        if (checksum==checksum2)
            return;
        else {
            int i = 0;
            useSD();
            emptyFile("listaTessere.txt");
            useEthernet();
            while (e.available()) {
                String cod = "";
                while (true) {
                    char c = e.read();
                    cod += c;
                    if (c == '\n') break;
                }
                useSD();
                f = openFile("listaTessere.txt", APPEND);
                write(f, cod);
                delay(1);
                Serial.println("n:" + i + " ,cod:" + cod);
                i++;
                f.close();
                useEthernet();
            }
            useSD();
            emptyFile("checksum.txt");
            f=openFile("checksum.txt",WRITE);
            write(f,checksum2);
        }
    }
    int quantita(short level){
        if (level>=Q0-MARGINE && level<=Q0+MARGINE) return 0;
        else if (level>=Q1-MARGINE && level<=Q1+MARGINE) return 1;
        else if (level>=Q2-MARGINE && level<=Q2+MARGINE) return 2;
        else if (level>=Q3-MARGINE && level<=Q3+MARGINE) return 3;
        else if (level>=Q4-MARGINE && level<=Q4+MARGINE) return 4;
    }
    bool updateValue(String& newValue){
        useSD();
        auto f=openFile("numero.txt");
        String s=readLine(f);
        if (s!=newValue){
            f.close();
            emptyFile("numero.txt");
            f=openFile("numero.txt");
            write(f,newValue);
            f.close();
            return true;
        }
        return false;
    }
    short getCount(String colore){
        auto f=openFile("numero.txt");
        String counts=readLine(f);
        f.close();
        if (colore=="rosso"){
            return counts[0];
        }else if (colore=="verde"){
            return counts[1];
        }else if (colore=="nero"){
            return counts[2];
        }else if (colore=="blu"){
            return counts[3];
        }
    }
}
class Pennarello{
private:
    const pin porta;
    const pin motore;
    const Servo m;
    const pin led;
    const pin sensore;
    short countdown;
    const String colore;
    short prec;
    const pin vuoto;
    const int numServo;  //boh, chi aveva fatto il programma all'inizio non ha messo i motori nello stesso modo
    bool selezionato=false;
public:
    Pennarello(String ccolore,pin cporta,pin cmotore,pin cled,pin cvuoto,int cNumServo,pin csens): colore(ccolore),porta(cporta),motore(cmotore),led(cled),vuoto(cvuoto),numServo(cNumServo),sensore(csens){
        lastReading=analogRead(sensore);
    }
    pin getPorta(){
        return porta;
    }
    pin getSensore(){
        return sensore;
    }
    pin getMotore(){
        return motore;
    }
    pin getLed(){
        return led;
    }
    pin getVuoto(){
        return vuoto;
    }
    void operator++ (){
        countdown++;
    }
    void operator-- (){
        countdown--;
    }
    void resetCountdown(){
        countdown=0;
    }
    void okMotore(){        //boh
        m.write(numServo);
    }
    short getCount(){
        return countdown;
    }
    void initPin(){
        pinMode(porta,INPUT);
        m.attach(motore);
        pinMode(vuoto,OUTPUT);
        pinMode(led,OUTPUT);
        pinMode(sensore,INPUT);
        digitalWrite(led,LOW);
        digitalWrite(vuoto,LOW);
        countdown=Pennarelli::getCount(colore);
    }
    bool checkporta(){
        return porta == HIGH && countdown < maxP;
    }

    void writeporta(){
        writeMotore();
        delay(500);
        countdown--;
        selezionato=false;
        digitalWrite(led,LOW);
    }
    void seleziona(){
        selezionato=true;
        digitalWrite(led,HIGH);
    }
    void writeMotore(){
        m.write(180-numServo);
        delay(1000);
        m.write(numServo);
    }
    /*bool checkreset(){
        return digitalRead(reset) == HIGH;
    }
    void writereset(){
        countdown1 = 0;
        digitalWrite(vuoto, LOW);
        delay(500);
        flag = false;
    }*/
    bool checkcountdown(){
        return countdown >= maxP;
    }
    void writecountdown(){
        digitalWrite(led, LOW);
        delay(1000);
        digitalWrite(vuoto, HIGH);
        delay(500);
    }
    short leggiSensore(){
        return analogRead(sensore);
    }
    bool aggiornaQuantita(){            //return true se cambia il numero
        short n=leggiSensore();
        if (n>=prec-MARGINE && n<=prec-MARGINE){
            int q=Pennarelli::quantita(n);
            if (q>countdown) countdown=q;
            prec=n;
            return true;
        }
        prec=n;
        return false;
    }
    bool isSelezionato(){
        return selezionato;
    }
};
using Pennarelli::useEthernet;
using Pennarelli::useSD;
namespace Loop{
    void checkPennarelliAggiunti(Pennarello** p,String codM,EthernetClient e){
        String value="";
        for (int i=0;i!=4;i++){
            p[i]->aggiornaQuantita();
            value+=char(p[i]->getCount());
        }
        if (Pennarelli::updateValue(value)){
            Pennarelli::httpRequest(Pennarelli::fileRequest(codM),e,FILE);
        }
    }
    bool tesseraValida(String tag){
        useSD();
        auto f=Pennarelli::openFile("listaTessere.txt");
        while (f.available()){
            String r=Pennarelli::readLine(f);
            if (tag==r) return true;
        }
        return false;
    }
    void notificaTake(String prof,String codM,String colore,EthernetClient e){
        String taker=Pennarelli::takeRequest(codM,prof,colore);
        Pennarelli::httpRequest(taker,e,Pennarelli::requestType::TAKE);
        String filer=Pennarelli::fileRequest(codM);
        Pennarelli::httpRequest(filer,e,Pennarelli::requestType::FILE);
    }
    void checkBuffers(EthernetClient e,String codM){
        useSD();
        auto f=Pennarelli::openFile("fileBuffer.txt");
        char c=Pennarelli::readChar(f);
        f.close();
        if (c=='1'){
            String s=Pennarelli::fileRequest(codM);
            useEthernet();
            if (Pennarelli::httpRequest(s,e,FILE)){
                useSD();
                auto f=Pennarelli::openFile("fileBuffer.txt",WRITE);
                Pennarelli::write(f,"0");
                f.close();
            }
        }
        useSD();
        f=Pennarelli::openFile("takeBuffer.txt");
        String s=Pennarelli::readContent(f);
        f.close();
        for (int i=0;i!=s.length();){
            String r;
            char c='\t';
            while (c!='\n' && i!=s.length()){
                c=s[i++];
                if (c!='\n') r+=c;
            }
            if (r.length()!=0){
                useEthernet();
                Pennarelli::httpRequest(r,e,TAKE);
            }
        }
    }
    int cicliPrimaDiAggiornare=0;
    void aggiornaListaTessere(EthernetClient e,String codM){
        if (cicliPrimaDiAggiornare==CICLI_START){
            cicliPrimaDiAggiornare=0;
            Pennarelli::sendStartRequest(codM,e);
        }else cicliPrimaDiAggiornare++;
    }
}
#endif //PENNARELLI_FUNZIONI_HPP
