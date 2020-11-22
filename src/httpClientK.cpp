#include "httpClientK.h"
#include <SPIFFS.h>
#include "esp_tls.h"
#include <WiFi.h>
//#include <main.h>
#include <ArduinoJson.h>
using namespace std;

/*const unsigned char cer[] = "-----BEGIN CERTIFICATE-----\n"
	                        "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n"
                            "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
                            "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n"
                            "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n"
                            "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
                            "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n"
                            "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n"
                            "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n"
                            "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n"
                            "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n"
                            "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n"
                            "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n"
                            "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n"
                            "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n"
                            "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n"
                            "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n"
                            "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n"
                            "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n"
                            "-----END CERTIFICATE-----\n";
*/
//size_t cer_size = sizeof(cer);
//esp_err_t esp_tls_init_global_ca_store(void);
//esp_err_t d = esp_tls_set_global_ca_store(cer, cer_size);

// extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
// extern const uint8_t server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

String auth_json;
String token;
String refreshtoken;

void writefile(String namefile, String content)
{
    File file = SPIFFS.open("/" + namefile, FILE_WRITE);

    if (!file || file.isDirectory())
    {
        //  Serial.println("There was an error opening the file for writing");
        return;
    }
    if (file.print(content))
    {
        //  Serial.println("File was written");
    }
    else
    {
        // Serial.println("File write failed");
    }

    file.close();
}

String readfile(String namefile)
{

    File file = SPIFFS.open("/" + namefile, FILE_READ);

    if (!file || file.isDirectory())
    {
#ifdef DEBUG
        Serial.println("Failed to open file for reading");
#endif
        return "";
    }
    String content;
    char ch;
    while (file.available())
    {
        ch = file.read();
        if (ch == 255)
        {
            SPIFFS.format();
            //ApiBoxAuth();
            return "";
        }
        content = content + ch;
#ifdef DEBUG
        //   Serial.print(ch, DEC);
#endif
    }
    //  Serial.print("Content ");

    // Serial.println(content);
    file.close();
    return content;
}

void deletefile(String namefile)
{
    //Serial.printf("Deleting file: %s\r\n", namefile);
    if (SPIFFS.remove("/" + namefile))
    {
        //    Serial.println("- file deleted");
    }
    else
    {
        //   Serial.println("- delete failed");
    }
}

String get_body(String buf, size_t beg)
{
    size_t begin = beg;
    String body = "";
    for (int i = begin; i < buf.length(); i++)
    {
        body += buf[i];
    }
    return body;
}

int httpRequest(String method, String host, String url, String header, String data, String *response)
{
    int ret, len, beg = -1;

    int contentLength = data.length();
    char intToStr[7];
    bzero(intToStr, sizeof(intToStr));
    __itoa(contentLength, intToStr, 10);
    String ScontentLength = String(intToStr);
#ifdef DEBUG
    Serial.println(contentLength);
#endif
    String REQUEST = method + " " + url + " HTTP/1.0\r\n" +
                     "Host: " + host + "\r\n" +
                     header +
                     "Connection: close\r\n" +
                     "Content-Type: application/json\r\n" +
                     "Content-Length:" + ScontentLength + "\r\n" +
                     "\r\n" +
                     data;

    char buf[512];
    struct esp_tls *tls = nullptr;
    esp_tls_cfg_t cfg = {};
    // cfg.cacert_pem_buf = (const unsigned char *)server_root_cert_pem_start;
    //cfg.cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start;
    cfg.timeout_ms = 30000; //таймаут попытки установки соединения
    tls = esp_tls_conn_http_new(url.c_str(), &cfg);
    if (tls != NULL)
    {
#ifdef DEBUG
        Serial.print("Connection established...");
#endif
        size_t written_bytes = 0;
        do
        {
            ret = esp_tls_conn_write(tls,
                                     REQUEST.c_str() + written_bytes,
                                     REQUEST.length() - written_bytes);
            if (ret >= 0)
            {
                //ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            }
            else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
//ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
#ifdef DEBUG
                printf("esp_tls_conn_write  returned 0x%x", ret);
#endif
            }
        } while (written_bytes < REQUEST.length());

        //ESP_LOGI(TAG, "Reading HTTP response...");

        String fullResponse;

        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, len);
            ret = esp_tls_conn_read(tls, (char *)buf, len);

            if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ)
                continue;

            if (ret < 0)
            {
                //ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
                break;
            }

            if (ret == 0)
            {
                //ESP_LOGI(TAG, "connection closed");
                break;
            }

            len = ret;
            //ESP_LOGD(TAG, "%d bytes read", len);
            /* Print response directly to stdout as it is read */
            for (int i = 0; i < len; i++)
            {
                fullResponse += buf[i];
                if (buf[i] == '{')
                {
                    beg = i;
                }
            }
        } while (1);
#ifdef DEBUG
        Serial.println();
#endif
        esp_tls_conn_delete(tls);

        *response = fullResponse;
        String ShttpCode;
        for (int i = 9; i < 13; i++)
        {
            ShttpCode += fullResponse[i];
        }
        int httpCode = atoi(ShttpCode.c_str());
        return httpCode;
    }
    else
    {
        esp_tls_conn_delete(tls);
#ifdef DEBUG
        Serial.println("Connection failed...\n");
#endif
        return 400;
    }
}

// void ApiBoxAuth()
// {
//     int httpCode = 0;
//     String response;
//     httpCode = httpRequest("POST", "api.climateguard.info", "https://api.climateguard.info/api/box/login_check", "",
//                            "{\"username\":\"" + user_id + "\",\"password\":\"" + user_password + "\"}", &response);
// #ifdef DEBUG
//     Serial.print("Athorization end. Code: ");
//     Serial.println(httpCode);
// #endif
//     if (httpCode == 200)
//         jsonparseTOKEN(response); //Получение токенов из ответа сервера
// }

// void postMeasures(int GLOBAL_FRAMEWAREVERSION, int LOCAL_FRAMEWAREVERSION)
// {
// #ifdef DEBUG
//     Serial.println("Send_Mesurements");
// #endif
//     if (!SPIFFS.begin(true))
//     {
// #ifdef DEBUG
//         Serial.println("An Error has occurred while mounting SPIFFS");
// #endif
//     }
//     if (!SPIFFS.exists("/refresh.txt"))
//         ApiBoxAuth();

//     cJSON *measures = cJSON_CreateObject(); //Создаем объект JSON, в котором содержатся измерения

//     cJSON_AddStringToObject(measures, "profile", "home");
//     cJSON_AddNumberToObject(measures, "temperature", Temp);
//     cJSON_AddNumberToObject(measures, "humidity", Wet);
//     cJSON_AddNumberToObject(measures, "dust_concentration", dust[1]);
//     cJSON_AddNumberToObject(measures, "voc_concentration", VOC);
//     cJSON_AddNumberToObject(measures, "co2_concentration", CO2);
//     cJSON_AddNumberToObject(measures, "magnetic_radiation", EMnoise);
//     cJSON_AddNumberToObject(measures, "noise_level", noise);
//     cJSON_AddNumberToObject(measures, "light_level", light);
//     cJSON_AddNumberToObject(measures, "light_ripple", blink);
//     cJSON_AddNumberToObject(measures, "vibration", acceleration);
//     cJSON_AddNumberToObject(measures, "dust_concentration_pm1", dust[0]);
//     cJSON_AddNumberToObject(measures, "dust_concentration_pm10", dust[2]);
//     cJSON_AddNumberToObject(measures, "r", spectrumValues[0][5]);
//     cJSON_AddNumberToObject(measures, "o", spectrumValues[0][4]);
//     cJSON_AddNumberToObject(measures, "y", spectrumValues[0][3]);
//     cJSON_AddNumberToObject(measures, "g", spectrumValues[0][2]);
//     cJSON_AddNumberToObject(measures, "b", spectrumValues[0][1]);
//     cJSON_AddNumberToObject(measures, "v", spectrumValues[0][0]);
//     cJSON_AddNumberToObject(measures, "co", co);
//     cJSON_AddNumberToObject(measures, "no2", no2);
//     cJSON_AddNumberToObject(measures, "c2h5oh", c2h5oh);
//     cJSON_AddNumberToObject(measures, "nh3", nh3);
//     cJSON_AddNumberToObject(measures, "fg", GLOBAL_FRAMEWAREVERSION);
//     cJSON_AddNumberToObject(measures, "fl", LOCAL_FRAMEWAREVERSION);

//     char *Sjson = cJSON_Print(measures);
//     cJSON_Delete(measures);
//     if (measures)
//     {
//         measures = nullptr;
//     }

//     String data = String(Sjson);
//     if (Sjson)
//     {
//         cJSON_free(Sjson);
//         Sjson = nullptr;
//     }

//     String response;
//     int httpCode = httpRequest("POST", "api.climateguard.info", "https://api.climateguard.info/api/box/measure", "Authorization: Bearer " + readfile("access.txt") + "\r\n", data, &response);
// #ifdef DEBUG
//     Serial.print("Post measuares end. Code: ");
//     Serial.println(httpCode);
// #endif
//     if (httpCode == 401)
//     {
//         int errPost = errPostMesures(response);
//         if (errPost == 1)
//         {
//             getNewToken();
//         }
//     }
// #ifdef DEBUG
//     Serial.println("EEEEND");
// #endif
// }

// void getNewToken()
// {
//     String response;

//     if (!SPIFFS.begin(true))
//     {
// #ifdef DEBUG
//         Serial.println("An Error has occurred while mounting SPIFFS");
// #endif
//     }
//     if (!SPIFFS.exists("/refresh.txt"))
//         ApiBoxAuth();

//     String data = "{\"refresh_token\":\"" + readfile("refresh.txt") + "\"}";
// #ifdef DEBUG
//     Serial.println("Reathorization start!");
// #endif
//     int httpCode = httpRequest("POST", "api.climateguard.info", "https://api.climateguard.info/api/box/token/refresh", "", data, &response);
//     deletefile("access.txt");
//     deletefile("refresh.txt");
// #ifdef DEBUG
//     Serial.print("Reathorization end. Code: ");
//     Serial.println(httpCode);
// #endif
//     if (httpCode == 200)
//     {
//         jsonparseTOKEN(response);
//     }
// }

// void jsonparseTOKEN(String str)
// {
// #ifdef DEBUG
//     Serial.println("Begin write in SPIFFS");
// #endif
//     cJSON *jobj = cJSON_Parse(str.c_str());
//     String token = cJSON_GetObjectItem(jobj, "token")->valuestring;
//     String refresh_token = cJSON_GetObjectItem(jobj, "refresh_token")->valuestring;
//     writefile("access.txt", token);
//     writefile("refresh.txt", refresh_token);
// #ifdef DEBUG
//     Serial.println("End write in SPIFFS");
// #endif
//     cJSON_Delete(jobj);
//     if (jobj)
//     {
//         jobj = nullptr;
//     }
// }

int errPostMesures(String str)
{
#ifdef DEBUG
    Serial.println("Cheking err of POST");
#endif
    cJSON *jobj = cJSON_Parse(str.c_str());
    int code = cJSON_GetObjectItem(jobj, "code")->valueint;
    cJSON_Delete(jobj);
    if (jobj)
    {
        jobj = nullptr;
    }

    if (code == 401)
    {
        return 1;
    }
    return 0;
}