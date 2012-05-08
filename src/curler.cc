#include <string>
#include <map>
#include <v8.h>
#include <node.h>
#include <curl/curl.h>
#include "curl_client.h"

using namespace std;
using namespace node;
using namespace v8;
		
class Curler: ObjectWrap
{
	public:
		static Persistent<FunctionTemplate> s_ct;
		static void Init(Handle<Object> target)
		{
			Local<FunctionTemplate> t = FunctionTemplate::New(New);

			s_ct = Persistent<FunctionTemplate>::New(t);
			s_ct->InstanceTemplate()->SetInternalFieldCount(1);
			s_ct->SetClassName(String::NewSymbol("Curler"));

			NODE_SET_PROTOTYPE_METHOD(s_ct, "request", Request);

			target->Set(String::NewSymbol("Curler"), s_ct->GetFunction());
		}

		Curler() {}
		
		~Curler() {}

		static Handle<Value> New(const Arguments& args)
		{
			Curler* hw = new Curler();
			hw->Wrap(args.This());

			return args.This();
		}

		/* struct for passing our request data around libeio */
		struct request_thread_data {
			Persistent<Function> cb;
			curl_request request;
			curl_response response;
		};

		/**************************************************************************//**
		* HTTP request
		*
		* @args request options
		* @args callback to javascript
		*****************************************************************************/
		static Handle<Value> Request (const Arguments& args) {
			HandleScope scope;
			
			if (args.Length() != 2 || !args[0]->IsObject() || !args[1]->IsFunction()) {
			 	return ThrowException(Exception::TypeError(String::New("Usage: options, callback")));
			}

			Local<Object> options = args[0]->ToObject();
			Local<Function> cb = Local<Function>::Cast(args[1]);

			//create the request struct
			request_thread_data *rtd = new request_thread_data();

			//create the curl request struct
			curl_request request;

			//validate url
			if (!options->Has(String::New("url")) || options->Get(String::New("url"))->IsUndefined() || options->Get(String::New("url"))->IsNull()) { 
				return ThrowException(Exception::TypeError(String::New("URL is a required parameter."))); 
			}
			
			//set url
			request.url = *String::Utf8Value(options->Get(String::New("url")));

			//set method type
			request.method = "GET";
			if (options->Has(String::New("method"))) {
				request.method = ToUpper(*String::Utf8Value(options->Get(String::New("method"))));
			}

			//headers
			if (HasField(options, "headers")) {
				Local<Object> headers = options->Get(String::New("headers"))->ToObject();
				if (headers->IsObject()) {
					Local<Array> keys = headers->GetPropertyNames();
					for (size_t i = 0; i< keys->Length (); ++i) {
						Local<Value> key = keys->Get(i);
						Local<Value> value = headers->Get(key);

						//add header
						request.headers.insert(pair<string,string>(*String::Utf8Value(key), *String::Utf8Value(value)));
					}
				}
			}

			//body data
			if (options->Has(String::New("data"))) {
				if (options->Get(String::New("data"))->IsObject()) {
					return ThrowException(Exception::TypeError(String::New("bodyData cannot be an object. Try using JSON.stringify(data)."))); 
				}

				request.bodyData = *String::Utf8Value(options->Get(String::New("data")));
			}

			//timeout
			request.timeout = 0;
			if (options->Has(String::New("timeout"))) {
				request.timeout = options->Get(String::New("timeout"))->IntegerValue();
			}

			//connection timeout
			request.connectionTimeout = 0;
			if (options->Has(String::New("connectionTimeout"))) {
				request.connectionTimeout = options->Get(String::New("connectionTimeout"))->IntegerValue();
			}

			//set curl request data
			rtd->request = request;

			//set callback function
			rtd->cb = Persistent<Function>::New(cb);

			eio_custom((void (*)(eio_req*))RequestWorker, EIO_PRI_DEFAULT, RequestComplete, rtd);
			ev_ref(EV_DEFAULT_UC);

			return scope.Close(Undefined());
		}

		static int RequestWorker (eio_req *req) {
			request_thread_data *rtd = static_cast<request_thread_data *> (req->data);

			CurlClient::CurlClient* curlClient = new CurlClient::CurlClient();
			rtd->response = curlClient->Request(rtd->request);
			delete curlClient;

			return 0;
		}

		/**************************************************************************//**
		* Request eio completion callback.
		*
		* @param request_thread_data
		*****************************************************************************/
		static int RequestComplete (eio_req *req) {
			request_thread_data *rtd = static_cast<request_thread_data *> (req->data);
			ev_unref(EV_DEFAULT_UC);

			Local<Value> argv[3];
			argv[0] = Local<Value>::New(Null());
			if(!rtd->response.error.empty()) {
				argv[0] = String::New(rtd->response.error.c_str());
			}

			//setup res object
			Local<Object> result = Object::New();
			result->Set(String::NewSymbol("statusCode"), Number::New(rtd->response.statusCode));
			
			//add headers
			Local<Object> headers = Object::New();
			typedef std::map<string, string>::iterator it_type;
			for(it_type iterator = rtd->response.headers.begin(); iterator != rtd->response.headers.end(); iterator++) {
				headers->Set(String::NewSymbol(ToLower(iterator->first).c_str()), String::New(iterator->second.c_str()));
			}
			result->Set(String::NewSymbol("headers"), headers);

			//add perf numbers
			result->Set(String::NewSymbol("dnsTime"), Number::New(rtd->response.dnsTime));
			result->Set(String::NewSymbol("connectTime"), Number::New(rtd->response.connectTime));
			result->Set(String::NewSymbol("preTransferTime"), Number::New(rtd->response.preTransferTime));
			result->Set(String::NewSymbol("startTransferTime"), Number::New(rtd->response.startTransferTime));
			result->Set(String::NewSymbol("totalTime"), Number::New(rtd->response.totalTime));
			argv[1] = result;

			//response data
			argv[2] = String::New(rtd->response.data.c_str());

			/* Pass the argv array object to our callback function */
			TryCatch try_catch;
			rtd->cb->Call(Context::GetCurrent()->Global(), 3, argv);
			if (try_catch.HasCaught()) {
			 	FatalException(try_catch);
			}
			
			//peanut butter clean up time
			rtd->cb.Dispose();
			delete rtd;

			return 0;
		}
	private:
		static bool HasField(Handle<Object> source, const char* fieldName)  {
			Local<String> field = String::New(fieldName);

			return (source->Has (field) && !source->Get(field)->IsUndefined() && !source->Get(field)->IsNull());
		}

		static string ToUpper(string data) {
			std::transform(data.begin(), data.end(), data.begin(), ::toupper);
			return data;
		}

		static string ToLower(string data) {
			std::transform(data.begin(), data.end(), data.begin(), ::tolower);
			return data;
		}
};

Persistent<FunctionTemplate> Curler::s_ct;

extern "C" {
	static void init (Handle<Object> target)
	{
		eio_set_min_parallel(25);
		Curler::Init(target);
	}

	NODE_MODULE(curler, init);
}
