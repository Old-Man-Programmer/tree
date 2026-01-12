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
extern bool focusflag, focuscounts;
extern int focuscollapse;  /* 0=none, 1=files, 2=folders, 3=all */
extern int focuscollapselimit;  /* 0=no limit, >0=collapse after N consecutive */
extern char **focuspaths;
extern int nfocuspaths;

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
extern const struct linedraw *linedraw;
extern int *dirs;

/**
 * Print an anonymous collapse summary line: "... (N directories, M files)"
 * Used when focuscollapse is 'folders' or 'all'
 */
void print_focus_collapse(int level, int ndirs, int nfiles, int notlast)
{
  /* Set dirs[level] to indicate whether this is the last entry */
  dirs[level] = notlast ? 1 : 2;

  /* indent() prints tree structure including the connector (vert_left or corner) */
  indent(level);

  fprintf(outfile, "...");
  if (focuscounts) {
    fprintf(outfile, " (");
    if (ndirs > 0) {
      fprintf(outfile, "%d director%s", ndirs, ndirs == 1 ? "y" : "ies");
      if (nfiles > 0) fprintf(outfile, ", ");
    }
    if (nfiles > 0) {
      fprintf(outfile, "%d file%s", nfiles, nfiles == 1 ? "" : "s");
    }
    fprintf(outfile, ")");
  }
  fprintf(outfile, "\n");
}

/**
 * Print a named folder collapse: "dirname/... (N subdirs, M files)"
 * Used when focuscollapse is 'none' or 'files'
 */
void print_named_folder_collapse(int level, const char *name, int subdirs, int files, int notlast)
{
  /* Set dirs[level] to indicate whether this is the last entry */
  dirs[level] = notlast ? 1 : 2;

  indent(level);
  fprintf(outfile, "%s/...", name);
  if (focuscounts && (subdirs > 0 || files > 0)) {
    fprintf(outfile, " (");
    if (subdirs > 0) {
      fprintf(outfile, "%d subdirector%s", subdirs, subdirs == 1 ? "y" : "ies");
      if (files > 0) fprintf(outfile, ", ");
    }
    if (files > 0) {
      fprintf(outfile, "%d file%s", files, files == 1 ? "" : "s");
    }
    fprintf(outfile, ")");
  }
  fprintf(outfile, "\n");
}

/**
 * Count subdirectories and files in a directory.
 * If hasfulltree is true and child is available, count from children.
 * Otherwise, do a quick directory scan.
 */
void count_dir_contents(const char *path, struct _info *info, bool hasfulltree, int *subdirs, int *files)
{
  struct _info **child;
  DIR *d;
  struct dirent *ent;
  struct stat st;
  char fullpath[PATH_MAX];

  *subdirs = 0;
  *files = 0;

  if (hasfulltree && info && info->child) {
    /* Count from already-loaded children */
    for (child = info->child; *child; child++) {
      if ((*child)->isdir) (*subdirs)++;
      else (*files)++;
    }
  } else {
    /* Quick directory scan */
    d = opendir(path);
    if (d) {
      while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' ||
            (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
          continue;
        snprintf(fullpath, PATH_MAX, "%s/%s", path, ent->d_name);
        if (lstat(fullpath, &st) == 0) {
          if (S_ISDIR(st.st_mode)) (*subdirs)++;
          else (*files)++;
        }
      }
      closedir(d);
    }
  }
}

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
  struct totals tot = { 0 }, subtotal;
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
    subtotal = (struct totals){0, 0, 0};

    if (!dir && n) {
      lc.error("error opening dir");
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      if (!info) errors++;
      else subtotal.files++;
    } else if (flimit > 0 && n > flimit) {
      sprintf(errbuf,"%ld entries exceeds filelimit, not opening dir", n);
      lc.error(errbuf);
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      subtotal.dirs++;
    } else {
      lc.newline(info, 0, 0, 0);
      if (dir) {
	subtotal = listdir(dirname[i], dir, 1, st.st_dev, needfulltree);
	subtotal.dirs++;
      }
    }
    if (dir) {
      free_dir(dir);
      dir = NULL;
    }
    if (needsclosed) lc.close(info, 0, dirname[i+1] != NULL);

    tot.files += subtotal.files;
    tot.dirs += subtotal.dirs;
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
  struct totals tot = {0}, subtotal;
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

  /* Focus mode variables */
  int collapsed_dirs = 0, collapsed_files = 0;
  int run_total = 0;       /* Total consecutive collapsed items in current run */
  int run_printed = 0;     /* How many we've printed in current run */
  bool on_focus;

  int es = (dirname[strlen(dirname) - 1] == '/');

  // Sanity check on dir, may or may not be necessary when using --fromfile:
  if (dir == NULL || *dir == NULL) return tot;

  for(n=0; dir[n]; n++);
  if (topsort) qsort(dir, (size_t)n, sizeof(struct _info *), (int (*)(const void *, const void *))topsort);

  dirs[lev] = *(dir+1)? 1 : 2;

  path = xmalloc(sizeof(char) * pathlen);

  for (;*dir != NULL; dir++) {
    /* Check if entry is on focus path */
    on_focus = !focusflag || is_on_focus_path(dirname, (*dir)->name);

    /* If not on focus path, decide whether to collapse */
    if (!on_focus) {
      if ((*dir)->isdir) {
        /* Collapse directories not on focus path */
        tot.dirs++;

        if (focuscollapse <= 1) {
          /* Named collapse mode (none/files) */
          /* At start of a new run, count ahead to determine total */
          if (run_total == 0 && focuscollapselimit > 0) {
            struct _info **scan = dir;
            while (*scan && !is_on_focus_path(dirname, (*scan)->name)) {
              if ((*scan)->isdir) run_total++;
              scan++;
            }
          }

          /* Decide based on total run size vs limit */
          if (focuscollapselimit > 0 && run_total > focuscollapselimit) {
            /* Run exceeds limit: accumulate all into anonymous summary */
            collapsed_dirs++;
          } else {
            /* Run within limit: print named collapse for each dir */
            int sub_dirs = 0, sub_files = 0;
            namelen = strlen((*dir)->name) + 1;
            if (namemax < namelen)
              path = xrealloc(path, dirlen + (namemax = namelen));
            if (es) sprintf(path, "%s%s", dirname, (*dir)->name);
            else sprintf(path, "%s/%s", dirname, (*dir)->name);

            count_dir_contents(path, *dir, hasfulltree, &sub_dirs, &sub_files);
            print_named_folder_collapse(lev, (*dir)->name, sub_dirs, sub_files, *(dir+1) != NULL);
            run_printed++;
          }
        } else {
          /* Anonymous collapse (folders/all): accumulate count */
          collapsed_dirs++;
        }
        /* Update dirs[] for proper tree drawing at end of siblings */
        if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
        continue;
      } else if (focuscollapse == 1 || focuscollapse == 3) {
        /* Collapse files when mode is 'files' or 'all' */
        collapsed_files++;
        tot.files++;
        /* Update dirs[] for proper tree drawing at end of siblings */
        if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
        continue;
      }
      /* Otherwise, show the file (fall through to normal processing) */
    }

    /* Print anonymous collapse summary for accumulated entries (limit exceeded or folders/all modes) */
    if (collapsed_dirs > 0 || collapsed_files > 0) {
      print_focus_collapse(lev, collapsed_dirs, collapsed_files, *(dir+1) != NULL);
      collapsed_dirs = collapsed_files = 0;
    }
    /* Reset run counters when hitting a focus entry */
    run_total = run_printed = 0;

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
    } else tot.files++;

    needsclosed = lc.printfile(dirname, filename, *dir, descend + htmldescend + (Jflag && errors));
    if (err) lc.error(err);

    if (descend > 0) {
      lc.newline(*dir, lev, 0, 0);

      subtotal = listdir(newpath, subdir, lev+1, dev, hasfulltree);
      tot.dirs += subtotal.dirs;
      tot.files += subtotal.files;
    } else if (!needsclosed) lc.newline(*dir, lev, 0, *(dir+1)!=NULL);

    if (subdir) {
      free_dir(subdir);
      subdir = NULL;
    }
    if (needsclosed) lc.close(*dir, descend? lev : -1, *(dir+1)!=NULL);

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  /* Print any remaining collapsed entries at end of directory */
  if (collapsed_dirs > 0 || collapsed_files > 0) {
    print_focus_collapse(lev, collapsed_dirs, collapsed_files, 0);
  }

  dirs[lev] = 0;
  free(path);
  return tot;
}
