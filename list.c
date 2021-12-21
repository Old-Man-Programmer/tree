#include "tree.h"

extern bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
extern bool Dflag, inodeflag, devflag, Rflag, duflag, pruneflag, metafirst;
extern bool hflag, siflag, noreport, noindent, force_color, xdev, nolinks, flimit;

extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err);
extern int (*topsort)();
extern FILE *outfile;
extern int Level, *dirs, maxdirs, errors;

extern bool colorize, linktargetcolor;
extern char *endcode;
extern const struct linedraw *linedraw;

static char errbuf[256];

/**
 * Maybe TODO: Refactor the listing calls / when they are called.  A more thorough
 * analysis of the different outputs is required.  This all is not as clean as I
 * Had hoped it to be.
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
}

void emit_tree(char **dirname, bool needfulltree)
{
  struct totals tot = { 0 };
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **dir = NULL, *info = NULL;
  char *err;
  int i, j, n, needsclosed;
  struct stat st;

  lc.intro();

  for(i=0; dirname[i]; i++) {
    if (fflag) {
      j=strlen(dirname[i]);
      do {
	if (j > 1 && dirname[i][j-1] == '/') dirname[i][--j] = 0;
      } while (j > 1 && dirname[i][j-1] == '/');
    }

    if ((n = lstat(dirname[i],&st)) >= 0) {
      saveino(st.st_ino, st.st_dev);
      info = stat2info(&st);
      info->name = dirname[i];

      if (needfulltree) {
	dir = getfulltree(dirname[i], 0, st.st_dev, &(info->size), &err);
	n = err? -1 : 0;
      } else {
	push_files(dirname[i], &ig, &inf);
	dir = read_dir(dirname[i], &n, inf != NULL);
      }

      lc.printinfo(dirname[i], info, 0);
    } else info = NULL;

    needsclosed = lc.printfile(NULL, dirname[i], info, dir != NULL);

    if (!dir && n) {
      lc.error("error opening dir");
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      errors++;
    } else if (flimit > 0 && n > flimit) {
      sprintf(errbuf,"%d entries exceeds filelimit, not opening dir", n);
      lc.error(errbuf);
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      errors++;
    } else {
      lc.newline(info, 0, 0, 0);
      if (dir) {
	tot = listdir(dirname[i], dir, 1, 0, needfulltree);
	free_dir(dir);
      } else tot = (struct totals){0, 0};
    }
    if (needsclosed) lc.close(info, 0, dirname[i+1] != NULL);

    if (duflag) tot.size = info->size;
    else tot.size += st.st_size;

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
  struct _info **subdir;
  int descend, htmldescend = 0, found, n, dirlen = strlen(dirname), pathlen = dirlen + 257;
  int needsclosed;
  char *path, *newpath, *filename, *err = NULL;

  int es = (dirname[strlen(dirname) - 1] == '/');

  for(n=0; dir[n]; n++);
  if (topsort) qsort(dir, n, sizeof(struct _info *), topsort);

  dirs[lev] = *(dir+1)? 1 : 2;

  path = xmalloc(sizeof(char) * pathlen);

  for (;*dir != NULL; dir++) {
    lc.printinfo(dirname, *dir, lev);

    if (es) sprintf(path,"%s%s",dirname,(*dir)->name);
    else sprintf(path,"%s/%s",dirname,(*dir)->name);
    if (fflag) filename = path;
    else filename = (*dir)->name;

    descend = 0;
    err = NULL;

    if ((*dir)->isdir) {
      tot.dirs++;

      found = findino((*dir)->inode,(*dir)->dev);
      if (!found) saveino((*dir)->inode, (*dir)->dev);

      if (!(xdev && dev != (*dir)->dev) && (!(*dir)->lnk || ((*dir)->lnk && lflag))) {
	descend = 1;
	newpath = path;

	if ((*dir)->lnk) {
	  if (*(*dir)->lnk == '/') newpath = (*dir)->lnk;
	  else {
	    if (fflag && !strcmp(dirname,"/")) sprintf(path,"%s%s",dirname,(*dir)->lnk);
	    else sprintf(path,"%s/%s",dirname,(*dir)->lnk);
	  }
	  if (found) {
	    err = "recursive, not followed";
	    descend = 0;
	  }
	}

	if ((Level >= 0) && (lev > Level)) {
	  if (Rflag) {
	    FILE *outsave = outfile;
	    char *paths[2] = {newpath, NULL}, *output = xmalloc(strlen(newpath) + 13);
	    int *dirsave = xmalloc(sizeof(int) * (lev + 2));

	    memcpy(dirsave, dirs, sizeof(int) * (lev+1));
	    sprintf(output, "%s/00Tree.html", newpath);
	    setoutput(output);
	    emit_tree(paths, hasfulltree);

	    free(output);
	    fclose(outfile);
	    outfile = outsave;

	    memcpy(dirs, dirsave, sizeof(int) * (lev+1));
	    free(dirsave);
	    htmldescend = 1;
	  } else htmldescend = 0;
	  descend = 0;
	}

	if (descend) {
	  if (hasfulltree) {
	    subdir = (*dir)->child;
	    err = (*dir)->err;
	  } else {
	    push_files(newpath, &ig, &inf);
	    subdir = read_dir(newpath, &n, inf != NULL);
	    if (!subdir && n) {
	      err = "error opening dir";
	      errors++;
	    } if (flimit > 0 && n > flimit) {
	      sprintf(err = errbuf,"%d entries exceeds filelimit, not opening dir", n);
	      errors++;
	      free_dir(subdir);
	      subdir = NULL;
	    }
	  }
	  if (subdir == NULL) descend = 0;
	}
      }
    } else tot.files++;

    needsclosed = lc.printfile(dirname, filename, *dir, descend | htmldescend);
    if (err) lc.error(err);

    if (descend) {
      lc.newline(*dir, lev, 0, 0);

      subtotal = listdir(newpath, subdir, lev+1, dev, hasfulltree);
      tot.dirs += subtotal.dirs;
      tot.files += subtotal.files;
      tot.size += subtotal.size;
      free_dir(subdir);
    } else if (!needsclosed) lc.newline(*dir, lev, 0, *(dir+1)!=NULL);

    if (needsclosed) lc.close(*dir, descend? lev : -1, *(dir+1)!=NULL);

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    tot.size += (*dir)->size;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  dirs[lev] = 0;
  free(path);
  return tot;
}
