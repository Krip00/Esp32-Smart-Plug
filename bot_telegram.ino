#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>



//Definizione costanti



// Pin per le misurazioni
#define pin_i 2
#define pin_v 25

// Wattmeter Unicas
#define CHANNEL_ID  "-1001573541673"

// Canale comandi non riconosciuti
#define CHANNEL_LOG  "-1001648254498"

// Inizializzazione Telegram BOT
#define BOTtoken ""  

// Credenziali wifi
const char* ssid = "Reti2022";
const char* password = "Esame";

//   Admin chat ID 
//                  Eugenio
//                              Claudio
//                                           Andrea
//                                                        Alberto                                                    
String _admin[5]={"414591792","842747883","288377235","5297298641"};

// Controlla se ci sono nuovi messaggi ogni secondo(1000 ms).
int botRequestDelay = 1000;

// Fusorario
unsigned int offset=7200;

// Modalità di interpretazione del testo
String parse_html = "HTML";

//Funzioni di trasferimento
double k_i=0.00041;
double k_v=17.5;

//Inizializzazione connessioni



// Connessione a telegram porta 443
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Connessione a server che ci restituisce l'ora su porta 1337
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "it.pool.ntp.org", offset);



//Definizione variabili


// Contatore
int checktime = 0;

// Minuti di attesa tra messaggi adiacenti pubblicati sul canale
int print_time = 1;

// Variabili di Stile con impostazioni di default
bool _italic = true;
bool _bold = true;
bool _spoiler = false;

// Messaggio fissato
int _initial_message=0;


// Variabili misurazione

// Vettori per il campionamento
double v_value [1020];
double i_value [1020];

//Valori efficaci
double v_eff;
double i_eff;

//Valori reali
double v_real;
double i_real;

//Valori medi
double v_med;
double i_med;

//Potenza apparente
double pow_app;

//Potenza attiva
double pow_act;

//Potenza complessa
double pow_comp;

//Fattore di potenza
double pow_fact;



//Funzioni


//Calcolo valor medio
void add_avg_value()
{
  i_med=0;
  v_med=0;
  for (int j=2;j<1003;j++){
    i_med=i_med+i_value[j];
    v_med=v_med+v_value[j];
  }
  i_med=i_med/1000;
  v_med=v_med/1000;      
}


//Sottrazione valor medio
void sub_avg_value()
{
  for (int j=2;j<1003;j++){
    i_value[j]=i_value[j]-i_med;
    v_value[j]=v_value[j]-v_med;
  }
      
}

//Calcolo valore reale
void real_value()
{
  for (int j=2;j<1003;j++){
    i_value[j]=i_value[j]*k_i;
    v_value[j]=v_value[j]*k_v;
  }
      
}

//Calcolo potenza attiva
void pow_active()
{
  pow_act=0;
  for (int j=2;j<1002;j++)
    pow_act=pow_act + i_value[j]*v_value[j];
  pow_act=pow_act/1000;
      
}

//Calcolo valori efficaci
void eff_value()
{
  for (int j=2;j<1003;j++)
  {
    i_eff=i_eff+i_value[j]*i_value[j];
    v_eff=v_eff+v_value[j]*v_value[j];
  }
  i_eff=sqrt(i_eff/1000);
  v_eff=sqrt(v_eff/1000); 
      
}

//Calcolo potenza apparente
void pow_apparent()
{
  pow_app=v_eff*i_eff;
      
}

//Calcolo potenza complessità
void pow_complex()
{
    pow_comp=sqrt(pow_app*pow_app - pow_act*pow_act);     
}


//Calcolo fattore di potenza
void pow_factor(){
    pow_fact=pow_act/pow_app;
}



// Funzione di calcolo potenza
void update_value ()
{
   add_avg_value();
   sub_avg_value();
   real_value();
   pow_active();
   eff_value();
   pow_apparent();
   pow_complex();
   pow_factor();
   
}  


// Funzione che verifica se il messaggio è un numero
bool is_number(const String &s) 
{
    return  std::all_of(s.begin(), s.end(), ::isdigit);
}


// Restituisce la stringa da inviare come update
String updates() 
{    
    // Aggiorna valori
    update_value();
    // Aggiorna tempo
    timeClient.update();
    String _update = "Update of: ";
    // Funzione modificata nelle librerie
    _update += timeClient.getFullFormattedTime();
    _update += "\n";
    if(_bold)
        _update += "<b>  ";
    if(_italic)
        _update += "<i>  ";
    _update += "\nIeff = ";    
    if(_spoiler)
        _update += "<tg-spoiler> ";
    _update += i_eff;
    _update += " A \n";
    if(_spoiler)
        _update += "</tg-spoiler> ";
    _update += "\nVeff = ";
   if(_spoiler)
        _update += "<tg-spoiler> ";
    _update += v_eff;
    _update += " V \n";
   if(_spoiler)
        _update += "</tg-spoiler> ";
    _update += "\nP active = ";
   if(_spoiler)
        _update += "<tg-spoiler> ";
    _update += pow_act;
    _update += " W";
    if(_spoiler)
        _update += "</tg-spoiler> ";
    _update += "\n\nP apparent = ";
   if(_spoiler)
        _update += "<tg-spoiler> ";
    _update += pow_app;
    _update += " VA";
   if(_spoiler)
        _update += "</tg-spoiler> ";
   _update += "\n\nP complex = ";
   if(_spoiler)
        _update += "<tg-spoiler> ";
    _update += pow_comp;
    _update += " var";
    if(_spoiler)
        _update += "</tg-spoiler> ";
   if(pow_app!=0){
              _update += "\n\nP factor = ";
         if(_spoiler)
              _update += "<tg-spoiler> ";
          _update += pow_fact;
          if(_spoiler)
              _update += "</tg-spoiler> ";
   }
   if(_italic)
        _update += "</i> \n";
   if(_bold)
        _update += "</b> \n";
   return _update;
}


// Gestisce i nuovi messaggi
void handleNewMessages(int numNewMessages) 
{
  // Per tutti i nuovi messaggi
  for (int i=0; i<numNewMessages; i++) 
  {
    // Acquisiamo informazioni sul contenuto del messaggio, sul mittente e sulla chat
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    String from_id = bot.messages[i].from_id;
    String from_username = bot.messages[i].from_username;
    int message_id = bot.messages[i].message_id;
    
  
    /*Eventuali comandi riservati agli admin
    for (int j=0;j<NUMERO_ADMIN;j++)
    {
        if (chat_id == _admin[0])
            bot.sendMessage(chat_id, "Welcome boss");
    }
    */


    //Modifica o Inizializzazione del messaggio fissato
    if (text == "Fix" && chat_id == CHANNEL_ID && _initial_message==0)
    {
        _initial_message =  message_id;
        String change_time = "Now publish on the channel every ";
        change_time += print_time;
        change_time += " minute ";
        bot.sendMessage(CHANNEL_ID, change_time, "", _initial_message);
    }

    
    //Salta i messaggi inviati nel canale
    if (chat_id == CHANNEL_ID)
        continue;


    //Vari comandi
    else if (text == "/update") 
    {
            String _update = "Hi, " + from_name + "\n\n\n";
            _update +=updates();
            bot.sendMessage(chat_id, _update, parse_html);
            continue;
    }

    
    else if (text == "/start")
    {
            String welcome = "Welcome, " + from_name + "\n";
            welcome += "This is the bot who control the channel: ";
            welcome += "<a href=\"t.me/Wattmeter_Unicas\"> Wattmeter Unicas</a> \n";
            welcome += "Send me /help to see all the aviable command\n";
            welcome += "For more info:";
            welcome += "<a href=\"tg://user?id="+_admin[0]+"\"> Di Gaetano Eugenio,</a>";
            welcome += "<a href=\"tg://user?id="+_admin[3]+"\"> Venditti Alberto Corrado,</a>";
            welcome += "<a href=\"tg://user?id="+_admin[1]+"\"> Colafrancesco Claudio,</a>";
            welcome += "<a href=\"tg://user?id="+_admin[2]+"\"> Calcagni Andrea</a>";
            bot.sendMessage(chat_id, welcome, parse_html);
            continue;
    }

          
    else if (text == "/help") {
            String help = "Use the following commands to control the message on the channel:\n\n";
            help += "/edit to edit the style of the update \n";
            help += "/update to print current misuraction in your chat \n";
            help += "/time_add to publish on the channel every <b>";
            help += print_time+1;
            help += "</b> minutes \n";
            help += "/time_decrese to publish on the channel every <b>";
            help += print_time-1;
            help += "</b> minutes \n";
            help += "/time_restore to publish on the channel every minute \n\n";
            help += "Send me number to use it how print time";
            bot.sendMessage(chat_id, help, parse_html);
            continue;
    }

    
    else if (text == "/time_restore") 
    {
            print_time=1;
            String change_time = "Now publish on the channel every ";
            change_time += print_time;
            change_time += " minute ";
            bot.sendMessage(chat_id, change_time);
            bot.sendMessage(CHANNEL_ID, change_time, "", _initial_message);
            continue;
    }

    
    else if (text == "/time_add") 
    {
            print_time=print_time+1;
            String change_time = "Now publish on the channel every ";
            change_time += print_time; 
            change_time += " minute ";
            bot.sendMessage(chat_id, change_time);
            bot.sendMessage(CHANNEL_ID, change_time, "", _initial_message);
            continue;
    }

    
    else if (text == "/time_decrese") 
    {
            print_time=print_time-1;
            String change_time = "Now publish on the channel every ";           
            if(print_time>0)
            { 
                change_time += print_time; 
                change_time += " minute ";
            }
            else
            {
                print_time=0;
                change_time += "5 seconds ";
            }
            bot.sendMessage(chat_id, change_time);
            bot.sendMessage(CHANNEL_ID, change_time, "", _initial_message);            
            continue;
    }


    else if (text == "/edit") 
    {
            String edit = "Welcome, you can edit the post on the channel\n";
            edit += "Use the command:\n";
            edit += "/italic_on\n/italic_off\n";
            edit += "/bold_on\n/bold_off\n";
            edit += "/spoiler_on\n/spoiler_off\n";
            edit += "/default";
            bot.sendMessage(chat_id, edit);
            continue;
    }

          
    else if (text == "/spoiler_on") 
    {
            String change_edit = "Spoiler is now <tg-spoiler>on</tg-spoiler> \n";
            _spoiler=true;
            bot.sendMessage(chat_id, change_edit, parse_html);
            continue;
    }

    
    else if (text == "/spoiler_off") 
    {
            String change_edit = "Spoiler is now off \n";
            _spoiler=false;
            bot.sendMessage(chat_id, change_edit);
            continue;
    }

          
    else if (text == "/italic_on") 
    {
            String change_edit = "<i>Italic is now on</i> \n";
            _italic=true;
            bot.sendMessage(chat_id, change_edit, parse_html);
            continue;
    }

    
    else if (text == "/italic_off") 
    {
            String change_edit = "Italic is now off \n";
            _italic=false;
            bot.sendMessage(chat_id, change_edit);
            continue;
    }


    else if (text == "/bold_on") 
    {
            String change_edit = "<b>Bold is now on</b> \n";
            _bold=true;
            bot.sendMessage(chat_id, change_edit, parse_html);
            continue;
    }

    
    else if (text == "/bold_off") 
    {
            String change_edit = "Bold is now off \n";
            _bold=false;
            bot.sendMessage(chat_id, change_edit);
            continue;
    }

    
    else if (text == "/default") 
    {
            String change_edit = "Default option is now restored \n";
            _bold=true;
            _italic=true;
            _spoiler=false;
            bot.sendMessage(chat_id, change_edit);
            continue;
    }

    
    else if(is_number(text))
    {
            print_time = text.toInt(); 
            String change_time = "Now publish on the channel every ";
            change_time += print_time; 
            change_time += " minute ";
            bot.sendMessage(chat_id, change_time);
            bot.sendMessage(CHANNEL_ID, change_time, "", _initial_message);            
            continue;
    }

    
    else
    {
            String error = "Command not found \n";
            error += "Send me /help for more info";
            String _log="Text: " + text +"\n";
            _log += "\nChat id: " + chat_id;
            _log += "\nName: " + from_name;
            _log += "\nUsername: @" + from_username;   
            _log += "\nUserid: <a href=\"tg://user?id="+from_id+"\">"+ from_id+"</a>";
            bot.sendMessage(CHANNEL_LOG, _log,parse_html);
            bot.sendMessage(chat_id, error);
            continue;
    }
   } 
}


void setup() 
{
  //Stabilisce una connessione seriale con il pc
  Serial.begin(115200);
  
  // Setto impostazioni Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
      delay(1000);       
  // Aggiungi certificato per api.telegram.org
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);   
  //Connessione al time client
  timeClient.begin();
}


void loop() 
{    
  //Salvo valori in vettori
  for(int j=0;j<1010;j++){
      v_value[j]=(analogRead(pin_i));
      delay(1);
      i_value[j+2]=(analogRead(pin_v));
      delay(1);
  }
 
  
  //Circa ogni secondo aumento
  checktime++;
  //Controllo nuovi messaggi
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while(numNewMessages) 
  {
    //Rispondo
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
  // Stampo sul canale se necessario
  if(checktime>print_time*18-1)
  {    
      bot.sendMessage(CHANNEL_ID, updates() , parse_html);
      checktime=0;
  }    

  
}
