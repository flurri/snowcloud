#include "cloud.h"

static size_t find_offset(const char *data) {
  const char *newline = strchr(data, '\n');
  return (!newline) ? 0 : ((size_t)(newline - data));
}

char *jsonGetString(json_t *json, char *key) {
  json_t *tmp;
  char *hold, *ret;
  const char *jsonstr;

  if (!key) return NULL;
  if (!json) return NULL;

  tmp = json_object_get(json, key);
  if (!json_is_string(tmp)) {
    json_decref(tmp);
    return NULL;
  } else {
    jsonstr = json_string_value(tmp);
    ret = (char *)calloc(strlen(jsonstr)+1, sizeof(char));
    if (ret) {
      strcpy(ret, jsonstr);
    }
    json_decref(tmp);
    return ret;
  }
}

char *deconst(const char *cs) {
  char *s; // our non-const
    
  if (!cs) return NULL;
    
  s = (char *)calloc(strlen(cs)+1, sizeof(char));
  strcpy(s, cs);
    
  return s;
}

char *replaceString(char *input, char *orig, char *rep) {
  char *buf;
  char *ptr;

  if (!input) return input;
  if (!orig) return input;
  if (!rep) return input;

  if (!(ptr = strstr(input, orig))) return input;

  buf = (char *)calloc(strlen(rep) + strlen(input) + 1, sizeof(char));

  strncpy(buf, input, ptr-input);

  buf[ptr-input] = 0;
  snprintf(buf+(ptr-input), (strlen(rep) + strlen(input)), "%s%s", rep, (char *)(ptr+strlen(orig)));

  return buf;
}

int cloudapp_request_upload(uploadReq *up) {
  char url[] = "/items/new";
  char *getData, *fullurl, *base;
  json_t *root, *tmp, *params;
  json_error_t err;

  // check for unallocated stuff

  if (!up) return -1;
  if (!up->config.baseURL) return -1;
  else base = up->config.baseURL;
  if (!up->config.auth.username) return -1;
  if (!up->config.auth.password) return -1;

  fullurl = (char *)calloc(sizeof(char), 513);
  if (!fullurl) return -1;

  snprintf(fullurl, 512, "%s%s", base, url);

#ifdef _DEBUG_MODE
  printf("[libcloudapp:cloud.c][DEBUG] fullurl = %s\n", fullurl);
#endif

  getData = cloudRequest(fullurl, up->config.auth, 0, NULL);
  free(fullurl);

  if (!getData) {
    return -1;
  }

#ifdef _DEBUG_MODE
  printf("[libcloudapp:cloud.c][DEBUG] getData:\n%s\n", getData);
#endif

  root = json_loads((getData+find_offset(getData)), 0, &err);
  free(getData);

  if (!root) {
    fprintf(stderr, "[libcloudapp:cloud.c][ERROR] line %d: %s\n", err.line, err.text);
    return -1;
  }

  if (!json_is_object(root)) {
    fprintf(stderr, "[libcloudapp:cloud.c][ERROR] root must be an object\n");
      json_decref(root);
    return -1;
  }

  // get data

  // uploads_remaining
  tmp = json_object_get(root, "uploads_remaining");
  if (!tmp) {
    up->uploads_remaining = -1;
    up->isFree = 0;
  } else {
    if (json_is_integer(tmp)) {
      up->uploads_remaining = (int)json_integer_value(tmp);
      up->isFree = 1;
    } else {
      json_decref(tmp);
      json_decref(root);
      return 403; // Assume no uploads
    }
  }

  if (up->uploads_remaining == 0) {
    json_decref(tmp);
    json_decref(root);
    return 403;
  }

  json_decref(tmp);

  // max_upload_size
  tmp = json_object_get(root, "max_upload_size");
  if (!tmp) {
    json_decref(tmp);
    json_decref(root);
    return -1; // Assume malformed
  } else {
    if (json_is_integer(tmp)) {
      up->max_upload_size = json_integer_value(tmp);
    } else {
      json_decref(tmp);
      json_decref(root);
      return -1; // Assume malformed
    }
  }

  json_decref(tmp);

  up->url = jsonGetString(root, "url");
  if (!up->url) {
    json_decref(root);
    return -1;
  }

  params = json_object_get(root, "params");
  if (!params) {
    json_decref(root);
    return -1; // Assume malformed
  }

  up->params.AWSAccessKeyId = jsonGetString(params, "AWSAccessKeyId");
  if (!up->params.AWSAccessKeyId) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  up->params.key = jsonGetString(params, "key");
  if (!up->params.key) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  up->params.key = replaceString(up->params.key, "${filename}", up->filename);

  up->params.acl = jsonGetString(params, "acl");
  if (!up->params.acl) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  up->params.success_action_redirect = jsonGetString(params, "success_action_redirect");
  if (!up->params.success_action_redirect) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  up->params.signature = jsonGetString(params, "signature");
  if (!up->params.signature) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  up->params.policy = jsonGetString(params, "policy");
  if (!up->params.policy) {
    json_decref(params);
    json_decref(root);
    return -1;
  }

  json_decref(params);

  if (root->refcount > 0) {
    json_decref(root);
  }

  up->ready = 1;

  return 0;
}

int cloudapp_upload_file_to_s3(uploadReq *up, char *url) {
  char *postData;
  json_t *root, *tmp;
  json_error_t err;
  struct curl_httppost *post = NULL, *last = NULL;

  if (!up) return -1;
  if (!up->config.auth.username) return -1;
  if (!up->config.auth.password) return -1;
  if (!up->filesize) return -1;
  if (!up->file) return -1;

  if (up->ready == 0) return -1;

  curl_formadd(&post, &last, CURLFORM_COPYNAME, "AWSAccessKeyId",
               CURLFORM_PTRCONTENTS, up->params.AWSAccessKeyId,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.AWSAccessKeyId),
               CURLFORM_END);
    
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "key",
               CURLFORM_PTRCONTENTS, up->params.key,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.key),
               CURLFORM_END);
    
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "acl",
               CURLFORM_PTRCONTENTS, up->params.acl,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.acl),
               CURLFORM_END);
    
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "success_action_redirect",
               CURLFORM_PTRCONTENTS, up->params.success_action_redirect,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.success_action_redirect),
               CURLFORM_END);
    
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "signature",
               CURLFORM_PTRCONTENTS, up->params.signature,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.signature),
               CURLFORM_END);
    
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "policy",
               CURLFORM_PTRCONTENTS, up->params.policy,
               CURLFORM_CONTENTSLENGTH, strlen(up->params.policy),
               CURLFORM_END);
  
  curl_formadd(&post, &last, CURLFORM_COPYNAME, "file",
               CURLFORM_PTRCONTENTS, up->file,
               CURLFORM_CONTENTSLENGTH, up->filesize,
               CURLFORM_END);
    
  postData = cloudRequest(up->url, up->config.auth, 1, post);

  if (!postData) {
    return -2;
  }

  root = json_loads(postData, 0, &err);

#ifdef _DEBUG_MODE
  printf("[libcloudapp:cloud.c][DEBUG] postData:\n%s\n", postData);
#endif

  free(postData);

  if (!root) {
    fprintf(stderr, "[libcloudapp:cloud.c][ERROR] line %d: %s\n", err.line, err.text);
    return -1;
  }

  if (!json_is_object(root)) {
    json_decref(root);
    fprintf(stderr, "[libcloudapp:cloud.c][ERROR] root must be an object\n");
    return -1;
  }

  url = jsonGetString(root, "href");

  if (!tmp) {
    json_decref(root);
    return -1;
  }

  return 0;
}
