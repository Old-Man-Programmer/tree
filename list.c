/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
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

extern bool fflag, duflag, lflag, Rflag, Jflag, xdev, noreport, hyperflag, Hflag;
extern bool wcflag; // Word count flag

extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err);
extern int (*topsort)(struct _info **, struct _info **);
extern FILE *outfile;
extern int flimit, *dirs, errors;
extern ssize_t Level;
extern size_t htmldirlen;

static char errbuf[256];
char realbasepath[PATH_MAX];
size_t dirpathoffset = 0;

/**
 * Maybe TODO: Refactor the listing calls / when they are called.  A more thorough
 * analysis of the different outputs is required.  This all is not as clean as I
 * had hoped it to be.
 */

extern struct listingcalls lc;

void null_intro(void)
{
  return;
}

void null_outtro(void)
{
  return;
}

void null_close(struct _info *file, int level, int needcomma)
{
  UNUSED(file);UNUSED(level);UNUSED(needcomma);
}

void emit_tree(char **dirname, bool needfulltree)
{
	struct totals tot = { 0,0,0,0 }, subtotal; // Assuming total_word_count is the 4th member
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **dir = NULL, *info = NULL;
  char *err;
  ssize_t n;
  int i, needsclosed;
  size_t j;
  struct stat st;

  lc.intro();

  for(i=0; dirname[i]; i++) {
    if (hyperflag) {
      if (realpath(dirname[i], realbasepath) == NULL) { realbasepath[0] = '\0'; dirpathoffset = 0; }
      else dirpathoffset = strlen(dirname[i]);
    }

    if (fflag) {
      j=strlen(dirname[i]);
      do {
	if (j > 1 && dirname[i][j-1] == '/') dirname[i][--j] = 0;
      } while (j > 1 && dirname[i][j-1] == '/');
    }
    if (Hflag) htmldirlen = strlen(dirname[i]);

    if ((n = lstat(dirname[i],&st)) >= 0) {
      saveino(st.st_ino, st.st_dev);
      info = stat2info(&st);
      info->name = ""; //dirname[i];

      if (needfulltree) {
	dir = getfulltree(dirname[i], 0, st.st_dev, &(info->size), &err);
	n = err? -1 : 0;
      } else {
	push_files(dirname[i], &ig, &inf, true);
	dir = read_dir(dirname[i], &n, inf != NULL);
      }

      lc.printinfo(dirname[i], info, 0);
    } else info = NULL;

    needsclosed = lc.printfile(dirname[i], dirname[i], info, (dir != NULL) || (!dir && n));
	subtotal = (struct totals){0,0,0,0}; // Assuming total_word_count is the 4th member

    if (!dir && n) {
      lc.error("error opening dir");
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      if (!info) errors++;
		else {
		subtotal.files++;
		if (wcflag && info) subtotal.total_word_count += info->word_count;
		}
    } else if (flimit > 0 && n > flimit) {
      sprintf(errbuf,"%ld entries exceeds filelimit, not opening dir", n);
      lc.error(errbuf);
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      subtotal.dirs++;
		if (wcflag && info) subtotal.total_word_count += info->word_count;
    } else {
      lc.newline(info, 0, 0, 0);
      if (dir) {
	subtotal = listdir(dirname[i], dir, 1, st.st_dev, needfulltree);
	subtotal.dirs++;
		// If info for current dir exists and wcflag is on, its word_count (sum) contributes to subtotal
		// This assumes listdir returns sum of children, and info->word_count is sum for the dir itself.
		// For consistency, listdir should return the complete sum for what it processed.
		// If info->word_count is the sum for the directory, and listdir returns sum of children,
		// then subtotal.total_word_count from listdir is sum of children.
		// We need to add info->word_count to it if it's not already included.
		// Let's assume listdir's returned total_word_count is comprehensive for its scope.
		} else if (info) { // Single file or directory not descended into
		if (info->isdir) subtotal.dirs++;
		else subtotal.files++;
		if (wcflag) subtotal.total_word_count += info->word_count;
      }
    }
    if (dir) {
      free_dir(dir);
      dir = NULL;
    }
    if (needsclosed) lc.close(info, 0, dirname[i+1] != NULL);

    tot.files += subtotal.files;
    tot.dirs += subtotal.dirs;
	if (wcflag) tot.total_word_count += subtotal.total_word_count;
    // Do not bother to accumulate tot.size in listdir.
    // This is already done in getfulltree()
    if (duflag) tot.size += info? info->size : 0;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  if (!noreport) lc.report(tot);

  lc.outtro();
}

struct totals listdir(char *dirname, struct _info **dir, int lev, dev_t dev, bool hasfulltree)
{
	struct totals tot = {0,0,0,0}, subtotal; // Assuming total_word_count is the 4th member
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **subdir = NULL;
  size_t namemax = 257, namelen;
  int descend, htmldescend = 0;
  int needsclosed;
  ssize_t n;
  size_t dirlen = strlen(dirname)+2, pathlen = dirlen + 257;
  bool found;
  char *path, *newpath, *filename, *err = NULL;

  int es = (dirname[strlen(dirname) - 1] == '/');

  // Sanity check on dir, may or may not be necessary when using --fromfile:
  if (dir == NULL || *dir == NULL) return tot;

  for(n=0; dir[n]; n++);
  if (topsort) qsort(dir, (size_t)n, sizeof(struct _info *), (int (*)(const void *, const void *))topsort);

  dirs[lev] = *(dir+1)? 1 : 2;

  path = xmalloc(sizeof(char) * pathlen);

  for (;*dir != NULL; dir++) {
    lc.printinfo(dirname, *dir, lev);

    namelen = strlen((*dir)->name) + 1;
    if (namemax < namelen)
      path = xrealloc(path, dirlen + (namemax = namelen));
    if (es) sprintf(path,"%s%s",dirname,(*dir)->name);
    else sprintf(path,"%s/%s",dirname,(*dir)->name);
    if (fflag) filename = path;
    else filename = (*dir)->name;

    descend = 0;
    err = NULL;
    newpath = path;

    if ((*dir)->isdir) {
      tot.dirs++;

      if (!hasfulltree) {
	found = findino((*dir)->inode,(*dir)->dev);
	if (!found) {
	  saveino((*dir)->inode, (*dir)->dev);
	}
      } else found = false;

      if (!(xdev && dev != (*dir)->dev) && (!(*dir)->lnk || ((*dir)->lnk && lflag))) {
	descend = 1;

	if ((*dir)->lnk) {
	  if (*(*dir)->lnk == '/') newpath = (*dir)->lnk;
	  else {
	    if (fflag && !strcmp(dirname,"/")) sprintf(path,"%s%s",dirname,(*dir)->lnk);
	    else sprintf(path,"%s/%s",dirname,(*dir)->lnk);
	  }
	  if (found) {
	    err = "recursive, not followed";
	    /* Not actually a problem if we weren't going to descend anyway: */
	    if (Level >= 0 && lev > Level) err = NULL;
	    descend = -1;
	  }
	}

	if ((Level >= 0) && (lev > Level)) {
	  if (Rflag) {
	    FILE *outsave = outfile;
	    char *paths[2] = {newpath, NULL}, *output = xmalloc(strlen(newpath) + 13);
	    int *dirsave = xmalloc(sizeof(int) * (size_t)(lev + 2));

	    memcpy(dirsave, dirs, sizeof(int) * (size_t)(lev+1));
	    sprintf(output, "%s/00Tree.html", newpath);
	    setoutput(output);
	    emit_tree(paths, hasfulltree);

	    free(output);
	    fclose(outfile);
	    outfile = outsave;

	    memcpy(dirs, dirsave, sizeof(int) * (size_t)(lev+1));
	    free(dirsave);
	    htmldescend = 10;
	  } else htmldescend = 0;
	  descend = 0;
	}

	if (descend > 0) {
	  if (hasfulltree) {
	    subdir = (*dir)->child;
	    err = (*dir)->err;
	  } else {
	    push_files(newpath, &ig, &inf, false);
	    subdir = read_dir(newpath, &n, inf != NULL);
	    if (!subdir && n) {
	      err = "error opening dir";
	      errors++;
	    } if (flimit > 0 && n > flimit) {
	      sprintf(err = errbuf,"%ld entries exceeds filelimit, not opening dir", n);
	      errors++;
	      free_dir(subdir);
	      subdir = NULL;
	    }
	  }
	  if (subdir == NULL) descend = 0;
	}
      }
	} else {
		tot.files++;
		if (wcflag) tot.total_word_count += (*dir)->word_count; // Add word count for files
	}

    needsclosed = lc.printfile(dirname, filename, *dir, descend + htmldescend + (Jflag && errors));
    if (err) lc.error(err);

    if (descend > 0) {
      lc.newline(*dir, lev, 0, 0);

      subtotal = listdir(newpath, subdir, lev+1, dev, hasfulltree);
      tot.dirs += subtotal.dirs;
      tot.files += subtotal.files;
		if (wcflag) tot.total_word_count += subtotal.total_word_count; // Add sum from subdirectory
	} else { // Not descended or not a directory that was descended into
		if (wcflag && (*dir)->isdir && !descend && !err) { // Directory not descended (e.g. level limit)
		tot.total_word_count += (*dir)->word_count;
		}
		if (!needsclosed) lc.newline(*dir, lev, 0, *(dir+1)!=NULL);
	}

    if (subdir) {
      free_dir(subdir);
      subdir = NULL;
    }
    if (needsclosed) lc.close(*dir, descend? lev : -1, *(dir+1)!=NULL);

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  dirs[lev] = 0;
  free(path);
  return tot;
}
