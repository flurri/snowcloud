#include "cloud.h"

void cloudInit() {
  curl_global_init(CURL_GLOBAL_ALL);
}

void cloudCleanup() {
  curl_global_cleanup();
}

// internal function: write data
static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream) {
  struct curl_result *result = (struct curl_result *)stream;
  if (result->pos + size * nmemb >= (KB(512))-1) {
    fprintf(stderr, "[libcloudapp:request.c][ERROR] buffer too small\n");
    return 0;
  } //if (result->pos + size * nmemb >= (KB(512))-1)

#ifdef _DEBUG_MODE
  printf("[libcloudapp:request.c][DEBUG] write_response called\n");
#endif

  memcpy(result->data + result->pos, ptr, size * nmemb);
  result->pos += size*nmemb;

  return size * nmemb;
}

char *cloudRequest(char *url, cloudAuth auth, int isPost, struct curl_httppost *params) {
  char *data = NULL;
  CURL *curl = NULL;
  CURLcode stat;
  struct curl_result result = { 0 };
  struct curl_slist *headers = NULL;
  long code;

  if (!url) return NULL;
  if (!auth.username) return NULL;
  if (!auth.password) return NULL;

  data = (char *)calloc(1, sizeof(char) * MB(1));

  if (!data) {
    return NULL;
  } // if (!data)

  result.data = data;
  result.pos = 0;
    

  // initialize cURL
  curl = curl_easy_init();

#ifdef _DEBUG_MODE
  printf("[libcloudapp:request.c][DEBUG] url = %s\n" \
         "                               result.data = %s\n", url, result.data);
#endif
  // cURL's happy, we're happy
  if (curl) {
    // time to POST!
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (isPost && params) {
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, params);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
    } // if (isPost && params)
    
    // establish headers
    headers = curl_slist_append(headers, "User-Agent: libcloudapp 1.0/X11");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // write_response function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    
    // we need to follow 301/302 redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    
    // authentication fun
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(curl, CURLOPT_USERNAME, auth.username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, auth.password);
    
    // send it out!
    stat = curl_easy_perform(curl);

    // bad response?
    if (stat != CURLE_OK) {
      fprintf(stderr, "[libcloudapp:request.c][ERROR] curl_easy_perform said: %s\n", curl_easy_strerror(stat));
#ifdef _DEBUG_MODE
      fprintf(stderr, "[libcloudapp:request.c][DEBUG] url: %s\n", url);
#endif

      if (data) {
        free(data);
        data = NULL;
      } // if (data)
    } // if (stat != CURLE_OK)

    // get the HTTP/1.x response
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    
    if (code != 200) {
      fprintf(stderr, "[libcloudapp:request.c][WARN] server responded to request %s with %ld\n", url, code);
    } // if (code != 200)

    // cleanup on cURL object
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  } // if (curl)
  
  if (data) {
    data[result.pos] = 0;
  } // if (data)

  return data;
}

