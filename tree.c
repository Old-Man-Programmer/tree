/* $Copyright: $
 * Copyright (c) 1996 - 2002 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This software is provided as is without any express or implied
 * warranties, including, without limitation, the implied warranties
 * of merchantability and fitness for a particular purpose.
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/file.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <locale.h>

static char *version = "$Version: $ tree v1.4b1 (c) 1996 - 2002 by Steve Baker, Thomas Moore $";
static char *hversion= "tree v1.4b1 \251 1996 - 2002 by Steve Baker and Thomas Moore\
<BR>HTML output hacked and copyleft \251 1998 by Francesc Rocher\
<BR>This software is experimental. Send commends and/or
<BR>suggestions to <A HREF=\"mailto:rocher@econ.udg.es\">rocher@econ.udg.es</A>";

#define scopy(x)	strcpy(xmalloc(strlen(x)+1),(x))
#define MAXDIRLEVEL	4096	/* BAD, it's possible to overflow this. */
#define MINIT		30	/* number of dir entries to initially allocate */
#define MINC		20	/* allocation increment */

#ifndef TRUE
enum bool {FALSE=0, TRUE};
#endif

struct _info {
  char *name;
  char *lnk;
  u_char isdir  : 1;
  u_char issok  : 1;
  u_char isexe  : 1;
  u_char isfifo : 1;
  u_char orphan : 1;
  u_short mode, lnkmode, uid, gid;
  u_long size;
  time_t atime, ctime, mtime;
  dev_t dev;
  ino_t inode;
};

/* Faster uid/gid -> name lookup with hash(tm)(r)(c) tables! */
#define HASH(x)		((x)&255)
struct xtable {
  u_short xid;
  char *name;
  struct xtable *nxt;
} *gtable[256], *utable[256];

/* Record inode numbers of followed sym-links to avoid refollowing them */
#define inohash(x)	((x)&255)
struct inotable {
  ino_t inode;
  dev_t device;
  struct inotable *nxt;
} *itable[256];

/* Hacked in DIR_COLORS support ---------------------------------------------- */
enum {
  CMD_COLOR, CMD_OPTIONS, CMD_TERM, CMD_EIGHTBIT, COL_NORMAL, COL_FILE, COL_DIR,
  COL_LINK, COL_FIFO, COL_SOCK, COL_BLK, COL_CHR, COL_EXEC, DOT_EXTENSION, ERROR,
  COL_ORPHAN, COL_MISSING, COL_LEFTCODE, COL_RIGHTCODE, COL_ENDCODE
};

char colorize = FALSE, ansilines = FALSE, asciilines = FALSE;
char *term, termmatch = FALSE, istty;
char *leftcode = NULL, *rightcode = NULL, *endcode = NULL;

char *norm_flgs = NULL, *file_flgs = NULL, *dir_flgs = NULL, *link_flgs = NULL;
char *fifo_flgs = NULL, *sock_flgs = NULL, *block_flgs = NULL, *char_flgs = NULL;
char *exec_flgs = NULL, *orphan_flgs = NULL, *missing_flgs = NULL;

char *vgacolor[] = {
  "black", "red", "green", "yellow", "blue", "fuchsia", "aqua", "white",
  NULL, NULL,
  "transparent", "red", "green", "yellow", "blue", "fuchsia", "aqua", "black"
};

struct colortable {
  char *term_flg, *CSS_name, *font_fg, *font_bg;
} colortable[11];

struct extensions {
  char *ext;
  char *term_flg, *CSS_name, *web_fg, *web_bg, *web_extattr;
  struct extensions *nxt;
} *ext = NULL;
/* Hacked in DIR_COLORS support ---------------------------------------------- */

/* Function prototypes: */
int color(u_short, char *, char, char), cmd(char *), patmatch(char *, char *);
int alnumsort(struct _info **, struct _info **);
int timesort(struct _info **, struct _info **);
int dirsfirstsort(struct _info **, struct _info **);
int findino(ino_t, dev_t);
void *xmalloc(size_t), *xrealloc(void *, size_t);
void listdir(char *, int *, int *, u_long, dev_t), usage(int);
void parse_dir_colors(), printit(char *), free_dir(struct _info **), indent();
void saveino(ino_t, dev_t);
char **split(char *), **parse(char *);
char *gidtoname(int), *uidtoname(int), *do_date(time_t), *prot(u_short);
char *gnu_getcwd();
struct _info **read_dir(char *, int *);

/* Globals */
int dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
int qflag, Nflag, Dflag, inodeflag, devflag;
int noindent, force_color, nocolor, xdev, noreport, nolinks;
char *pattern = NULL, *ipattern = NULL, *host = NULL, *sp = " ";

int (*cmpfunc)() = alnumsort;

u_char Hflag, Rflag;
int Level;
char *sLevel, *curdir, *outfilename = NULL;
FILE *outfile;

int dirs[MAXDIRLEVEL];


int main(int argc, char **argv)
{
  char **dirname = NULL;
  int i,j,n,p,q,dtotal,ftotal,colored = FALSE;
  struct stat st;

  q = p = dtotal = ftotal = 0;
  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag = FALSE;
  Dflag = qflag = Nflag = Hflag = Rflag = FALSE;
  inodeflag = devflag = FALSE;
  Level = -1;
  noindent = force_color = nocolor = xdev = noreport = nolinks = FALSE;

  setlocale(LC_CTYPE, "");

  for(i = 0;i < 256; i++) {
    utable[i] = NULL;
    gtable[i] = NULL;
    itable[i] = NULL;
  }

  for(n=i=1;i<argc;i=n) {
    n++;
    if (argv[i][0] == '-' && argv[i][1]) {
      for(j=1;argv[i][j];j++) {
	switch(argv[i][j]) {
	case 'N':
	  Nflag = TRUE;
	  break;
	case 'q':
	  qflag = TRUE;
	  break;
	case 'd':
	  dflag = TRUE;
	  break;
	case 'l':
	  lflag = TRUE;
	  break;
	case 's':
	  sflag = TRUE;
	  break;
	case 'u':
	  uflag = TRUE;
	  break;
	case 'g':
	  gflag = TRUE;
	  break;
	case 'f':
	  fflag = TRUE;
	  break;
	case 'F':
	  Fflag = TRUE;
	  break;
	case 'a':
	  aflag = TRUE;
	  break;
	case 'p':
	  pflag = TRUE;
	  break;
	case 'i':
	  noindent = TRUE;
	  break;
	case 'C':
	  force_color = TRUE;
	  break;
	case 'n':
	  nocolor = TRUE;
	  break;
	case 'x':
	  xdev = TRUE;
	  break;
	case 'P':
	  pattern = argv[n++];
	  break;
	case 'I':
	  ipattern = argv[n++];
	  break;
	case 'A':
	  ansilines = TRUE;
	  break;
	case 'S':
	  asciilines = TRUE;
	  break;
	case 'D':
	  Dflag = TRUE;
	  break;
	case 't':
	  cmpfunc = timesort;
	  break;
	case 'H':
	  Hflag = TRUE;
	  host = argv[n++];
	  sp = "&nbsp;";
	  break;
	case 'R':
	  Rflag = TRUE;
	  break;
	case 'L':
	  if ((sLevel = argv[n++]) == NULL) {
	    fprintf(stderr,"tree: Missing argument to -L option.\n");
	    exit(1);
	  }
	  Level = strtoul(sLevel,NULL,0)-1;
	  if (Level < 0) {
	    fprintf(stderr,"tree: Invalid level, must be greater than 0.\n");
	    exit(1);
	  }
	  break;
	case 'o':
	  outfilename = argv[n++];
	  break;
	case '-':
	  if (j == 1) {
	    if (!strcmp("--help",argv[i])) usage(2);
	    if (!strcmp("--version",argv[i])) {
	      char *v = version+12;
	      printf("%.*s\n",(int)strlen(v)-1,v);
	      exit(0);
	    }
	    if (!strcmp("--inodes",argv[i])) {
	      j = strlen(argv[i])-1;
	      inodeflag=TRUE;
	      break;
	    }
	    if (!strcmp("--device",argv[i])) {
	      j = strlen(argv[i])-1;
	      devflag=TRUE;
	      break;
	    }
	    if (!strcmp("--noreport",argv[i])) {
	      j = strlen(argv[i])-1;
	      noreport = TRUE;
	      break;
	    }
	    if (!strcmp("--nolinks",argv[i])) {
	      j = strlen(argv[i])-1;
	      nolinks = TRUE;
	      break;
	    }
	    if (!strcmp("--dirsfirst",argv[i])) {
	      j = strlen(argv[i])-1;
	      cmpfunc = dirsfirstsort;
	      break;
	    }
	  }
	default:
	  fprintf(stderr,"tree: Invalid argument -`%c'.\n",argv[i][j]);
	  usage(1);
	  exit(1);
	  break;
	}
      }
    } else {
      if (!dirname) dirname = (char **)xmalloc(sizeof(char *) * (q=MINIT));
      else if (p == (q-2)) dirname = (char **)xrealloc(dirname,sizeof(char *) * (q+=MINC));
      dirname[p++] = scopy(argv[i]);
    }
  }
  if (p) dirname[p] = NULL;

  if (outfilename == NULL) outfile = stdout;
  else {
    outfile = fopen(outfilename,"w");
    if (outfile == NULL) {
      fprintf(stderr,"tree: invalid filename '%s'\n",outfilename);
      exit(1);
    }
  }

  parse_dir_colors();

  if (Rflag && (Level == -1))
    Rflag = FALSE;

  if (Hflag) {
    fprintf(outfile,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n");
    fprintf(outfile,"<HTML>\n");
    fprintf(outfile,"<HEAD>\n");
    fprintf(outfile,"\t<TITLE>Tree Output</TITLE>\n");
    fprintf(outfile,"\t<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n");
    fprintf(outfile,"\t<META NAME=\"Author\" CONTENT=\"Made by 'tree'\">\n");
    fprintf(outfile,"\t<META NAME=\"GENERATOR\" CONTENT=\"Linux\">\n");
    fprintf(outfile,"\t\t<STYLE type=\"text/css\">\n\t\t<!-- \n");
    fprintf(outfile,"\t\t  BODY { font-family : courier, arial, sans-serif; }\n");
    fprintf(outfile,"\t\t  .VERSION { font-family : arial, sans-serif; }\n");
    if (force_color && !nolinks) {
      fprintf(outfile,"\t\t  A.NORM:link { color: black; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.FIFO:link { color: purple; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.CHAR:link { color: yellow; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.DIR:link  { color: blue; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.BLOCK:link { color: yellow; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.LINK:link { color: aqua; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.SOCK:link { color: fuchsia; background-color: transparent;}\n");
      fprintf(outfile,"\t\t  A.EXEC:link { color: green; background-color: transparent;}\n");
    }
    fprintf(outfile,"\t\t-->\n\t\t</STYLE>\n");
    fprintf(outfile,"</HEAD>\n");
    fprintf(outfile,"<BODY>\n");
    fprintf(outfile,"<H1>Directory Tree</H1>\n<P>");
    fflag = FALSE;
    if (nolinks) {
      if (force_color) fprintf(outfile, "<font color=black>%s</font>\n",host);
      else fprintf(outfile,"%s\n",host);
    } else {
      if (force_color) fprintf(outfile,"<A CLASS=\"NORM\" HREF=\"%s\">%s</A>\n",host,host);
      else fprintf(outfile,"<A HREF=\"%s\">%s</A>\n",host,host);
    }
    curdir = gnu_getcwd();
  }

  if (dirname) {
    for(colored=i=0;dirname[i];i++,colored=0) {
      if ((n = lstat(dirname[i],&st)) >= 0) {
	saveino(st.st_ino, st.st_dev);
	if (colorize) colored = color(st.st_mode,dirname[i],n<0,FALSE);
      }
      if (!Hflag) printit(dirname[i]);
      if (colored) fprintf(outfile,"%s",endcode);
      if (!Hflag) listdir(dirname[i],&dtotal,&ftotal,0,0);
      else {
	if (chdir(dirname[i])) {
	  fprintf(outfile,"%s [error opening dir]\n",dirname[i]);
	  exit(1);
	} else {
	  listdir(".",&dtotal,&ftotal,0,0);
	  chdir(curdir);
	}
      }
    }
  } else {
    if ((n = lstat(".",&st)) >= 0) {
      saveino(st.st_ino, st.st_dev);
      if (colorize) colored = color(st.st_mode,".",n<0,FALSE);
    }
    if (!Hflag) fprintf(outfile,".");
    if (colored) fprintf(outfile,"%s",endcode);
    listdir(".",&dtotal,&ftotal,0,0);
  }

  if (Hflag)
    fprintf(outfile,"<BR><BR>");

  if (!noreport) {
    if (dflag)
      fprintf(outfile,"\n%d director%s\n",dtotal,(dtotal==1? "y":"ies"));
    else
      fprintf(outfile,"\n%d director%s, %d file%s\n",dtotal,(dtotal==1? "y":"ies"),ftotal,(ftotal==1? "":"s"));
  }

  if (Hflag) {
    fprintf(outfile,"<BR><BR>\n");
    fprintf(outfile,"<HR>\n");
    fprintf(outfile,"<P CLASS=\"VERSION\">\n");
    fprintf(outfile,"%s\n",hversion);
    fprintf(outfile,"</P>\n");
    fprintf(outfile,"</BODY>\n");
    fprintf(outfile,"</HTML>\n");
  }

  if (outfilename != NULL) fclose(outfile);

  return 0;
}

void usage(int n)
{
  switch(n) {
    case 1:
      fprintf(stderr,"usage: tree [-adfgilnpqstuxACDFNS] [-H baseHREF] [-L level [-R]] [-P pattern]\n\t[-I pattern] [-o filename] [--version] [--help] [--inodes] [--device]\n\t[--noreport] [--nolinks] [--dirsfirst] [<directory list>]\n");
      break;
    case 2:
      fprintf(stderr,"usage: tree [-adfgilnpqstuxACDFNS] [-H baseHREF] [-L level [-R]] [-P pattern]\n\t[-I pattern] [-o filename] [--version] [--help] [--inodes] [--device]\n\t[--noreport] [--nolinks] [--dirsfirst] [<directory list>]\n");
      fprintf(stderr,"    -a          All files are listed.\n");
      fprintf(stderr,"    -d          List directories only.\n");
      fprintf(stderr,"    -l          Follow symbolic links like directories.\n");
      fprintf(stderr,"    -f          Print the full path prefix for each file.\n");
      fprintf(stderr,"    -i          Don't print indentation lines.\n");
      fprintf(stderr,"    -q          Print non-printable characters as '?'.\n");
      fprintf(stderr,"    -N          Print non-printable characters as is.\n");
      fprintf(stderr,"    -p          Print the protections for each file.\n");
      fprintf(stderr,"    -u          Displays file owner or UID number.\n");
      fprintf(stderr,"    -g          Displays file group owner or GID number.\n");
      fprintf(stderr,"    -s          Print the size in bytes of each file.\n");
      fprintf(stderr,"    -D          Print the date of last modification.\n");
      fprintf(stderr,"    -F          Appends '/', '=', '*', or '|' as per ls -F.\n");
      fprintf(stderr,"    -t          Sort files by last modification time.\n");
      fprintf(stderr,"    -x          Stay on current filesystem only.\n");
      fprintf(stderr,"    -L level    Descend only level directories deep.\n");
      fprintf(stderr,"    -A          Print ANSI lines graphic indentation lines.\n");
      fprintf(stderr,"    -S          Print with ASCII graphics identation lines.\n");
      fprintf(stderr,"    -n          Turn colorization off always (-C overrides).\n");
      fprintf(stderr,"    -C          Turn colorization on always.\n");
      fprintf(stderr,"    -P pattern  List only those files that match the pattern given.\n");
      fprintf(stderr,"    -I pattern  Do not list files that match the given pattern.\n");
      fprintf(stderr,"    -H baseHREF Prints out HTML format with baseHREF as top directory.\n");
      fprintf(stderr,"    -R          Rerun tree when max dir level reached.\n");
      fprintf(stderr,"    -o file     Output to file instead of stdout.\n");
      fprintf(stderr,"    --inodes    Print inode number of each file.\n");
      fprintf(stderr,"    --device    Print device ID number to which each file belongs.\n");
      fprintf(stderr,"    --noreport  Turn off file/directory count at end of tree listing.\n");
      fprintf(stderr,"    --nolinks   Turn off hyperlinks in HTML output.\n");
      fprintf(stderr,"    --dirsfirst List directories before files.\n");
  }
  exit(0);
}


void listdir(char *d, int *dt, int *ft, u_long lev, dev_t dev)
{
  char *path, nlf = FALSE, colored = FALSE;
  long pathsize = 0;
  struct _info **dir, **sav;
  struct stat sb;
  int n,m,e;
  char hclr[20], *hdir, *hcmd;

  path=malloc(pathsize=4096);

  if ((Level >= 0) && (lev > Level)) {
    if (!Hflag) fprintf(outfile,"\n");
    return;
  }

  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }

  sav = dir = read_dir(d,&n);
  if (!dir && n) {
    fprintf(outfile," [error opening dir]\n");
    free(path);
    return;
  }
  if (!n) {
    fprintf(outfile,"\n");
    free(path);
    free_dir(sav);
    return;
  }
  qsort(dir,n,sizeof(struct _info *),cmpfunc);
  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");
  while(*dir) {
    if (!noindent) indent();

    path[0] = 0;
    if (inodeflag) sprintf(path," %7ld",(*dir)->inode);
    if (devflag) sprintf(path+strlen(path), " %3d", (int)(*dir)->dev);
    if (pflag) sprintf(path+strlen(path), " %s", prot((*dir)->mode));
    if (uflag) sprintf(path+strlen(path), " %-8.8s", uidtoname((*dir)->uid));
    if (gflag) sprintf(path+strlen(path), " %-8.8s", gidtoname((*dir)->gid));
    if (sflag) sprintf(path+strlen(path), " %9ld", (*dir)->size);
    if (Dflag) sprintf(path+strlen(path), " %s", do_date((*dir)->mtime));
    if (path[0] == ' ') {
      path[0] = '[';
      if (Hflag) {
	int i;
	for(i=0;path[i];i++) {
	  if (path[i] == ' ') fprintf(outfile,"%s",sp);
	  else fprintf(outfile,"%c", path[i]);
	}
	fprintf(outfile,"]%s%s", sp, sp);
      } else fprintf(outfile, "%s]  ",path);
    }

    if (colorize)
      colored = color((*dir)->mode,(*dir)->name,(*dir)->orphan,FALSE);

    if (fflag) {
      if (sizeof(char) * (strlen(d)+strlen((*dir)->name)+2) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen(d)+strlen((*dir)->name)+1024)));
      sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",(*dir)->name);
    }

    if (Hflag) {
      if (Rflag && (lev == Level) && (*dir)->isdir) {
	if (nolinks) fprintf(outfile,"%s",(*dir)->name);
	else fprintf(outfile,"<A HREF=\"%s%s/%s/00Tree.html\">%s</A>",host,d+1,(*dir)->name,(*dir)->name);

	hdir = gnu_getcwd();
	if (sizeof(char) * (strlen(hdir)+strlen(d)+strlen((*dir)->name)+2) > pathsize)
	  path = xrealloc(path, pathsize = sizeof(char) * (strlen(hdir)+strlen(d)+strlen((*dir)->name) + 1024));

	sprintf(path,"%s%s/%s",hdir,d+1,(*dir)->name);
	fprintf(stderr,"Entering directory %s\n",path);

	hcmd = xmalloc(sizeof(char) * (49 + strlen(host) + strlen(d) + strlen((*dir)->name)) + 10 + (2*strlen(path)));
	sprintf(hcmd,"tree -n -H %s%s/%s -L %d -R -o %s/00Tree.html %s\n", host,d+1,(*dir)->name,Level+1,path,path);
	system(hcmd);
	free(hdir);
	free(hcmd);
      } else {
	if (nolinks) {
	  if (force_color)
	    fprintf(outfile, "<font color=%s>%s</font>",
		    (*dir)->isdir  ? "blue"  :
		    (*dir)->isexe  ? "green" :
		    (*dir)->isfifo ? "purple":
		    (*dir)->issok  ? "fuchsia" : "black", (*dir)->name);
	  else
	    fprintf(outfile,"%s%s",(*dir)->name,(*dir)->isdir? "/" : "");
	} else {
	  if (force_color) {
	    sprintf(hclr, "%s",
		    (*dir)->isdir ?  "DIR"  :
		    (*dir)->isexe ?  "EXEC" :
		    (*dir)->isfifo ? "FIFO" :
		    (*dir)->issok ?  "SOCK" : "NORM");
	    fprintf(outfile,"<A CLASS=\"%s\" HREF=\"%s%s/%s%s\">%s%s</A>", hclr, host,d+1,(*dir)->name,
		    ((*dir)->isdir?"/":""),(*dir)->name,((*dir)->isdir?"/":""));
	  } else fprintf(outfile,"<A HREF=\"%s%s/%s%s\">%s%s</A>",host, d+1, (*dir)->name,
			 (*dir)->isdir? "/" : "", (*dir)->name, (*dir)->isdir? "/" : "" );
	  }
      }
    } else printit(path);

    if (colored) fprintf(outfile,"%s",endcode);
    
    if (Fflag && !(*dir)->lnk) {
      if (!dflag && (*dir)->isdir) fprintf(outfile,"/");
      else if ((*dir)->issok) fprintf(outfile,"=");
      else if ((*dir)->isfifo) fprintf(outfile,"|");
      else if (!(*dir)->isdir && (*dir)->isexe) fprintf(outfile,"*");
    }

    if ((*dir)->lnk && !Hflag) {
      fprintf(outfile,"%s->%s",sp,sp);
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,TRUE);
      printit((*dir)->lnk);
      if (colored) fprintf(outfile,"%s",endcode);
      if (Fflag) {
	m = (*dir)->lnkmode & S_IFMT;
	e = ((*dir)->lnkmode & S_IEXEC) | ((*dir)->lnkmode & (S_IEXEC>>3)) | ((*dir)->lnkmode & (S_IEXEC>>6));
	if (!dflag && m == S_IFDIR) fprintf(outfile,"/");
	else if (m == S_IFSOCK) fprintf(outfile,"=");
	else if (m == S_IFIFO) fprintf(outfile,"|");
	else if (!(m == S_IFDIR) && e) fprintf(outfile,"*");
      }
    }

    if ((*dir)->isdir) {
      if ((*dir)->lnk) {
	if (lflag && !(xdev && dev != (*dir)->dev)) {
	  if (findino((*dir)->inode,(*dir)->dev)) {
	    fprintf(outfile,"  [recursive, not followed]");
	  } else {
	    saveino((*dir)->inode, (*dir)->dev);
	    if (*(*dir)->lnk == '/')
	      listdir((*dir)->lnk,dt,ft,lev+1,dev);
	    else {
	      if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	      sprintf(path,"%s/%s",d,(*dir)->lnk);
	      listdir(path,dt,ft,lev+1,dev);
	    }
	    nlf = TRUE;
	  }
	}
      } else if (!(xdev && dev != (*dir)->dev)) {
	if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	sprintf(path,"%s/%s",d,(*dir)->name);
	saveino((*dir)->inode, (*dir)->dev);
	listdir(path,dt,ft,lev+1,dev);
	nlf = TRUE;
      }
      *dt += 1;
    } else *ft += 1;
    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;
    if (nlf) nlf = FALSE;
    else fprintf(outfile,"\n");
    dir++;
  }
  dirs[lev] = 0;
  free(path);
  free_dir(sav);
}


struct _info **read_dir(char *dir, int *n)
{
  static char *path = NULL, *lbuf = NULL;
  static long pathsize = PATH_MAX+1, lbufsize = PATH_MAX+1;
  struct _info **dl;
  struct dirent *ent;
  struct stat lst,st;
  DIR *d;
  int ne, p = 0, len, rs;

  if (path == NULL) {
    path=xmalloc(pathsize);
    lbuf=xmalloc(lbufsize);
  }

  *n = 1;
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)xmalloc(sizeof(struct _info *) * (ne = MINIT));

  while((ent = (struct dirent *)readdir(d))) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (!Hflag && !strcmp(ent->d_name,"00Tree.html")) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    if (strlen(dir)+strlen(ent->d_name)+2 > pathsize) path = xrealloc(path,pathsize=(strlen(dir)+strlen(ent->d_name)+4096));
    sprintf(path,"%s/%s",dir,ent->d_name);
    if (lstat(path,&lst) < 0) continue;
    if ((rs = stat(path,&st)) < 0) st.st_mode = 0;

    if ((lst.st_mode & S_IFMT) != S_IFDIR && !(((st.st_mode & S_IFMT) == S_IFLNK) && lflag)) {
      if (pattern && patmatch(ent->d_name,pattern) != 1) continue;
    }
    if (ipattern && patmatch(ent->d_name,ipattern) == 1) continue;

    if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) continue;
    if (pattern && ((st.st_mode & S_IFMT) == S_IFLNK) && !lflag) continue;


    if (p == (ne-1)) dl = (struct _info **)xrealloc(dl,sizeof(struct _info *) * (ne += MINC));
    dl[p] = (struct _info *)xmalloc(sizeof(struct _info));

    dl[p]->name = scopy(ent->d_name);
    dl[p]->mode = lst.st_mode;
    dl[p]->uid = lst.st_uid;
    dl[p]->gid = lst.st_gid;
    dl[p]->size = lst.st_size;
    dl[p]->dev = st.st_dev;
    dl[p]->inode = st.st_ino;
    dl[p]->lnk = NULL;
    dl[p]->orphan = FALSE;

    dl[p]->atime = lst.st_atime;
    dl[p]->ctime = lst.st_ctime;
    dl[p]->mtime = lst.st_mtime;

    if ((lst.st_mode & S_IFMT) == S_IFLNK) {
      if (lst.st_size+1 > lbufsize) lbuf = xrealloc(lbuf,lbufsize=(lst.st_size+8192));
      if ((len=readlink(path,lbuf,lbufsize-1)) < 0) {
	dl[p]->lnk = scopy("[Error reading symbolic link information]");
	dl[p]->isdir = dl[p]->issok = dl[p]->isexe = FALSE;
	dl[p++]->lnkmode = st.st_mode;
	continue;
      } else {
	lbuf[len] = 0;
	dl[p]->lnk = scopy(lbuf);
	if (rs < 0) dl[p]->orphan = TRUE;
	dl[p]->lnkmode = st.st_mode;
      }
    }
    
    dl[p]->isdir = ((st.st_mode & S_IFMT) == S_IFDIR);
    dl[p]->issok = ((st.st_mode & S_IFMT) == S_IFSOCK);
    dl[p]->isfifo = ((st.st_mode & S_IFMT) == S_IFIFO);
    dl[p++]->isexe = (st.st_mode & S_IEXEC) | (st.st_mode & (S_IEXEC>>3)) | (st.st_mode & (S_IEXEC>>6));
  }
  closedir(d);
  *n = p;
  dl[p] = NULL;
  return dl;
}

/* Sorting functions */
int alnumsort(struct _info **a, struct _info **b)
{
  return strcmp((*a)->name,(*b)->name);
}

int timesort(struct _info **a, struct _info **b)
{
  return (*a)->mtime < (*b)->mtime;
}

int dirsfirstsort(struct _info **a, struct _info **b)
{
  if ((*a)->isdir == (*b)->isdir) return strcmp((*a)->name,(*b)->name);
  else return (*a)->isdir ? -1 : 1;
}

/** Necessary only on systems without glibc **/
void *xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

void *xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr,size);
  if (value == 0) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

char *gnu_getcwd()
{
  int size = 100;
  char *buffer = (char *) xmalloc (size);
     
  while (1)
    {
      char *value = getcwd (buffer, size);
      if (value != 0)
	return buffer;
      size *= 2;
      free (buffer);
      buffer = (char *) xmalloc (size);
    }
}

/*
 * Patmatch() code courtesy of Thomas Moore (dark@mama.indstate.edu)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
int patmatch(char *buf, char *pat)
{
  int match = 1,m,n;

  while(*pat && match) {
    switch(*pat) {
    case '[':
      pat++;
      if(*pat != '^') {
	n = 1;
	match = 0;
      } else {
	pat++;
	n = 0;
      }
      while(*pat != ']'){
	if(*pat == '\\') pat++;
	if(!*pat /* || *pat == '/' */ ) return -1;
	if(pat[1] == '-'){
	  m = *pat;
	  pat += 2;
	  if(*pat == '\\' && *pat)
	    pat++;
	  if(*buf >= m && *buf <= *pat)
	    match = n;
	  if(!*pat)
	    pat--;
	} else if(*buf == *pat) match = n;
	pat++;
      }
      buf++;
      break;
    case '*':
      pat++;
      if(!*pat) return 1;
      while(*buf && !(match = patmatch(buf++,pat)));
      return match;
    case '?':
      if(!*buf) return 0;
      buf++;
      break;
    case '\\':
      if(*pat)
	pat++;
    default:
      match = (*buf++ == *pat);
      break;
    }
    pat++;
    if(match<1) return match;
  }
  if(!*buf) return match;
  return 0;
}


/*
 * They cried out for ANSI-lines (not really), but here they are, as an option
 * for the xterm and console capable among you, as a run-time option.
 */
void indent()
{
  int i;

  if (ansilines) {
    if (dirs[0]) fprintf(outfile,"\033(0");
    for(i=0;dirs[i];i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) fprintf(outfile,"\170   ");
	else printf("    ");
      } else {
	if (dirs[i] == 1) fprintf(outfile,"\164\161\161 ");
	else fprintf(outfile,"\155\161\161 ");
      }
    }
    if (dirs[0]) fprintf(outfile,"\033(B");
  } else if (asciilines) {
    for(i=0;dirs[i];i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) printf("\263  ");
	else printf("    ");
      } else {
	if (dirs[i] == 1) printf("\303\304\304");
	else printf("\300\304\304");
      }
    }
  } else {
    if (Hflag) fprintf(outfile,"<BR>%s%s%s",sp,sp,sp);
    for(i=0;dirs[i];i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) fprintf(outfile,"|%s%s ",sp,sp);
	else fprintf(outfile,"%s%s%s ",sp,sp,sp);
      }	else {
	if (dirs[i] == 1) fprintf(outfile,"|-- ");
	else
	  if (Hflag) fprintf(outfile,"&middot;-- ");
	  else fprintf(outfile,"`-- ");
      }
    }
  }
}

void free_dir(struct _info **d)
{
  int i;

  for(i=0;d[i];i++) {
    free(d[i]->name);
    if (d[i]->lnk) free(d[i]->lnk);
    free(d[i]);
  }
  free(d);
}

char *prot(u_short m)
{
  static char buf[11];
  static u_short ifmt[] = {S_IFIFO, S_IFCHR, S_IFDIR, S_IFBLK, S_IFREG, S_IFLNK, S_IFSOCK, 0};
  static char fmt[] = {'p','c','d','b','-','l','s','?'};
  int i;

  for(i=0;ifmt[i] && (m&S_IFMT) != ifmt[i];i++);
  buf[0] = fmt[i];

  buf[1] = (m & S_IREAD)? 'r' : '-';
  buf[2] = (m & S_IWRITE)? 'w' : '-';
  buf[3] = (m & S_IEXEC)? 'x' : '-';
  if (m & S_ISUID) buf[3] = (buf[3]=='-')? 'S' : 's';

  buf[4] = (m & (S_IREAD>>3))? 'r' : '-';
  buf[5] = (m & (S_IWRITE>>3))? 'w' : '-';
  buf[6] = (m & (S_IEXEC>>3))? 'x' : '-';
  if (m & S_ISGID) buf[6] = (buf[6]=='-')? 'S' : 's';

  buf[7] = (m & (S_IREAD>>6))? 'r' : '-';
  buf[8] = (m & (S_IWRITE>>6))? 'w' : '-';
  buf[9] = (m & (S_IEXEC>>6))? 'x' : '-';
  if (m & S_ISVTX) buf[9] = (buf[9]=='-')? 'T' : 't';

  buf[10] = 0;
  return buf;
}

static char *month[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define SIXMONTHS (6*31*24*60*60)

char *do_date(time_t t)
{
  time_t c = time(0);
  struct tm *tm;
  static char buf[30];

  if (t > c) return " The Future ";

  tm = localtime(&t);

  sprintf(buf,"%s %2d",month[tm->tm_mon],tm->tm_mday);
  if (t+SIXMONTHS < c) sprintf(buf+6,"  %4d",1900+tm->tm_year);
  else sprintf(buf+6," %2d:%02d",tm->tm_hour,tm->tm_min);
  return buf;
}

char *uidtoname(int uid)
{
  struct xtable *o, *p, *t;
  struct passwd *ent;
  char ubuf[6];
  int uent = HASH(uid);

  for(o = p = utable[uent]; p ; p=p->nxt) {
    if (uid == p->xid) return p->name;
    else if (uid < p->xid) break;
    o = p;
  }
  /* uh oh, time to do a real lookup */
  t = xmalloc(sizeof(struct xtable));
  if ((ent = getpwuid(uid)) != NULL) t->name = scopy(ent->pw_name);
  else {
    sprintf(ubuf,"%d",uid);
    t->name = scopy(ubuf);
  }
  t->xid = uid;
  t->nxt = p;
  if (p == utable[uent]) utable[uent] = t;
  else o->nxt = t;
  return t->name;
}

char *gidtoname(int gid)
{
  struct xtable *o, *p, *t;
  struct group *ent;
  char gbuf[6];
  int gent = HASH(gid);

  for(o = p = gtable[gent]; p ; p=p->nxt) {
    if (gid == p->xid) return p->name;
    else if (gid < p->xid) break;
    o = p;
  }
  /* uh oh, time to do a real lookup */
  t = xmalloc(sizeof(struct xtable));
  if ((ent = getgrgid(gid)) != NULL) t->name = scopy(ent->gr_name);
  else {
    sprintf(gbuf,"%d",gid);
    t->name = scopy(gbuf);
  }
  t->xid = gid;
  t->nxt = p;
  if (p == gtable[gent]) gtable[gent] = t;
  else o->nxt = t;
  return t->name;
}

void printit(char *s)
{
  char c;

  if (Nflag) fprintf(outfile,"%s",s);
  else {
    for(;*s;s++)
      if (isprint(*s)) fprintf(outfile,"%c",*s);
      else {
	if (qflag) fprintf(outfile,"?");
	else {
	  c = *s & 127;
	  if (c < ' ') fprintf(outfile,"^%c",c+'@');
	  else if (c == 127) fprintf(outfile,"^?");
	}
      }
  }
}

void saveino(ino_t inode, dev_t device)
{
  struct inotable *it, *ip, *pp;
  int hp = inohash(inode);

  for(pp = ip = itable[hp];ip;ip = ip->nxt) {
    if (ip->inode > inode) break;
    if (ip->inode == inode && ip->device >= device) break;
    pp = ip;
  }

  if (ip && ip->inode == inode && ip->device == device) return;

  it = xmalloc(sizeof(struct inotable));
  it->inode = inode;
  it->device = device;
  it->nxt = ip;
  if (ip == itable[hp]) itable[hp] = it;
  else pp->nxt = it;
}

int findino(ino_t inode, dev_t device)
{
  struct inotable *it;

  for(it=itable[inohash(inode)]; it; it=it->nxt) {
    if (it->inode > inode) break;
    if (it->inode == inode && it->device >= device) break;
  }

  if (it && it->inode == inode && it->device == device) return TRUE;
  return FALSE;
}

/*
 * Hacked in DIR_COLORS support for linux. ------------------------------
 */
/*
 *  If someone asked me, I'd extend dircolors, to provide more generic
 * color support so that more programs could take advantage of it.  This
 * is really just hacked in support.  The dircolors program should:
 * 1) Put the valid terms in a environment var, like:
 *    COLOR_TERMS=linux:console:xterm:vt100...
 * 2) Put the COLOR and OPTIONS directives in a env var too.
 * 3) Have an option to dircolors to silently ignore directives that it
 *    doesn't understand (directives that other programs would
 *    understand).
 * 4) Perhaps even make those unknown directives environment variables.
 *
 * The environment is the place for cryptic crap no one looks at, but
 * programs.  No one is going to care if it takes 30 variables to do
 * something.
 */
void parse_dir_colors()
{
  char buf[1025], **arg, *colors;
  char *s, **c;
  int i;
  struct extensions *e;

  if (Hflag) return;

  if (getenv("TERM") == NULL) {
    colorize = FALSE;
    return;
  }

  s = getenv("LS_COLORS");
  if (s == NULL && force_color) s = ":no=00:fi=00:di=01;34:ln=01;36:pi=40;33:so=01;35:bd=40;33;01:cd=40;33;01:ex=01;32:*.cmd=01;32:*.exe=01;32:*.com=01;32:*.btm=01;32:*.bat=01;32:*.tar=01;31:*.tgz=01;31:*.arj=01;31:*.taz=01;31:*.lzh=01;31:*.zip=01;31:*.z=01;31:*.Z=01;31:*.gz=01;31:*.jpg=01;35:*.gif=01;35:*.bmp=01;35:*.xbm=01;35:*.xpm=01;35:*.tif=01;35:";

  if (s == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = FALSE;
    return;
  } else {
    colorize = TRUE;
    /* You can uncomment the below line and tree will always try to ANSI-fy the indentation lines */
    /*    ansilines = TRUE; */
  }

  colors = scopy(s);

  arg = parse(colors);

  for(i=0;arg[i];i++) {
    c = split(arg[i]);
    switch(cmd(c[0])) {
    case COL_NORMAL:
      if (c[1]) norm_flgs = scopy(c[1]);
      break;
    case COL_FILE:
      if (c[1]) file_flgs = scopy(c[1]);
      break;
    case COL_DIR:
      if (c[1]) dir_flgs = scopy(c[1]);
      break;
    case COL_LINK:
      if (c[1]) link_flgs = scopy(c[1]);
      break;
    case COL_FIFO:
      if (c[1]) fifo_flgs = scopy(c[1]);
      break;
    case COL_SOCK:
      if (c[1]) sock_flgs = scopy(c[1]);
      break;
    case COL_BLK:
      if (c[1]) block_flgs = scopy(c[1]);
      break;
    case COL_CHR:
      if (c[1]) char_flgs = scopy(c[1]);
      break;
    case COL_EXEC:
      if (c[1]) exec_flgs = scopy(c[1]);
      break;
    case COL_ORPHAN:
      if (c[1]) orphan_flgs = scopy(c[1]);
      break;
    case COL_MISSING:
      if (c[1]) missing_flgs = scopy(c[1]);
      break;
    case COL_LEFTCODE:
      if (c[1]) leftcode = scopy(c[1]);
      break;
    case COL_RIGHTCODE:
      if (c[1]) rightcode = scopy(c[1]);
      break;
    case COL_ENDCODE:
      if (c[1]) endcode = scopy(c[1]);
      break;
    case DOT_EXTENSION:
      if (c[1]) {
	e = xmalloc(sizeof(struct extensions));
	e->ext = scopy(c[0]+1);
	e->term_flg = scopy(c[1]);
	e->nxt = ext;
	ext = e;
      }
    default:
    }
  }

  /* make sure at least norm_flgs is defined.  We're going to assume vt100 support */
  if (!leftcode) leftcode = scopy("\033[");
  if (!rightcode) rightcode = scopy("m");
  if (!norm_flgs) norm_flgs = scopy("00");

  if (!endcode) {
    sprintf(buf,"%s%s%s",leftcode,norm_flgs,rightcode);
    endcode = scopy(buf);
  }

  free(colors);

  /*  if (!termmatch) colorize = FALSE; */
}

char **parse(char *b)
{
  static int n = 1000;
  static char **arg = NULL;
  int i;

  if (!arg) arg = malloc(sizeof(char *) * n);
  for(i=0;*b;b++) {
    while(*b && *b == ':') b++;
    if (i == (n-1)) arg = realloc(arg,sizeof(char *) * (n+=500));
    arg[i++] = b;
    while(*b && *b != ':') b++;
    if (!*b) break;
    *b = 0;
  }
  if (i == (n-1)) arg = realloc(arg,sizeof(char *) * (n+=50));
  arg[i] = NULL;
  return arg;
}

char **split(char *b)
{
  static char *arg[2];

  arg[0] = b;
  while(*b != '=') b++;
  *b = 0;
  b++;
  arg[1] = b;

  return arg;
}

int cmd(char *s)
{
  static struct {
    char *cmd;
    char cmdnum;
  } cmds[] = {
    {"no", COL_NORMAL}, {"fi", COL_FILE}, {"di", COL_DIR}, {"ln", COL_LINK}, {"pi", COL_FIFO},
    {"so", COL_SOCK}, {"bd", COL_BLK}, {"cd", COL_CHR}, {"ex", COL_EXEC}, {"mi", COL_MISSING},
    {"or", COL_ORPHAN}, {"lc", COL_LEFTCODE}, {"rc", COL_RIGHTCODE}, {"ec", COL_ENDCODE},
    {NULL, 0}
  };
  int i;

  for(i=0;cmds[i].cmdnum;i++)
    if (!strcmp(cmds[i].cmd,s)) return cmds[i].cmdnum;
  if (s[0] == '*') return DOT_EXTENSION;
  return ERROR;
}

int color(u_short mode, char *name, char orphan, char islink)
{
  struct extensions *e;
  int l, xl;

  if (orphan) {
    if (islink) {
      if (missing_flgs) {
	fprintf(outfile,"%s%s%s",leftcode,missing_flgs,rightcode);
	return TRUE;
      }
    } else {
      if (orphan_flgs) {
	fprintf(outfile,"%s%s%s",leftcode,orphan_flgs,rightcode);
	return TRUE;
      }
    }
  }
  l = strlen(name);
  for(e=ext;e;e=e->nxt) {
   xl = strlen(e->ext);
   if (!strcmp((l>xl)?name+(l-xl):name,e->ext)) {
      fprintf(outfile,"%s%s%s",leftcode,e->term_flg,rightcode);
      return TRUE;
    }
  }
  switch(mode & S_IFMT) {
  case S_IFIFO:
    if (!fifo_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,fifo_flgs,rightcode);
    return TRUE;
  case S_IFCHR:
    if (!char_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,char_flgs,rightcode);
    return TRUE;
  case S_IFDIR:
    if (!dir_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,dir_flgs,rightcode);
    return TRUE;
  case S_IFBLK:
    if (!block_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,block_flgs,rightcode);
    return TRUE;
  case S_IFLNK:
    if (!link_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,link_flgs,rightcode);
    return TRUE;
  case S_IFSOCK:
    if (!sock_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,sock_flgs,rightcode);
    return TRUE;
  case S_IFREG:
    if (!exec_flgs) return FALSE;
    if ((mode & S_IEXEC) | (mode & (S_IEXEC>>3)) | (mode & (S_IEXEC>>6))) {
      fprintf(outfile,"%s%s%s",leftcode,exec_flgs,rightcode);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}
