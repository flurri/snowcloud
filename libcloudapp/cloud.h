// cloud.h - Header for libcloudapp
// Part of snowcloud

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <curl/curl.h>

#define KB(x) x*1024
#define MB(x) KB(x)*1024

typedef struct {
  char *username;
  char *password;
} cloudAuth;

typedef struct {
  char *baseURL;
  cloudAuth auth;
} cloudConfig;

typedef struct {
  char *AWSAccessKeyId;
  char *key;
  char *acl;
  char *success_action_redirect;
  char *signature;
  char *policy;
} cloudParams;

typedef struct {
  cloudConfig config;
  int isFree;
  int uploads_remaining;
  size_t max_upload_size;
  char *url;
  char *filename;
  char *file;
  size_t filesize;
  cloudParams params;
  int ready;
} uploadReq;

struct curl_result {
  char *data;
  int pos;
};

typedef struct {
  char *key;
  size_t keyl;
  char *value;
  size_t valuel;
} paramlist;

int cloudapp_request_upload(uploadReq *);
int cloudapp_upload_file_to_s3(uploadReq *, char *);
char *cloudRequest(char *, cloudAuth, int, struct curl_httppost *);
char *deconst(const char *);
void cloudInit();
void cloudCleanup();
