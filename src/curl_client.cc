#define CURL_CLIENT
#include "curl_client.h"

CurlClient::CurlClient() {}

CurlClient::~CurlClient() {}

curl_response CurlClient::Request(curl_request request) {
	curl_response response;

	CURL *curl_handle = curl_easy_init();
 
	/* set request URL */ 
	curl_easy_setopt(curl_handle, CURLOPT_URL, request.url.c_str());

	/* set method type */
	if (request.method == "HEAD") {
		curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1); //no need to get response data if it's a HEAD request
	}
	else {
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, request.method.c_str());
	}

	/* no signal. should stop curl from killing the thread */
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

	/* set timeout */
	if (request.timeout > 0) {
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, request.timeout);
	}

	/* set connection timeout */
	if (request.connectionTimeout > 0) {
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, request.connectionTimeout);
	}

	/* add custom headers */
	struct curl_slist *list = NULL;
	if (request.headers.size() > 0) {
		typedef std::map<string, string>::iterator it_type;
		for(it_type iterator = request.headers.begin(); iterator != request.headers.end(); iterator++) {
			string header = iterator->first + ": " + iterator->second;
			list = curl_slist_append(list, header.c_str());
		}
	}
	
	/* no expect header. not required but may be beneficial in some cases, but I don't need it. */
	/* todo: allow this to be configurable. */
	list = curl_slist_append(list, "Expect:");
	curl_easy_setopt (curl_handle, CURLOPT_HTTPHEADER, list);

	/* send body data. Illegal to send body data with a GET request (even though libcurl will send it). */
	if (request.method != "GET") {
		if (request.bodyData.size() > 0) {
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request.bodyData.c_str());
		}
	}

	/* send all data to this function  */ 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);

	/* we want the headers to this file handle */
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, HeaderCallback);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response.headers);
	
	/* set error buffer */
	char errorBuffer[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);

	/* no progress meter please */ 
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

	/* todo: proxy support */
	/* do not verify peer or host */
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

	/* let's do it */
	CURLcode res = curl_easy_perform(curl_handle);
	if (res != 0) {
		response.error = errorBuffer;
	}

	/* lets get some stats on the request */
	curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &response.totalTime); 
	curl_easy_getinfo(curl_handle, CURLINFO_NAMELOOKUP_TIME, &response.dnsTime); 
	curl_easy_getinfo(curl_handle, CURLINFO_PRETRANSFER_TIME, &response.preTransferTime); 
	curl_easy_getinfo(curl_handle, CURLINFO_CONNECT_TIME, &response.connectTime); 
	curl_easy_getinfo(curl_handle, CURLINFO_STARTTRANSFER_TIME, &response.startTransferTime); 

	/* get status code */
	response.statusCode = 0;
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response.statusCode);

	/* cleanup curl handle */ 
	curl_easy_cleanup(curl_handle);

	return response;
}

size_t CurlClient::HeaderCallback(void *buffer, size_t size, size_t nmemb, void *userp) {
	int result = size * nmemb;

	std::string data = (char*) buffer;
	std::string key, value;

	size_t p = data.find(":");

	if((int)p > 0) {
		key = data.substr(0, p);
		value = data.substr(p + 2, (result - key.size() - 4)); //4 = 2 (aka ": " between header name and value) + carriage return 
		
		map<string, string> *headers = (map<string, string>*)userp;
		headers->insert(pair<string,string>(key, value));
	}

	return result;
}

size_t CurlClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	int result = size * nmemb;

	if (result > 0) {
		curl_response *response = (curl_response*)userp;
		string data = string((char*)contents, result);

		response->data = response->data + data;
	}

	return result;
}