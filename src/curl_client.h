#ifndef CURLCLIENT_H
#define CURLCLIENT_H
#include <list>
#include <string>
#include <map>
#include <iostream>
#include <curl/curl.h>
#include <curl/easy.h>

using namespace std;

typedef struct {
  string method;
  string url;
  string userAgent;
  string proxy;
  long maxRedirects;
  map<string, string> headers;
  string bodyData;
  long timeout;
  long connectionTimeout;
} curl_request;

typedef struct {
  int statusCode;
  map<string, string> headers;
  string data;
  string error;
  double dnsTime;
  double connectTime;
  double preTransferTime;
  double startTransferTime;
  double totalTime;
} curl_response;

class CurlClient {
 public:
  curl_response request(curl_request request);
 ~CurlClient();
  CurlClient();

 private:
  static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
  static size_t header_callback(void *buffer, size_t size, size_t nmemb, void *userp);
};
#endif