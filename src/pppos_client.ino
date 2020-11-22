#include "gsm.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "httpClientK.h"
#include "lwip/err.h"
#include <Arduino.h>
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

String input = "";
char c;

#define WEB_SERVER "www.w3.org"
#define WEB_URL "https://www.w3.org/TR/PNG/iso_8859-1.txt"
static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\r\n"
                             "Host: " WEB_SERVER "\r\n"
                             "User-Agent: esp/1.0 esp32\r\n"
                             "\r\n";

#define BUF_SIZE (1024)
char *data = (char *)malloc(BUF_SIZE);

void gsmGprsConnect() {
  sendData("AT+CGATT=1", 500, true);
  sendData("AT+CSQ", 500, true);
  sendData("AT+CREG?", 500, true);
  sendData("AT+CPSI?", 500, true);
  sendData("AT+CGREG?", 500, true);
  sendData("AT+CGAUTH=1,1,\"\",\"\"", 500, true);
  sendData("AT+CGDCONT=1,\"IP\",\"internet.tele2.ru\"", 500, true);
  sendData("AT+CGSOCKCONT=1,\"IP\",\"internet.tele2.ru\"", 500, true);
  sendData("AT+CSOCKAUTH=1,1,\"\",\"\"", 500, true);
  sendData("AT+CSOCKSETPN=1", 500, true);
  sendData("AT+CIPMODE=0", 500, true);
  sendData("AT+CIPSENDMODE=0", 500, true);
  sendData("AT+CIPCCFG=10,0,0,0,1,0,75000", 500, true);
  sendData("AT+CIPTIMEOUT=75000,15000,15000", 500, true);
  sendData("AT+NETOPEN", 500, true);
  sendData("AT+IPADDR", 500, true);
}

void gsmPppOn(){
  sendData("AT+CGDATA=\"PPP\",1", 1000, true);
}

void gsmGpsOn(){
  sendData("AT+CGSOCKCONT=1,\"IP\",\"internet.tele2.ru\"", 500, true);
  sendData("AT+CGPSURL=\"193.193.165.37:26589\"", 500, true);
  sendData("AT+CGPS=0", 500, true);
  sendData("AT+CGPS=1,2", 500, true);
  // sendData("AT+CGPSAUTO=1", 500, true);
}

String gsmGetGpsPosition(){
  sendData("AT+CGPSINFO", 500, true);
  return "";
}

String parseGsmResponse(String response)
{
  response.trim();
  int ind = response.indexOf("\n");
  if (ind != -1)
  {
    response = response.substring(ind, response.length());
  }
  response.trim();
  return response;
}

String sendData(String command, const int timeout, boolean debug)
{
  String response = "";
  //Serial2.println(command);
  command += "\r\n";
  gsmWrite((char *)command.c_str());
  if (debug)
  {
    //Serial.println(command);
  }
  long int time = millis();
  bool startResponse = false;

  while ((time + timeout) > millis())
  {

    char *dataR;
    dataR = gsmRead();
    if (dataR != NULL)
    {
      //Serial.println(data);
      response += dataR;
    }
  }
  response.trim();
  if (debug)
  {
    Serial.println(response);
  }
  return response;
}

struct in_addr getIp(char *hostname)
{
  ip_addr_t dnssrv = dns_getserver(0);
  Serial.print("DNS server: ");
  Serial.println(inet_ntoa(dnssrv));
  struct in_addr retAddr;
  struct hostent *he = gethostbyname(hostname);
  if (he == nullptr)
  {
    retAddr.s_addr = 0;
    Serial.println("Unable");
  }
  else
  {
    retAddr = *(struct in_addr *)(he->h_addr_list[0]);
    Serial.print("DNS get: ");
    Serial.println(inet_ntoa(retAddr));
  }
  return retAddr;
}

void getFile(struct in_addr retAddr, const char *request)
{
  int s, r;
  char recv_buf[5000];
  struct sockaddr_in sa;
  inet_aton(inet_ntoa(retAddr), &sa.sin_addr.s_addr);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(80);
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
  {
    Serial.println("Failed to allocate socket.");
  }
  if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) != 0)
  {
    close(s);
    Serial.println("Failed to connect socket.");
  }
  Serial.println("connected.");
  if (write(s, request, strlen(request)) < 0)
  {
    Serial.println("Failed to send data.");
    close(s);
  }
  Serial.println("socket send success.");
  do
  {
    bzero(recv_buf, sizeof(recv_buf));
    r = read(s, recv_buf, sizeof(recv_buf) - 1);
    Serial.write(recv_buf);
  } while (r > 0);
  close(s);
  Serial.println("socket closed.");
}

void setup()
{
  Serial.begin(115200);
  gsmInit(17, 16, 115200, 1);
  ppposInit("", "");
  gsmGprsConnect();
  gsmPppOn();
  ppposStart();
  //gsmGpsOn();
  // Serial.println("1) Start GSM Communication: AT\\n");
  // Serial.println("2) Configure GSM APN: AT+CGDCONT=1i,\"IP\",\"internet\"\\n");
  // Serial.println("3) Set GSM to PPP Mode: ATD*99***1#\\n or AT+CGDATA=\"PPP\",1\\n");
  // Serial.println("4) Set ESP32 PPP: ppp\\n");
  // Serial.println("5) Check if PPP works: get\\n");
  // Serial.println("6) Try to download file: getfile\\n");
  // Serial.println("7) Stop PPP Mode and return to AT command: stop\\n");
  Serial.println();
  delay(1000);
}

void loop()
{
  if (Serial.available())
  {
    c = (char)Serial.read();
    if (c == '\n')
    {
      String inputAT = input;
      input.trim();
      if (input == "ppp")
      {
        Serial.println("ppp");
        ppposStart();
      }
      else if (input == "get")
      {
        getIp("google.com");
      }
      else if (input == "getfile")
      {
        getFile(getIp(WEB_SERVER), REQUEST);
      }
      else if (input == "lol")
      {
        String res = parseGsmResponse(sendData("AT", 500, true));
      }
      else if (input == "esptls")
      {
        String response;
        int code = httpRequest("GET", "www.google.ru", "https://google.ru", "", "", &response);
        Serial.print("Code: ");
        Serial.println(code);
        Serial.println(response);
      }
      else if (input == "gps")
      {
        gsmGetGpsPosition();
      }
      else if (input == "stop")
      {
        Serial.println("stop");
        ppposStop();
      }
      else
      {
        Serial.println(input);
        inputAT += "\n";
        gsmWrite((char *)inputAT.c_str());
      }
      input = "";
    }
    else
    {
      input += c;
    }
  }

  if (!ppposStatus())
  {
    data = gsmRead();
    if (data != NULL)
    {
      Serial.println(data);
    }
  }
}
