#include "tree.h"

extern bool hyperlinkflag;
char *hyperlinkHostname = NULL;
extern FILE *outfile;
bool osc8_enabled=FALSE;

// This function is from html.c
void url_encode(FILE *fd, char *s);

int print_osc8(const char* dirname, char* filename)
{
  int len = 0;
  char fullpath[PATH_MAX];
  char* path=NULL;

  // For the root, there is no dirname given
  if (dirname != NULL)
    len += strlen(dirname) + 1; // +1 for /
  len += strlen(filename) + 1; // +1 for \0
  path = calloc(len, sizeof(char));

  if (dirname != NULL)
    snprintf(path, len, "%s/%s", dirname, filename);
  else
    snprintf(path, len, "%s", filename);

  if (realpath(path, fullpath) == NULL) {
    perror("tree: Error executing realpath");
    return 1;
  }

  fprintf(outfile, "\033]8;;file://%s", hyperlinkHostname);
  url_encode(outfile, fullpath);
  fprintf(outfile, "\033\\");

  free(path);
  return 0;
}

void endosc8() 
{
  fprintf(outfile, "\033]8;;\033\\");
}

void osc8_setup()
{
  hyperlinkHostname = getenv("TREE_OSC8_HOST");
  if (hyperlinkHostname != NULL){
    osc8_enabled=TRUE;
  }
  else if (hyperlinkflag) {
    int maxHostLength = 256;
    char hostname[maxHostLength];
    memset(hostname, 0, maxHostLength);
    if (gethostname(hostname, maxHostLength) < 0) {
      fprintf(stderr, "tree: Error in gethostname, using \"localhost\" as a hostname.\n");
      hyperlinkHostname = strdup("localhost");
    }
    else {
      hyperlinkHostname = strdup(hostname);
    }
    osc8_enabled=TRUE;
  }
}
