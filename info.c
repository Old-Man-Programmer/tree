#include "tree.h"

/**
 * TODO: Make a "filenote" command for info comments.
 * maybe TODO: Support language extensions (i.e. .info.en, .info.gr, etc)
 * # comments
 * pattern
 * pattern
 * 	info messages
 * 	more info
 */
extern FILE *outfile;
extern const struct linedraw *linedraw;

struct infofile *infostack = NULL;

struct comment *new_comment(struct pattern *phead, char **line, int lines)
{
  struct comment *com = xmalloc(sizeof(struct comment));
  com->pattern = phead;
  com->desc = xmalloc(sizeof(char *) * (lines+1));
  int i;
  for(i=0; i < lines; i++) com->desc[i] = line[i];
  com->desc[i] = NULL;
  com->next = NULL;
  return com;
}

struct infofile *new_infofile(char *path)
{
  char buf[PATH_MAX];
  struct infofile *inf;
  struct comment *chead = NULL, *cend = NULL, *com;
  struct pattern *phead = NULL, *pend = NULL, *p;
  char *line[PATH_MAX];
  FILE *fp;
  int lines = 0;

  if (strcmp(path,INFO_PATH) == 0) fp = fopen(path, "r");
  else {
    snprintf(buf, PATH_MAX, "%s/.info", path);
    fp = fopen(buf, "r");
  }
  if (fp == NULL) return NULL;

  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') continue;
    gittrim(buf);
    if (strlen(buf) < 1) continue;

    if (buf[0] == '\t') {
      line[lines++] = scopy(buf+1);
    } else {
      if (lines) {
	// Save previous pattern/message:
	if (phead) {
	  com = new_comment(phead, line, lines);
	  if (!chead) chead = cend = com;
	  else cend = cend->next = com;
	} else {
	  // Accumulated info message lines w/ no associated pattern?
	  for(int i=0; i < lines; i++) free(line[i]);
	}
	// Reset for next pattern/message:
	phead = pend = NULL;
	lines = 0;
      }
      p = new_pattern(buf);
      if (phead == NULL) phead = pend = p;
      else pend = pend->next = p;
    }
  }
  if (phead) {
    com = new_comment(phead, line, lines);
    if (!chead) chead = cend = com;
    else cend = cend->next = com;
  } else {
    for(int i=0; i < lines; i++) free(line[i]);
  }

  fclose(fp);

  inf = xmalloc(sizeof(struct infofile));
  inf->comments = chead;
  inf->path = scopy(path);
  inf->next = NULL;

  return inf;
}

void push_infostack(struct infofile *inf)
{
  if (inf == NULL) return;
  inf->next = infostack;
  infostack = inf;
}

struct infofile *pop_infostack(void)
{
  struct infofile *inf = infostack;
  struct comment *cn, *cc;
  struct pattern *p, *c;
  infostack = infostack->next;

  if (inf == NULL) return NULL;

  for(cn = cc = inf->comments; cn != NULL; cc = cn) {
    cn = cn->next;
    for(p=c=cc->pattern; p != NULL; c = p) {
      p=p->next;
      free(c->pattern);
    }
    for(int i=0; cc->desc[i] != NULL; i++) free(cc->desc[i]);
    free(cc->desc);
    free(cc);
  }
  free(inf->path);
  free(inf);
  return NULL;
}

/**
 * Returns an info pointer if a path matches a pattern.
 * top == 1 if called in a directory with a .info file.
 */
struct comment *infocheck(char *path, char *name, int top)
{
  struct infofile *inf = infostack;
  struct comment *com;
  struct pattern *p;

  if (inf == NULL) return NULL;

  for(inf = infostack; inf != NULL; inf = inf->next) {
    for(com = inf->comments; com != NULL; com = com->next) {
      for(p = com->pattern; p != NULL; p = p->next) {
	if (patmatch(path, p->pattern) == 1) return com;
	if (top && patmatch(name, p->pattern) == 1) return com;
      }
    }
    top = 0;
  }
  return NULL;
}

void printcomment(int line, int lines, char *s)
{
  if (lines == 1) fprintf(outfile, "%s ", linedraw->csingle);
  else {
    if (line == 0) fprintf(outfile, "%s ", linedraw->ctop);
    else if (line < 2) {
      fprintf(outfile, "%s ", (lines==2)? linedraw->cbot : linedraw->cmid);
    } else {
      fprintf(outfile, "%s ", (line == lines-1)? linedraw->cbot : linedraw->cext);
    }
  }
  fprintf(outfile, "%s\n", s);
}
