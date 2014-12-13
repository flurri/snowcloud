#include "cloud.h"

int cloudapp_request_upload(cloudAuth auth, uploadReq *up) {
  char url[] = "/items/new";
  char *data, *input;
  size_t datalen, tokenlen, i, j;
  jsmntok_t *tokens, *tmp;
  pstate state = START;
  int httplevel = -1;

  input = cloudapp_get(url, auth, &httplevel);

  if (!input) {
    return httplevel;
  }

  tokens = json_tokenize(input);

  for (i=0,j=1;j>0;i++,j--) {
    tmp = &tokens[i];

    if ((tmp->start == -1) || (tmp->end == -1)) {
      fprintf(stderr, "[ERROR] Uninitialized token!\n");
      free(input);
      return -2;
    }

    if (tmp->type == JSMN_ARRAY || tmp->type == JSMN_OBJECT) {
      j+= tmp->size;
    }

    switch (state) {
    case START:
      if (tmp->type != JSMN_OBJECT) {
	fprintf(stderr, "[ERROR] Root must be an object! Aborting...\n");
	return -3;
      }

      state = KEY;
      tokenlen = tmp->size;

      if (!tokenlen) {
	state = STOP;
      }

      if (tokenlen % 2) {
	fprintf(stderr, "[ERROR] Object must have an even number of children! Aborting...\n");
	return -3;
      }
      break;
    case KEY:
      tokenlen--;
      if (tmp->type != JSMN_STRING) {
	fprintf(stderr, "[ERROR] Keys must be strings! Aborting...\n");
	return -4;
      }

      state = SKIP;

      for (i=0;i<sizeof(KEYS)/sizeof(char *);i++) {
	if (json_token_streq(input, tmp, KEYS[i])) {
	  printf("%s: ", KEYS[i]);
	  state = PRINT;
	  break;
	}
      }

      break;

    case SKIP:
      if (tmp->type
