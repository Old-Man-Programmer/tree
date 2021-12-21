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

struct ignorefile *filterstack = NULL;

void gittrim(char *s)
{
  int i, e = strnlen(s,PATH_MAX)-1;

  if (s[e] == '\n') e--;

  for(i = e; i >= 0; i--) {
    if (s[i] != ' ') break;
    if (i && s[i-1] != '\\') e--;
  }
  s[e+1] = '\0';
  for(i = e = 0; s[i] != '\0';) {
    if (s[i] == '\\') i++;
    s[e++] = s[i++];
  }
  s[e] = '\0';
}

struct pattern *new_pattern(char *pattern)
{
  struct pattern *p = xmalloc(sizeof(struct pattern));
  p->pattern = scopy(pattern);
  p->next = NULL;
  return p;
}

struct ignorefile *new_ignorefile(char *path)
{
  char buf[PATH_MAX];
  struct ignorefile *ig;
  struct pattern *remove = NULL, *remend, *p;
  struct pattern *reverse = NULL, *revend;
  int rev;
  FILE *fp;

  snprintf(buf, PATH_MAX, "%s/.gitignore", path);
  fp = fopen(buf, "r");
  if (fp == NULL) return NULL;

  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') continue;
    rev = (buf[0] == '!');
    gittrim(buf);
    p = new_pattern(buf + (rev? 1 : 0));
    if (rev) {
      if (reverse == NULL) reverse = revend = p;
      else {
	revend->next = p;
	revend = p;
      }
    } else {
      if (remove == NULL) remove = remend = p;
      else {
	remend->next = p;
	remend = p;
      }
    }
    
  }

  fclose(fp);

  ig = xmalloc(sizeof(struct ignorefile));
  ig->remove = remove;
  ig->reverse = reverse;
  ig->path = scopy(path);
  ig->next = NULL;

  return ig;
}

void push_filterstack(struct ignorefile *ig)
{
  if (ig == NULL) return;
  ig->next = filterstack;
  filterstack = ig;
}

struct ignorefile *pop_filterstack(void)
{
  struct ignorefile *ig = filterstack;
  struct pattern *p, *c;
  filterstack = filterstack->next;

  for(p=c=ig->remove; p != NULL; c = p) {
    p=p->next;
    free(c->pattern);
  }
  for(p=c=ig->reverse; p != NULL; c = p) {
    p=p->next;
    free(c->pattern);
  }
  free(ig->path);
  free(ig);
  return NULL;
}

/**
 * true if remove filter matches and no reverse filter matches.
 */
int filtercheck(char *path, char *name)
{
  int filter = 0;
  struct ignorefile *ig = filterstack;
  struct pattern *p;

  while (!filter && ig) {
    for(p = ig->remove; p != NULL; p = p->next) {
      if (patmatch(path, p->pattern) == 1) {
	filter = 1;
	break;
      }
    }
    ig = ig->next;
  }
  if (!filter) return 0;

  ig = filterstack;
  while (ig) {
    for(p = ig->reverse; p != NULL; p = p->next) {
      if (patmatch(path, p->pattern) == 1) return 0;
    }
    ig = ig->next;
  }

  return 1;
}
