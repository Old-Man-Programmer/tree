/* $Copyright: $
 * Copyright (c) 1996 - 2021 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern bool dflag, Fflag, aflag, fflag, pruneflag;
extern bool noindent, force_color, flimit, matchdirs;
extern bool reverse;
extern int pattern, ipattern;

extern int (*topsort)();
extern FILE *outfile;
extern int Level, *dirs, maxdirs;

extern bool colorize;
extern char *endcode;

extern char *file_comment, *file_pathsep;

enum ftok { T_PATHSEP, T_DIR, T_FILE, T_EOP };

char *nextpc(char **p, int *tok)
{
  static char prev = 0;
  char *s = *p;
  if (!**p) {
    *tok = T_EOP;	// Shouldn't happen.
    return NULL;
  }
  if (prev) {
    prev = 0;
    *tok = T_PATHSEP;
    return NULL;
  }
  if (strchr(file_pathsep, **p) != NULL) {
    (*p)++;
    *tok = T_PATHSEP;
    return NULL;
  }
  while(**p && strchr(file_pathsep,**p) == NULL) (*p)++;

  if (**p) {
    *tok = T_DIR;
    prev = **p;
    *(*p)++ = '\0';
  } else *tok = T_FILE;
  return s;
}

struct _info *newent(char *name) {
  struct _info *n = xmalloc(sizeof(struct _info));
  memset(n,0,sizeof(struct _info));
  n->name = scopy(name);
  n->child = NULL;
  n->tchild = n->next = NULL;
  return n;
}

// Should replace this with a Red-Black tree implementation or the like
struct _info *search(struct _info **dir, char *name)
{
  struct _info *ptr, *prev, *n;
  int cmp;

  if (*dir == NULL) return (*dir = newent(name));

  for(prev = ptr = *dir; ptr != NULL; ptr=ptr->next) {
    cmp = strcmp(ptr->name,name);
    if (cmp == 0) return ptr;
    if (cmp > 0) break;
    prev = ptr;
  }
  n = newent(name);
  n->next = ptr;
  if (prev == ptr) *dir = n;
  else prev->next = n;
  return n;
}

void freefiletree(struct _info *ent)
{
  struct _info *ptr = ent, *t;

  while (ptr != NULL) {
    if (ptr->tchild) freefiletree(ptr->tchild);
    t = ptr;
    ptr = ptr->next;
    free(t);
  }
}

/**
 * Recursively prune (unset show flag) files/directories of matches/ignored
 * patterns:
 */
struct _info **fprune(struct _info *head, bool matched, bool root)
{
  struct _info **dir, *new = NULL, *end = NULL, *ent, *t;
  int show, count = 0;

  for(ent = head; ent != NULL;) {
    if (ent->tchild) ent->isdir = 1;

    show = 1;
    if (dflag && !ent->isdir) show = 0;
    if (!aflag && !root && ent->name[0] == '.') show = 0;
    if (show && !matched) {
      if (!ent->isdir) {
	if (pattern && !patinclude(ent->name)) show = 0;
	if (ipattern && patignore(ent->name)) show = 0;
      }
      if (ent->isdir && show && matchdirs && pattern) {
	if (patinclude(ent->name)) matched = TRUE;
      }
    }
    if (pruneflag && !matched && ent->isdir && ent->tchild == NULL) show = 0;
    if (show && ent->tchild != NULL) ent->child = fprune(ent->tchild, matched, FALSE);

    t = ent;
    ent = ent->next;
    if (show) {
      if (end) end = end->next = t;
      else new = end = t;
      count++;
    } else {
      t->next = NULL;
      freefiletree(t);
    }
  }
  if (end) end->next = NULL;

  dir = xmalloc(sizeof(struct _info *) * (count+1));
  for(count = 0, ent = new; ent != NULL; ent = ent->next, count++) {
    dir[count] = ent;
  }
  dir[count] = NULL;

  if (topsort) qsort(dir,count,sizeof(struct _info *),topsort);
  
  return dir;
}

struct _info **file_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err)
{
  FILE *fp = (strcmp(d,".")? fopen(d,"r") : stdin);
  char *path, *spath, *s;
  long pathsize;
  struct _info *root = NULL, **cwd, *ent;
  int l, tok;

  size = 0;
  if (fp == NULL) {
    fprintf(stderr,"Error opening %s for reading.\n", d);
    return NULL;
  }
  // 64K paths maximum
  path = xmalloc(sizeof(char *) * (pathsize = (64 * 1024)));

  while(fgets(path, pathsize, fp) != NULL) {
    if (file_comment != NULL && strcmp(path,file_comment) == 0) continue;
    l = strlen(path);
    while(l && isspace(path[l-1])) path[--l] = '\0';
    if (l == 0) continue;

    spath = path;
    cwd = &root;
    do {
      s = nextpc(&spath, &tok);
      if (tok == T_PATHSEP) continue;
      switch(tok) {
	case T_PATHSEP: continue;
	case T_FILE:
	case T_DIR:
	  // Should probably handle '.' and '..' entries here
	  ent = search(cwd, s);
	  // Might be empty, but should definitely be considered a directory:
	  if (tok == T_DIR) {
	    ent->isdir = 1;
	    ent->mode = S_IFDIR;
	  } else {
	    ent->mode = S_IFREG;
	  }
	  cwd = &(ent->tchild);
	  break;
      }
    } while (tok != T_FILE && tok != T_EOP);
  }
  if (fp != stdin) fclose(fp);

  // Prune accumulated directory tree:
  return fprune(root, FALSE, TRUE);
}
