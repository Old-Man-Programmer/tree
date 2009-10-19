/* $Copyright: $
 * Copyright (c) 1996 - 2009 by Steve Baker (ice@mama.indstate.edu)
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __TANDEM
#  include <strings.h>
#  define S_IEXEC  S_IXUSR
#  define S_IREAD  S_IRUSR
#  define S_IWRITE S_IWUSR
#else
#   include <sys/file.h>
#endif
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#ifdef __EMX__  /* for OS/2 systems */
#  define INCL_DOSFILEMGR
#  define INCL_DOSNLS
#  include <os2.h>
#  include <sys/nls.h>
#  include <io.h>
  /* On many systems stat() function is idential to lstat() function.
   * But the OS/2 does not support symbolic links and doesn't have lstat() function.
   */
#  define         lstat          stat
#  define         strcasecmp     stricmp
  /* Following two functions, getcwd() and chdir() don't support for drive letters.
   * To implement support them, use _getcwd2() and _chdir2().
   */
#  define getcwd _getcwd2
#  define chdir _chdir2
#endif

#include <locale.h>
#include <wchar.h>
#include <wctype.h>

static char *version ="$Version: $ tree v1.5.3 (c) 1996 - 2009 by Steve Baker, Thomas Moore, Francesc Rocher, Kyosuke Tokoro $";
static char *hversion="\t\t\t tree v1.5.3 %s 1996 - 2009 by Steve Baker and Thomas Moore <br>\n"
		      "\t\t\t HTML output hacked and copyleft %s 1998 by Francesc Rocher <br>\n"
		      "\t\t\t Charsets / OS/2 support %s 2001 by Kyosuke Tokoro\n";


#define scopy(x)	strcpy(xmalloc(strlen(x)+1),(x))
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
  u_short mode, lnkmode;
  uid_t uid;
  gid_t gid;
  off_t size;
  time_t atime, ctime, mtime;
  dev_t dev;
  ino_t inode;
#ifdef __EMX__
  long attr;
#endif
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

char colorize = FALSE, ansilines = FALSE;
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

const struct linedraw {
  const char **name, *vert, *vert_left, *corner, *copy;
} *linedraw;

/* Hacked in DIR_COLORS support ---------------------------------------------- */

/* Function prototypes: */
int color(u_short, char *, char, char), cmd(char *), patmatch(char *, char *);
int alnumsort(struct _info **, struct _info **);
int versort(struct _info **a, struct _info **b);
int reversealnumsort(struct _info **, struct _info **);
int timesort(struct _info **, struct _info **);
int dirsfirstsort(struct _info **, struct _info **);
int findino(ino_t, dev_t);
void *xmalloc(size_t), *xrealloc(void *, size_t);
void listdir(char *, int *, int *, u_long, dev_t), usage(int);
void parse_dir_colors(), printit(char *), free_dir(struct _info **), indent(int maxlevel);
void saveino(ino_t, dev_t);
char **split(char *, char *, int *);
char *gidtoname(int), *uidtoname(int), *do_date(time_t);
char *gnu_getcwd();
struct _info **read_dir(char *, int *);
void html_encode(FILE *, char *), url_encode(FILE *, char *);
const char *getcharset(void);
void initlinedraw(int);
void psize(char *buf, off_t size);
#ifdef __EMX__
  char *prot(long);
#else
  char *prot(u_short);
#endif

/* We use the strverscmp.c file if we're not linux */
#if ! defined (LINUX)
int strverscmp (const char *s1, const char *s2);
#endif

/* Globals */
int dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
int qflag, Nflag, Dflag, inodeflag, devflag, hflag;
int noindent, force_color, nocolor, xdev, noreport, nolinks, flimit;
char *pattern = NULL, *ipattern = NULL, *host = NULL, *title = "Directory Tree", *sp = " ";
const char *charset=NULL;

int (*cmpfunc)() = alnumsort;

u_char Hflag, Rflag;
int Level;
char *sLevel, *curdir, *outfilename = NULL;
FILE *outfile;
int *dirs, maxdirs;

int mb_cur_max;


int main(int argc, char **argv)
{
  char **dirname = NULL;
  int i,j,n,p,q,dtotal,ftotal,colored = FALSE;
  struct stat st;

  q = p = dtotal = ftotal = 0;
  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag = FALSE;
  Dflag = qflag = Nflag = Hflag = Rflag = hflag = FALSE;
  noindent = force_color = nocolor = xdev = noreport = nolinks = FALSE;
  inodeflag = devflag = FALSE;
  flimit = 0;
  dirs = xmalloc(sizeof(int) * (maxdirs=4096));
  memset(dirs, 0, sizeof(int) * maxdirs);
  dirs[0] = 0;
  Level = -1;

  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");

  charset = getcharset();
  if (charset == NULL && patmatch(setlocale(LC_CTYPE,NULL), "*[Uu][Tt][Ff]-8") == 1) {
    charset = "UTF-8";
  }

/* Until I get rid of this hack, make it linux/cygwin/HP nonstop only: */
#if defined (LINUX) || defined (CYGWIN) || defined (__TANDEM)
  mb_cur_max = (int)MB_CUR_MAX;
#else
  mb_cur_max = 1;
#endif

  memset(utable,0,sizeof(utable));
  memset(gtable,0,sizeof(gtable));
  memset(itable,0,sizeof(itable));

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
	case 'h':
	  hflag = TRUE;
	  sflag = TRUE; /* Assume they also want -s */
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
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: missing argument to -P option.\n");
	    exit(1);
	  }
	  pattern = argv[n++];
	  break;
	case 'I':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: missing argument to -I option.\n");
	    exit(1);
	  }
	  ipattern = argv[n++];
	  break;
	case 'A':
	  ansilines = TRUE;
	  break;
	case 'S':
	  charset = "IBM437";
	  break;
	case 'D':
	  Dflag = TRUE;
	  break;
	case 't':
	  cmpfunc = timesort;
	  break;
	case 'r':
	  cmpfunc = reversealnumsort;
	  break;
	case 'v':
	  cmpfunc = versort;
	  break;
	case 'H':
	  Hflag = TRUE;
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: missing argument to -H option.\n");
	    exit(1);
	  }
	  host = argv[n++];
	  sp = "&nbsp;";
	  break;
	case 'T':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: missing argument to -T option.\n");
	    exit(1);
	  }
	  title = argv[n++];
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
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: missing argument to -o option.\n");
	    exit(1);
	  }
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
	    if (!strncmp("--filelimit",argv[i],11)) {
	      j = 11;
	      if (*(argv[i]+11) == '=') {
		if (*(argv[i]+12)) {
		  flimit=atoi(argv[i]+12);
		  j = strlen(argv[i])-1;
		  break;
		}
	      }
	      if (argv[n] != NULL) {
		flimit = atoi(argv[n++]);
		j = strlen(argv[i])-1;
	      } else {
		fprintf(stderr,"tree: missing argument to --filelimit\n");
		exit(1);
	      }
	      break;
	    }
	    if (!strncmp("--charset",argv[i],9)){
	      j = 9;
	      if (*(argv[i]+j) == '=') {
		if (*(charset = (argv[i]+10))) {
		  j = strlen(argv[i])-1;
		  break;
		}
	      }
	      if (argv[n] != NULL) {
		charset = argv[n++];
		j = strlen(argv[i])-1;
	      } else {
		initlinedraw(1);
		exit(1);
	      }
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

  if (outfilename == NULL) {
#ifdef __EMX__
    _fsetmode(outfile=stdout,Hflag?"b":"t");
#else
    outfile = stdout;
#endif
  } else {
#ifdef __EMX__
    outfile = fopen(outfilename,Hflag?"wb":"wt");
#else
    outfile = fopen(outfilename,"w");
#endif
    if (outfile == NULL) {
      fprintf(stderr,"tree: invalid filename '%s'\n",outfilename);
      exit(1);
    }
  }

  parse_dir_colors();
  initlinedraw(0);

  if (Rflag && (Level == -1))
    Rflag = FALSE;

  if (Hflag) {
    fprintf(outfile,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n");
    fprintf(outfile,"<html>\n");
    fprintf(outfile,"\t<head>\n");
    if (charset)
      fprintf(outfile,"\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n",charset);
    else
      fprintf(outfile,"\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n");

    fprintf(outfile,"\t\t<meta name=\"Author\" content=\"Made by 'tree'\">\n");
    fprintf(outfile,"\t\t<meta name=\"GENERATOR\" content=\"%s\">\n",version);
    fprintf(outfile,"\t\t<title>%s</title>\n",title);
    fprintf(outfile,"\t\t<style type=\"text/css\">\n\t\t\t<!-- \n");
    fprintf(outfile,"\t\t\tBODY { font-family : courier, monospace, sans-serif; }\n");
    fprintf(outfile,"\t\t\tP { font-weight: normal; font-family : courier, monospace, sans-serif; color: black; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\tB { font-weight: normal; color: black; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\tA:visited { font-weight : normal; text-decoration : none; background-color : transparent; margin : 0px 0px 0px 0px; padding : 0px 0px 0px 0px; display: inline; }\n");
    fprintf(outfile,"\t\t\tA:link    { font-weight : normal; text-decoration : none; margin : 0px 0px 0px 0px; padding : 0px 0px 0px 0px; display: inline; }\n");
    fprintf(outfile,"\t\t\tA:hover   { color : #000000; font-weight : normal; text-decoration : underline; background-color : yellow; margin : 0px 0px 0px 0px; padding : 0px 0px 0px 0px; display: inline; }\n");
    fprintf(outfile,"\t\t\tA:active  { color : #000000; font-weight: normal; background-color : transparent; margin : 0px 0px 0px 0px; padding : 0px 0px 0px 0px; display: inline; }\n");
    fprintf(outfile,"\t\t\t.VERSION { font-size: small; font-family : arial, sans-serif; }\n");

    /* We can use CSS for nolinks as well */
    fprintf(outfile,"\t\t\t.NORM { color: black; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.FIFO { color: purple; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.CHAR { color: yellow; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.DIR  { color: blue; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.BLOCK { color: yellow; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.LINK { color: aqua; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.SOCK { color: fuchsia; background-color: transparent;}\n");
    fprintf(outfile,"\t\t\t.EXEC { color: green; background-color: transparent;}\n");

    fprintf(outfile,"\t\t\t-->\n\t\t</style>\n");
    fprintf(outfile,"\t</head>\n");
    fprintf(outfile,"\t<body>\n");
    fprintf(outfile,"\t\t<h1>%s</h1>\n\t\t\t<p>",title);

    fflag = FALSE;
    if (nolinks) {
      if (force_color) fprintf(outfile, "<b class=\"NORM\">%s</b>\n",host);
      else fprintf(outfile,"%s\n",host);
    } else {
      if (force_color) fprintf(outfile,"<a class=\"NORM\" href=\"%s\">%s</a>\n",host,host);
      else fprintf(outfile,"<a href=\"%s\">%s</a>\n",host,host);
    }
    curdir = gnu_getcwd();
  }

  if (dirname) {
    for(colored=i=0;dirname[i];i++,colored=0) {
      if (fflag) {
	do {
	  j=strlen(dirname[i]);
	  if (j > 1 && dirname[i][j-1] == '/') dirname[i][--j] = 0;
	} while (j > 1 && dirname[i][j-1] == '/');
      }
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
    fprintf(outfile,"\t\t<br><br>\n\t\t</p>\n\t\t<p>\n");

  if (!noreport) {
    if (dflag)
      fprintf(outfile,"\n%d director%s\n",dtotal,(dtotal==1? "y":"ies"));
    else
      fprintf(outfile,"\n%d director%s, %d file%s\n",dtotal,(dtotal==1? "y":"ies"),ftotal,(ftotal==1? "":"s"));
  }

  if (Hflag) {
    fprintf(outfile,"\t\t<br><br>\n\t\t</p>\n");
    fprintf(outfile,"\t\t<hr>\n");
    fprintf(outfile,"\t\t<p class=\"VERSION\">\n");
    fprintf(outfile,hversion,linedraw->copy, linedraw->copy, linedraw->copy);
    fprintf(outfile,"\t\t</p>\n");
    fprintf(outfile,"\t</body>\n");
    fprintf(outfile,"</html>\n");
  }

  if (outfilename != NULL) fclose(outfile);

  return 0;
}

/*
usage: tree [-adfghilnpqrstuvxACDFNS] [-H baseHREF] [-T title ] [-L level [-R]]
	[-P pattern] [-I pattern] [-o filename] [--version] [--help] [--inodes]
	[--device] [--noreport] [--nolinks] [--dirsfirst] [--charset charset]
	[--filelimit #] [<directory list>]
*/
void usage(int n)
{
  switch(n) {
    case 1:
      fprintf(stderr,"usage: tree [-adfghilnpqrstuvxACDFNS] [-H baseHREF] [-T title ] [-L level [-R]]\n\t[-P pattern] [-I pattern] [-o filename] [--version] [--help] [--inodes]\n\t[--device] [--noreport] [--nolinks] [--dirsfirst] [--charset charset]\n\t[--filelimit #] [<directory list>]\n");
      break;
    case 2:
      fprintf(stderr,"usage: tree [-adfghilnpqrstuvxACDFNS] [-H baseHREF] [-T title ] [-L level [-R]]\n\t[-P pattern] [-I pattern] [-o filename] [--version] [--help] [--inodes]\n\t[--device] [--noreport] [--nolinks] [--dirsfirst] [--charset charset]\n\t[--filelimit #] [<directory list>]\n");
      fprintf(stderr,"  -a            All files are listed.\n");
      fprintf(stderr,"  -d            List directories only.\n");
      fprintf(stderr,"  -l            Follow symbolic links like directories.\n");
      fprintf(stderr,"  -f            Print the full path prefix for each file.\n");
      fprintf(stderr,"  -i            Don't print indentation lines.\n");
      fprintf(stderr,"  -q            Print non-printable characters as '?'.\n");
      fprintf(stderr,"  -N            Print non-printable characters as is.\n");
      fprintf(stderr,"  -p            Print the protections for each file.\n");
      fprintf(stderr,"  -u            Displays file owner or UID number.\n");
      fprintf(stderr,"  -g            Displays file group owner or GID number.\n");
      fprintf(stderr,"  -s            Print the size in bytes of each file.\n");
      fprintf(stderr,"  -h            Print the size in a more human readable way.\n");
      fprintf(stderr,"  -D            Print the date of last modification.\n");
      fprintf(stderr,"  -F            Appends '/', '=', '*', or '|' as per ls -F.\n");
      fprintf(stderr,"  -v            Sort files alphanumerically by version.\n");
      fprintf(stderr,"  -r            Sort files in reverse alphanumeric order.\n");
      fprintf(stderr,"  -t            Sort files by last modification time.\n");
      fprintf(stderr,"  -x            Stay on current filesystem only.\n");
      fprintf(stderr,"  -L level      Descend only level directories deep.\n");
      fprintf(stderr,"  -A            Print ANSI lines graphic indentation lines.\n");
      fprintf(stderr,"  -S            Print with ASCII graphics indentation lines.\n");
      fprintf(stderr,"  -n            Turn colorization off always (-C overrides).\n");
      fprintf(stderr,"  -C            Turn colorization on always.\n");
      fprintf(stderr,"  -P pattern    List only those files that match the pattern given.\n");
      fprintf(stderr,"  -I pattern    Do not list files that match the given pattern.\n");
      fprintf(stderr,"  -H baseHREF   Prints out HTML format with baseHREF as top directory.\n");
      fprintf(stderr,"  -T string     Replace the default HTML title and H1 header with string.\n");
      fprintf(stderr,"  -R            Rerun tree when max dir level reached.\n");
      fprintf(stderr,"  -o file       Output to file instead of stdout.\n");
      fprintf(stderr,"  --inodes      Print inode number of each file.\n");
      fprintf(stderr,"  --device      Print device ID number to which each file belongs.\n");
      fprintf(stderr,"  --noreport    Turn off file/directory count at end of tree listing.\n");
      fprintf(stderr,"  --nolinks     Turn off hyperlinks in HTML output.\n");
      fprintf(stderr,"  --dirsfirst   List directories before files.\n");
      fprintf(stderr,"  --charset X   Use charset X for HTML and indentation line output.\n");
      fprintf(stderr,"  --filelimit # Do not descend dirs with more than # files in them.\n");
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
  if (flimit > 0 && n > flimit) {
    fprintf(outfile," [%d entries exceeds filelimit, not opening dir]\n",n);
    free(path);
    free_dir(sav);
    return;
  }
  qsort(dir,n,sizeof(struct _info *),cmpfunc);
  if (lev >= maxdirs-1) {
    dirs = xrealloc(dirs,sizeof(int) * (maxdirs += 1024));
    memset(dirs+(maxdirs-1024), 0, sizeof(int) * 1024);
  }
  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  fprintf(outfile,"\n");
  while(*dir) {
    if (!noindent) indent(lev);

    path[0] = 0;
#ifdef __USE_FILE_OFFSET64
    if (inodeflag) sprintf(path," %7lld",(*dir)->inode);
#else
    if (inodeflag) sprintf(path," %7ld",(*dir)->inode);
#endif
    if (devflag) sprintf(path+strlen(path), " %3d", (int)(*dir)->dev);
#ifdef __EMX__
    if (pflag) sprintf(path+strlen(path), " %s",prot((*dir)->attr));
#else
    if (pflag) sprintf(path+strlen(path), " %s", prot((*dir)->mode));
#endif
    if (uflag) sprintf(path+strlen(path), " %-8.8s", uidtoname((*dir)->uid));
    if (gflag) sprintf(path+strlen(path), " %-8.8s", gidtoname((*dir)->gid));
    if (sflag) psize(path+strlen(path),(*dir)->size);
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
      if (!strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
      else sprintf(path,"%s/%s",d,(*dir)->name);
    } else {
      if (sizeof(char) * (strlen((*dir)->name)+1) > pathsize)
	path=xrealloc(path,pathsize=(sizeof(char) * (strlen((*dir)->name)+1024)));
      sprintf(path,"%s",(*dir)->name);
    }

    if (Hflag) {
      if (Rflag && (lev == Level) && (*dir)->isdir) {
	if (nolinks) fprintf(outfile,"%s",(*dir)->name);
	else {
	  fprintf(outfile,"<a href=\"%s",host);
	  url_encode(outfile,d+1);
	  putc('/',outfile);
	  url_encode(outfile,(*dir)->name);
	  fprintf(outfile,"/00Tree.html\">");
	  html_encode(outfile,(*dir)->name);
	  fprintf(outfile,"</a>\n");
	}

	hdir = gnu_getcwd();
	if (sizeof(char) * (strlen(hdir)+strlen(d)+strlen((*dir)->name)+2) > pathsize)
	  path = xrealloc(path, pathsize = sizeof(char) * (strlen(hdir)+strlen(d)+strlen((*dir)->name) + 1024));

	sprintf(path,"%s%s/%s",hdir,d+1,(*dir)->name);
	fprintf(stderr,"Entering directory %s\n",path);

	hcmd = xmalloc(sizeof(char) * (49 + strlen(host) + strlen(d) + strlen((*dir)->name)) + 10 + (2*strlen(path)));
	sprintf(hcmd,"tree -n -H \"%s%s/%s\" -L %d -R -o \"%s/00Tree.html\" \"%s\"\n", host,d+1,(*dir)->name,Level+1,path,path);
	system(hcmd);
	free(hdir);
	free(hcmd);
      } else {
	if (nolinks) {
	  if (force_color) {
	   /*
	    * Note that the B element has been set to normal weight in the
	    * style portion of the output. so using <b> will just gives us a element
	    * for which we can assign a color class to.
	    */
	    fprintf(outfile, "<b class=\"%s\">%s</b>",
		    (*dir)->isdir ?  "DIR"  :
		    (*dir)->isexe ?  "EXEC" :
		    (*dir)->isfifo ? "FIFO" :
		    (*dir)->issok ?  "SOCK" : "NORM", (*dir)->name);
	  } else
	    fprintf(outfile,"%s",(*dir)->name);
	} else {
	  if (force_color) {
	    sprintf(hclr, "%s",
		    (*dir)->isdir ?  "DIR"  :
		    (*dir)->isexe ?  "EXEC" :
		    (*dir)->isfifo ? "FIFO" :
		    (*dir)->issok ?  "SOCK" : "NORM");
	    fprintf(outfile,"<a class=\"%s\" href=\"%s%s/%s%s\">%s</a>", hclr, host,d+1,(*dir)->name,
		    ((*dir)->isdir?"/":""),(*dir)->name);
	  } else {
	    fprintf(outfile,"<a href=\"%s",host);
	    url_encode(outfile,d+1);
	    putc('/',outfile);
	    url_encode(outfile,(*dir)->name);
	    fprintf(outfile,"%s\">",((*dir)->isdir?"/":""));
	    html_encode(outfile,(*dir)->name);
	    fprintf(outfile,"</a>");
	  }
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
	      if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->lnk);
	      else sprintf(path,"%s/%s",d,(*dir)->lnk);
	      listdir(path,dt,ft,lev+1,dev);
	    }
	    nlf = TRUE;
	  }
	}
      } else if (!(xdev && dev != (*dir)->dev)) {
	if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
	else sprintf(path,"%s/%s",d,(*dir)->name);
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
  static long pathsize, lbufsize;
  struct _info **dl;
  struct dirent *ent;
  struct stat lst,st;
  DIR *d;
  int ne, p = 0, len, rs;

  pathsize = lbufsize = strlen(dir)+4096;
  if (path == NULL) {
    path=xmalloc(pathsize);
    lbuf=xmalloc(lbufsize);
  }

  *n = 1;
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)xmalloc(sizeof(struct _info *) * (ne = MINIT));

  while((ent = (struct dirent *)readdir(d))) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (Hflag && !strcmp(ent->d_name,"00Tree.html")) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    if (strlen(dir)+strlen(ent->d_name)+2 > pathsize) path = xrealloc(path,pathsize=(strlen(dir)+strlen(ent->d_name)+4096));
    sprintf(path,"%s/%s",dir,ent->d_name);
    if (lstat(path,&lst) < 0) continue;
    if ((rs = stat(path,&st)) < 0) st.st_mode = 0;

#ifndef __EMX__
    if ((lst.st_mode & S_IFMT) != S_IFDIR && !(((st.st_mode & S_IFMT) == S_IFLNK) && lflag)) {
      if (pattern && patmatch(ent->d_name,pattern) != 1) continue;
    }
    if (ipattern && patmatch(ent->d_name,ipattern) == 1) continue;
#endif

    if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) continue;
#ifndef __EMX__
    if (pattern && ((st.st_mode & S_IFMT) == S_IFLNK) && !lflag) continue;
#endif

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

#ifdef __EMX__
    dl[p]->attr = lst.st_attr;
#else

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
#endif

    dl[p]->isdir = ((st.st_mode & S_IFMT) == S_IFDIR);
    dl[p]->issok = ((st.st_mode & S_IFMT) == S_IFSOCK);
    dl[p]->isfifo = ((st.st_mode & S_IFMT) == S_IFIFO);
    dl[p++]->isexe = ((st.st_mode & S_IEXEC) | (st.st_mode & (S_IEXEC>>3)) | (st.st_mode & (S_IEXEC>>6))) ? 1 : 0;
  }
  closedir(d);
  *n = p;
  dl[p] = NULL;
  return dl;
}

/* Sorting functions */
int alnumsort(struct _info **a, struct _info **b)
{
  return strcoll((*a)->name,(*b)->name);
}

int versort(struct _info **a, struct _info **b)
{
  return strverscmp((*a)->name,(*b)->name);
}

int reversealnumsort(struct _info **a, struct _info **b)
{
  return strcoll((*b)->name,(*a)->name);
}

int timesort(struct _info **a, struct _info **b)
{
  if ((*a)->mtime == (*b)->mtime) return strcoll((*a)->name,(*b)->name);
  return (*a)->mtime < (*b)->mtime;
}

int dirsfirstsort(struct _info **a, struct _info **b)
{
  if ((*a)->isdir == (*b)->isdir) return strcoll((*a)->name,(*b)->name);
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
 * '|' support added by David MacMahon (davidm@astron.Berkeley.EDU)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
int patmatch(char *buf, char *pat)
{
  int match = 1,m,n;
  char *bar = strchr(pat, '|');

  /* If a bar is found, call patmatch recursively on the two sub-patterns */

  if (bar) {
    /* If the bar is the first or last character, it's a syntax error */
    if (bar == pat || !bar[1]) {
      return -1;
    }
    /* Break pattern into two sub-patterns */
    *bar = '\0';
    match = patmatch(buf, pat);
    if (!match) {
      match = patmatch(buf,bar+1);
    }
    /* Join sub-patterns back into one pattern */
    *bar = '|';
    return match;
  }

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
void indent(int maxlevel)
{
  int i;

  if (ansilines) {
    if (dirs[0]) fprintf(outfile,"\033(0");
    for(i=0; dirs[i] && i <= maxlevel; i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) fprintf(outfile,"\170   ");
	else printf("    ");
      } else {
	if (dirs[i] == 1) fprintf(outfile,"\164\161\161 ");
	else fprintf(outfile,"\155\161\161 ");
      }
    }
    if (dirs[0]) fprintf(outfile,"\033(B");
  } else {
    if (Hflag) fprintf(outfile,"<br>\t\t\t\t   ");
    for(i=0; dirs[i] && i <= maxlevel; i++) {
      fprintf(outfile,"%s ",
	      dirs[i+1] ? (dirs[i]==1 ? linedraw->vert     : (Hflag? "&nbsp;&nbsp;&nbsp;" : "   ") )
			: (dirs[i]==1 ? linedraw->vert_left:linedraw->corner));
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

#ifdef __EMX__
char *prot(long m)
#else
char *prot(u_short m)
#endif
{
#ifdef __EMX__
  static const u_short ifmt[]={
    FILE_ARCHIVED, FILE_DIRECTORY, FILE_SYSTEM,FILE_HIDDEN, FILE_READONLY
  };
  const u_short*p;
  static char buf[6];
  char*cp;

  for(p=ifmt,cp=strcpy(buf,"adshr");*cp;++p,++cp)
    if(!(m&*p))
      *cp='-';
#else
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
#endif
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
  int c;

  if (Nflag) {
    fprintf(outfile,"%s",s);
    return;
  }
  if (mb_cur_max > 1) {
    wchar_t *ws, *tp;
    ws = xmalloc(sizeof(wchar_t)* (c=(strlen(s)+1)));
    if (mbstowcs(ws,s,c) > 0) {
      for(tp=ws;*tp;tp++)
	if (iswprint(*tp)) fprintf(outfile,"%lc",(wint_t)*tp);
	else {
	  if (qflag) putc('?',outfile);
	  else fprintf(outfile,"\\%03o",(unsigned int)*tp);
	}
      free(ws);
      return;
    }
    free(ws);
  }
  for(;*s;s++) {
    c = (unsigned char)*s;
#ifdef __EMX__
    if(_nls_is_dbcs_lead(*(unsigned char*)s)){
      putc(*s,outfile);
      putc(*++s,outfile);
    } else
#endif
      if (isprint(c)) putc(c,outfile);
      else {
	if (qflag) {
	  if (mb_cur_max > 1 && c > 127) putc(c,outfile);
	  else putc('?',outfile);
	} else {
	  if (c < ' ') fprintf(outfile,"^%c",c+'@');
	  else if (c == 127) fprintf(outfile,"^?");
	  else fprintf(outfile,"\\%03o",c);
	}
      }
  }
}

void psize(char *buf, off_t size)
{
  char *unit="BKMGTPEZY";
  int idx;

  if (!hflag) sprintf(buf, sizeof(off_t) == sizeof(long long)? " %11lld" : " %9ld", size);
  else {
    for (idx=size<1024?0:1; size >= (1024*1024); idx++,size>>=10);
    if (!idx) sprintf(buf, " %4d", (int)size);
    else sprintf(buf, ((size>>10) >= 10)? " %3.0f%c" : " %3.1f%c", (float)size/(float)1024,unit[idx]);
  }
}

void html_encode(FILE *fd, char *s)
{
  for(;*s;s++) {
    switch(*s) {
      case '<':
	fputs("&lt;",fd);
	break;
      case '>':
	fputs("&gt;",fd);
	break;
      case '&':
	fputs("&amp;",fd);
	break;
      case '"':
	fputs("&quot;",fd);
	break;
      default:
	fputc(*s,fd);
//	fputc(isprint(*s)?*s:'?',fd);
	break;
    }
  }
}

void url_encode(FILE *fd, char *s)
{
  for(;*s;s++) {
    switch(*s) {
      case ' ':
      case '"':
      case '#':
      case '%':
      case '<':
      case '>':
      case '[':
      case ']':
      case '^':
      case '\\':
      case '?':
      case '+':
	fprintf(fd,"%%%02X",*s);
	break;
      case '&':
	fprintf(fd,"&amp;");
	break;
      default:
	fprintf(fd,isprint((u_int)*s)?"%c":"%%%02X",(u_char)*s);
        break;
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
  char buf[1025], **arg, **c, *colors, *s;
  int i, n;
  struct extensions *e;

  if (Hflag) return;

  if (getenv("TERM") == NULL) {
    colorize = FALSE;
    return;
  }

  s = getenv("LS_COLORS");
  if ((s == NULL || strlen(s) == 0) && force_color) s = ":no=00:fi=00:di=01;34:ln=01;36:pi=40;33:so=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:ex=01;32:*.bat=01;32:*.BAT=01;32:*.btm=01;32:*.BTM=01;32:*.cmd=01;32:*.CMD=01;32:*.com=01;32:*.COM=01;32:*.dll=01;32:*.DLL=01;32:*.exe=01;32:*.EXE=01;32:*.arj=01;31:*.bz2=01;31:*.deb=01;31:*.gz=01;31:*.lzh=01;31:*.rpm=01;31:*.tar=01;31:*.taz=01;31:*.tb2=01;31:*.tbz2=01;31:*.tbz=01;31:*.tgz=01;31:*.tz2=01;31:*.z=01;31:*.Z=01;31:*.zip=01;31:*.ZIP=01;31:*.zoo=01;31:*.asf=01;35:*.ASF=01;35:*.avi=01;35:*.AVI=01;35:*.bmp=01;35:*.BMP=01;35:*.flac=01;35:*.FLAC=01;35:*.gif=01;35:*.GIF=01;35:*.jpg=01;35:*.JPG=01;35:*.jpeg=01;35:*.JPEG=01;35:*.m2a=01;35:*.M2a=01;35:*.m2v=01;35:*.M2V=01;35:*.mov=01;35:*.MOV=01;35:*.mp3=01;35:*.MP3=01;35:*.mpeg=01;35:*.MPEG=01;35:*.mpg=01;35:*.MPG=01;35:*.ogg=01;35:*.OGG=01;35:*.ppm=01;35:*.rm=01;35:*.RM=01;35:*.tga=01;35:*.TGA=01;35:*.tif=01;35:*.TIF=01;35:*.wav=01;35:*.WAV=01;35:*.wmv=01;35:*.WMV=01;35:*.xbm=01;35:*.xpm=01;35:";


  if (s == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = FALSE;
    return;
  } else {
    colorize = TRUE;
    /* You can uncomment the below line and tree will always try to ANSI-fy the indentation lines */
    /*    ansilines = TRUE; */
  }

  colors = scopy(s);

  arg = split(colors,":",&n);

  for(i=0;arg[i];i++) {
    c = split(arg[i],"=",&n);
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
    }
    free(c);
  }
  free(arg);

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

/*
 * You must free the pointer that is allocated by split() after you
 * are done using the array.
 */
char **split(char *str, char *delim, int *nwrds)
{
  int n = 128;
  char **w = xmalloc(sizeof(char *) * n);

  w[*nwrds = 0] = strtok(str,delim);

  while (w[*nwrds]) {
    if (*nwrds == (n-2)) w = xrealloc(w,sizeof(char *) * (n+=256));
    w[++(*nwrds)] = strtok(NULL,delim);
  }

  w[*nwrds] = NULL;
  return w;
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
#ifndef __EMX__
  case S_IFBLK:
    if (!block_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,block_flgs,rightcode);
    return TRUE;
  case S_IFLNK:
    if (!link_flgs) return FALSE;
    fprintf(outfile,"%s%s%s",leftcode,link_flgs,rightcode);
    return TRUE;
#endif
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
    /* not a directory, link, special device, etc, so check for extension match */
    l = strlen(name);
    for(e=ext;e;e=e->nxt) {
      xl = strlen(e->ext);
      if (!strcmp((l>xl)?name+(l-xl):name,e->ext)) {
	fprintf(outfile,"%s%s%s",leftcode,e->term_flg,rightcode);
	return TRUE;
      }
    }
    return FALSE;
  }
  return FALSE;
}

/*
 * Charsets provided by Kyosuke Tokoro (NBG01720@nifty.ne.jp)
 */
const char *getcharset(void)
{
#ifndef __EMX__
  return getenv("TREE_CHARSET");
#else
  static char buffer[13];
  ULONG aulCpList[3],ulListSize,codepage=0;
  char*charset=getenv("TREE_CHARSET");
  if(charset)
    return charset;

  if(!getenv("WINDOWID"))
    if(!DosQueryCp(sizeof aulCpList,aulCpList,&ulListSize))
      if(ulListSize>=sizeof*aulCpList)
	codepage=*aulCpList;

  switch(codepage){
    case 437: case 775: case 850: case 851: case 852: case 855:
    case 857: case 860: case 861: case 862: case 863: case 864:
    case 865: case 866: case 868: case 869: case 891: case 903:
    case 904:
      sprintf(buffer,"IBM%03lu",codepage);
      break;
    case 367:
      return"US-ASCII";
    case 813:
      return"ISO-8859-7";
    case 819:
      return"ISO-8859-1";
    case 881: case 882: case 883: case 884: case 885:
      sprintf(buffer,"ISO-8859-%lu",codepage-880);
      break;
    case  858: case  924:
      sprintf(buffer,"IBM%05lu",codepage);
      break;
    case 874:
      return"TIS-620";
    case 897: case 932: case 942: case 943:
      return"Shift_JIS";
    case 912:
      return"ISO-8859-2";
    case 915:
      return"ISO-8859-5";
    case 916:
      return"ISO-8859-8";
    case 949: case 970:
      return"EUC-KR";
    case 950:
      return"Big5";
    case 954:
      return"EUC-JP";
    case 1051:
      return"hp-roman8";
    case 1089:
      return"ISO-8859-6";
    case 1250: case 1251: case 1253: case 1254: case 1255: case 1256:
    case 1257: case 1258:
      sprintf(buffer,"windows-%lu",codepage);
      break;
    case 1252:
      return"ISO-8859-1-Windows-3.1-Latin-1";
    default:
      return NULL;
  }
#endif
}

void initlinedraw(int flag)
{
  static const char*latin1_3[]={
    "ISO-8859-1", "ISO-8859-1:1987", "ISO_8859-1", "latin1", "l1", "IBM819",
    "CP819", "csISOLatin1", "ISO-8859-3", "ISO_8859-3:1988", "ISO_8859-3",
    "latin3", "ls", "csISOLatin3", NULL
  };
  static const char*iso8859_789[]={
    "ISO-8859-7", "ISO_8859-7:1987", "ISO_8859-7", "ELOT_928", "ECMA-118",
    "greek", "greek8", "csISOLatinGreek", "ISO-8859-8", "ISO_8859-8:1988",
    "iso-ir-138", "ISO_8859-8", "hebrew", "csISOLatinHebrew", "ISO-8859-9",
    "ISO_8859-9:1989", "iso-ir-148", "ISO_8859-9", "latin5", "l5",
    "csISOLatin5", NULL
  };
  static const char*shift_jis[]={
    "Shift_JIS", "MS_Kanji", "csShiftJIS", NULL
  };
  static const char*euc_jp[]={
    "EUC-JP", "Extended_UNIX_Code_Packed_Format_for_Japanese",
    "csEUCPkdFmtJapanese", NULL
  };
  static const char*euc_kr[]={
    "EUC-KR", "csEUCKR", NULL
  };
  static const char*iso2022jp[]={
    "ISO-2022-JP", "csISO2022JP", "ISO-2022-JP-2", "csISO2022JP2", NULL
  };
  static const char*ibm_pc[]={
    "IBM437", "cp437", "437", "csPC8CodePage437", "IBM852", "cp852", "852",
    "csPCp852", "IBM863", "cp863", "863", "csIBM863", "IBM855", "cp855",
    "855", "csIBM855", "IBM865", "cp865", "865", "csIBM865", "IBM866",
    "cp866", "866", "csIBM866", NULL
  };
  static const char*ibm_ps2[]={
    "IBM850", "cp850", "850", "csPC850Multilingual", "IBM00858", "CCSID00858",
    "CP00858", "PC-Multilingual-850+euro", NULL
  };
  static const char*ibm_gr[]={
    "IBM869", "cp869", "869", "cp-gr", "csIBM869", NULL
  };
  static const char*gb[]={
    "GB2312", "csGB2312", NULL
  };
  static const char*utf8[]={
    "UTF-8", "utf8", NULL
  };
  static const char*big5[]={
    "Big5", "csBig5", NULL
  };
  static const char*viscii[]={
    "VISCII", "csVISCII", NULL
  };
  static const char*koi8ru[]={
    "KOI8-R", "csKOI8R", "KOI8-U", NULL
  };
  static const char*windows[]={
    "ISO-8859-1-Windows-3.1-Latin-1", "csWindows31Latin1",
    "ISO-8859-2-Windows-Latin-2", "csWindows31Latin2", "windows-1250",
    "windows-1251", "windows-1253", "windows-1254", "windows-1255",
    "windows-1256", "windows-1256", "windows-1257", NULL
  };
  static const struct linedraw cstable[]={
    { latin1_3,    "|  ",              "|--",            "&middot;--",     "&copy;"   },
    { iso8859_789, "|  ",              "|--",            "&middot;--",     "(c)"      },
    { shift_jis,   "\204\240 ",        "\204\245",       "\204\244",       "(c)"      },
    { euc_jp,      "\250\242 ",        "\250\247",       "\250\246",       "(c)"      },
    { euc_kr,      "\246\242 ",        "\246\247",       "\246\246",       "(c)"      },
    { iso2022jp,   "\033$B(\"\033(B ", "\033$B('\033(B", "\033$B(&\033(B", "(c)"      },
    { ibm_pc,      "\263  ",           "\303\304\304",   "\300\304\304",   "(c)"      },
    { ibm_ps2,     "\263  ",           "\303\304\304",   "\300\304\304",   "\227"     },
    { ibm_gr,      "\263  ",           "\303\304\304",   "\300\304\304",   "\270"     },
    { gb,          "\251\246 ",        "\251\300",       "\251\270",       "(c)"      },
    { utf8,        "\342\224\202\302\240\302\240",
                   "\342\224\234\342\224\200\342\224\200", "\342\224\224\342\224\200\342\224\200", "\302\251" },
    { big5,        "\242x ",           "\242u",          "\242|",          "(c)"      },
    { viscii,      "|  ",              "|--",            "`--",            "\371"     },
    { koi8ru,      "\201  ",           "\206\200\200",   "\204\200\200",   "\277"     },
    { windows,     "|  ",              "|--",            "`--",            "\251"     },
    { NULL,        "|  ",              "|--",            "`--",            "(c)"      }
  };
  const char**s;

  if (flag) {
    fprintf(stderr,"tree: missing argument to --charset, valid charsets include:\n");
    for(linedraw=cstable;linedraw->name;++linedraw)
      for(s=linedraw->name;*s;++s)
	fprintf(stderr,"  %s\n",*s);
    return;
  }
  if (charset)
    for(linedraw=cstable;linedraw->name;++linedraw)
      for(s=linedraw->name;*s;++s)
	if(!strcasecmp(charset,*s))
	  return;

  linedraw=cstable+sizeof cstable/sizeof*cstable-1;
}
