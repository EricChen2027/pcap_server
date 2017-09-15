//
//  MY_Curl.cpp
//
//  Created by chenyu on 17/7/26.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#include <string.h>
#include "mycurl.h"

unsigned int mycurl_init() {
#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
      curlHandle = LOAD_CURLDLL("lib/libcurl.dll");
      if (!curlHandle) {
          return LIBCURL_LOAD_FAILED;
      }

      curl_global_init_p      curl_global_init    = (curl_global_init_p)GET_CURLFUNC(curlHandle, curl_global_init);
      curl_global_cleanup_p   curl_global_cleanup = (curl_global_cleanup_p)GET_CURLFUNC(curlHandle, curl_global_cleanup);
      curl_easy_init_p        curl_easy_init      = (curl_easy_init_p)GET_CURLFUNC(curlHandle, curl_easy_init);
      curl_easy_cleanup_p     curl_easy_cleanup   = (curl_easy_cleanup_p)GET_CURLFUNC(curlHandle, curl_easy_cleanup);
      curl_easy_setopt_p      curl_easy_setopt    = (curl_easy_setopt_p)GET_CURLFUNC(curlHandle, curl_easy_setopt);
      curl_easy_perform_p     curl_easy_perform   = (curl_easy_perform_p)GET_CURLFUNC(curlHandle, curl_easy_perform);
      curl_easy_getinfo_p     curl_easy_getinfo   = (curl_easy_getinfo_p)GET_CURLFUNC(curlHandle, curl_easy_getinfo);
      curl_easy_strerror_p    curl_easy_strerror  = (curl_easy_strerror_p)GET_CURLFUNC(curlHandle,  curl_easy_strerror);
#else
	//use static lib, do nothing
#endif

	curl_global_init(CURL_GLOBAL_DEFAULT);

	return 0;
}
unsigned int mycurl_uninit() {
	curl_global_cleanup();
	return 0;
}
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
	
	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;	

	return realsize;
}

unsigned int basic_request_submit(const char *url, const char *post_param, MEMORY_STRUCT &chunk) {
	mycurl_init();
    CURL *curl = curl_easy_init();
	CURLcode res = CURLE_OK;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);	
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, -1);
		if (post_param != "") {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_param));
 			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_param);
		}
   		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	  	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

		res = curl_easy_perform(curl);
		if (CURLE_OK != res) {
			long rsp_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
   			if (200 != rsp_code) {
				printf("something is wrong and the errorcode is %d\n", rsp_code);
			}
		}
	}

	mycurl_uninit();

	return res;
}
static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	FILE* file = (FILE *)userp;
	if (NULL == file) {
		return 0;
	}
	fwrite(contents, size, nmemb, file);

	return realsize;
}

unsigned int basic_request_get(const char *url, FILE* file) {
	mycurl_init();
    CURL *curl = curl_easy_init();
	CURLcode res = CURLE_OK;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);	
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, -1);
   		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
	  	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)file);

		res = curl_easy_perform(curl);
		if (CURLE_OK != res) {
			long rsp_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
   			if (200 != rsp_code) {
				printf("something is wrong and the errorcode is %d\n", rsp_code);
			}
		}
	}

	mycurl_uninit();

	return res;
}

unsigned int request_get_withua(const char *url, FILE *file, char *user_agent) {
	mycurl_init();
    CURL *curl = curl_easy_init();
	CURLcode res = CURLE_OK;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);	
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, -1);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
   		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
	  	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)file);

		res = curl_easy_perform(curl);
		if (CURLE_OK != res) {
			long rsp_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
   			if (200 != rsp_code) {
				printf("something is wrong and the errorcode is %d\n", rsp_code);
			}
		}
	}

	mycurl_uninit();

	return res;
}
