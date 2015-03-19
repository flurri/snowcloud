#include "cloud.h"

int main() {
  uploadReq up;
  char *url;
  int res;
  FILE *fp;

  up.config.baseURL = "https://my.cl.ly";
  up.config.auth.username = deconst("testuser");
  up.config.auth.password = deconst("testpass");

  up.filename = (char *)malloc(sizeof(char) * 64);

  if (!up.filename) {
    printf("Could not allocate enough memory.\n");
    return 1;
  }

  printf("Filename: ");
  scanf("%[^\n]", up.filename);

  fp = fopen(up.filename, "r");

  if (!fp) {
    printf("No such file %s\n", up.filename);
    return 2;
  }

  fseek(fp, SEEK_SET, SEEK_END);

  up.filesize = ftell(fp);

  rewind(fp);

  up.file = (char *)malloc(sizeof(char) * up.filesize);

  if (!up.file) {
    printf("Could not allocate memory for file.\n");
    fclose(fp);
    free(up.filename);
    return 1;
  }

  fread(up.file, 1, up.filesize, fp);

  fclose(fp);
  
  cloudInit();

  if ((res = cloudapp_request_upload(&up)) != 0) {
    free(up.filename);
    free(up.file);
    printf("cloudapp_request_upload() returned %d", res);
    return 2;
  }

  printf("Upload to S3...\n");

  if ((res = cloudapp_upload_file_to_s3(&up, url)) != 0) {
    free(up.filename);
    free(up.file);
    printf("cloudapp_upload_file_to_s3() returned %d", res);
    return 2;
  }
  
  cloudCleanup();

  printf("Your URL: %s\n", url);
  return 0;
}
