/* $Copyright: $
 * Copyright (c) 1996 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This software is provided as is without any express or implied
 * warranties, including, without limitation, the implied warranties
 * of merchantability and fitness for a particular purpose.
 */
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/limits.h>

static char *version = "$Version: $ tree v1.1 (c) 1996 by Steve Baker, Thomas Moore $";

void *strcpy(), *malloc();
char **parse(), **split(), *getenv();

#define SCOPY(x)	strcpy(malloc(strlen(x)+1),(x))

#define MAXDIRLEVEL	PATH_MAX/2
#define MINIT		30  /* number of dir entries to initially allocate */
#define MINC		20  /* allocation increment */

enum {FALSE=0, TRUE};

u_char dflag, lflag, pflag, sflag, Fflag, aflag, fflag;
u_char noindent, force_color, nocolor, xdev;
char *pattern = NULL;

int cmp();
struct _info **read_dir();
char *prot();

int dirs[MAXDIRLEVEL];
struct _info {
  char *name;
  char *lnk;
  u_char isdir : 1;
  u_char issok : 1;
  u_char isexe : 1;
  u_short mode, lnkmode, uid, gid;
  u_long size;
  dev_t dev;
};

/* Hacked in DIR_COLORS support ---------------------------------------------- */

enum {
  CMD_COLOR, CMD_OPTIONS, CMD_TERM, CMD_EIGHTBIT, COL_NORMAL, COL_FILE, COL_DIR,
  COL_LINK, COL_FIFO, COL_SOCK, COL_BLK, COL_CHR, COL_EXEC, DOT_EXTENSION, ERROR
};

char colorize = FALSE, ansilines = FALSE;
char *term, termmatch = FALSE, istty;

char *norm_flgs = NULL, *file_flgs = NULL, *dir_flgs = NULL, *link_flgs = NULL;
char *fifo_flgs = NULL, *sock_flgs = NULL, *block_flgs = NULL, *char_flgs = NULL;
char *exec_flgs = NULL;

struct extensions {
  char *ext;
  char *flgs;
  struct extensions *nxt;
} *ext;


main(argc,argv)
int argc;
char **argv;
{
  char **dirname = NULL;
  int i,j,n,p,dtotal,ftotal,colored;
  struct stat st;

  n = p = dtotal = ftotal = 0;
  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = FALSE;
  noindent = force_color = nocolor = xdev = FALSE;

  for(n=i=1;i<argc;i=n) {
    n++;
    if (argv[i][0] == '-') {
      for(j=1;argv[i][j];j++) {
	switch(argv[i][j]) {
	  case 'd':
	    dflag = TRUE;
	    break;
	  case 'l':
	    lflag = TRUE;
	    break;
	  case 's':
	    sflag = TRUE;
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
	  case 'A':
	    ansilines = TRUE;
	    break;
	  default:
	    fprintf(stderr,"tree: Invalid argument.\nusage: tree [-adflpsFiCn] [<directory list>]\n");
	    exit(1);
	    break;
	}
      }
    } else {
      if (!dirname) dirname = (char **)malloc(sizeof(char *) * (n=MINIT));
      else if (p == (n-1)) dirname = (char **)realloc(dirname,sizeof(char *) * (n+=MINC));
      dirname[p++] = SCOPY(argv[i]);
    }
  }
  if (p) dirname[p] = NULL;

  parse_dir_colors();

  if (dirname) {
    for(colored=i=0;dirname[i];i++,colored=0) {
      if (colorize && stat(dirname[i],&st) >= 0)
	colored = color(st.st_mode,dirname[i]);
      printf("%s",dirname[i]);
      if (colored) printf("\033[%sm",norm_flgs);
      listdir(dirname[i],&dtotal,&ftotal,0,0);
    }
  } else {
    if (colorize && stat(".",&st) >= 0)
      colored = color(st.st_mode,".");
    putchar('.');
    if (colored) printf("\033[%sm",norm_flgs);
    listdir(".",&dtotal,&ftotal,0,0);
  }
  if (dflag)
    printf("\n%d director%s\n",dtotal,(dtotal==1? "y":"ies"));
  else
    printf("\n%d director%s, %d file%s\n",dtotal,(dtotal==1? "y":"ies"),ftotal,(ftotal==1? "":"s"));
  exit(0);
}

listdir(d,dt,ft,lev,dev)
char *d;
int *dt, *ft;
u_char lev;
dev_t dev;
{
  char path[PATH_MAX+1], nlf = FALSE, colored = FALSE;
  struct _info **dir, **sav;
  struct stat sb;
  int n;

  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }

  sav = dir = read_dir(d,&n);
  if (!dir && n) {
    printf(" [error opening dir]\n");
    return;
  }
  if (!n) {
    putchar('\n');
    return;
  }
  qsort(dir,n,sizeof(struct _info *),cmp);
  dirs[lev] = 1;
  if (!*(dir+1)) dirs[lev] = 2;
  putchar('\n');
  while(*dir) {
    if (!noindent) indent();

    if (pflag || sflag) printf("[");
    if (pflag) printf("%s",prot((*dir)->mode));
    if (pflag && sflag) printf(" ");
    if (sflag) printf("%9d",(*dir)->size);
    if (pflag || sflag) printf("]  ");

    if (colorize)
      colored = color((*dir)->mode,(*dir)->name);

    if (fflag) printf("%s/%s",d,(*dir)->name);
    else printf("%s",(*dir)->name);

    if (colored) printf("\033[%sm",norm_flgs);
    
    if (Fflag)
      if (!dflag && (*dir)->isdir) putchar('/');
      else if (!(*dir)->isdir && (*dir)->isexe) putchar('*');
      else if ((*dir)->issok) putchar('=');

    if ((*dir)->lnk) {
      printf(" -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk);
      printf("%s",(*dir)->lnk);
      if (colored) printf("\033[%sm",norm_flgs);
    }

    if ((*dir)->isdir) {
      if ((*dir)->lnk) {
	if (lflag && !(xdev && dev != (*dir)->dev)) {
	  if (*(*dir)->lnk == '/')
	    listdir((*dir)->lnk,dt,ft,lev+1,dev);
	  else {
	    sprintf(path,"%s/%s",d,(*dir)->lnk);
	    listdir(path,dt,ft,lev+1,dev);
	  }
	  nlf = TRUE;
	}
      } else if (!(xdev && dev != (*dir)->dev)) {
	sprintf(path,"%s/%s",d,(*dir)->name);
	listdir(path,dt,ft,lev+1,dev);
	nlf = TRUE;
      }
      *dt += 1;
    } else *ft += 1;
    dir++;
    if (!*(dir+1)) dirs[lev] = 2;
    if (nlf) nlf = FALSE;
    else putchar('\n');
  }
  dirs[lev] = 0;
  free_dir(sav);
}


struct _info **read_dir(dir, n)
char *dir;
int *n;
{
  static char path[PATH_MAX+1];
  static char lbuf[PATH_MAX+1];
  struct _info **dl;
  struct direct *ent;
  struct stat lst,st;
  DIR *d;
  int ne, p = 0, len;

  *n = 1;
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)malloc(sizeof(struct _info *) * (ne = MINIT));

  while(ent = (struct direct *)readdir(d)) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    sprintf(path,"%s/%s",dir,ent->d_name);
    if (stat(path,&st) < 0) continue;
    if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) continue;

    if ((st.st_mode & S_IFMT) != S_IFDIR && pattern && patmatch(ent->d_name,pattern) != 1) continue;

    if (lstat(path,&lst) < 0) continue;
    if (pattern && ((lst.st_mode & S_IFMT) == S_IFLNK) && !lflag) continue;

    if (p == (ne-1)) dl = (struct _info **)realloc(dl,sizeof(struct _info) * (ne += MINC));
    dl[p] = (struct _info *)malloc(sizeof(struct _info));

    if ((lst.st_mode & S_IFMT) == S_IFLNK) {
      if ((len=readlink(path,lbuf,PATH_MAX)) < 0) {
	dl[p]->lnk = SCOPY("[Error reading symbolic link information]");
	dl[p]->name = SCOPY(ent->d_name);
	dl[p]->isdir = dl[p]->issok = dl[p]->isexe = FALSE;
	dl[p]->mode = lst.st_mode;
	dl[p]->lnkmode = st.st_mode;
	dl[p]->uid = lst.st_uid;
	dl[p]->gid = lst.st_gid;
	dl[p]->dev = lst.st_dev;
	dl[p++]->size = lst.st_size;
	continue;
      } else {
	lbuf[len] = 0;
	dl[p]->lnk = SCOPY(lbuf);
	dl[p]->mode = lst.st_mode;
	dl[p]->lnkmode = st.st_mode;
	dl[p]->uid = lst.st_uid;
	dl[p]->gid = lst.st_gid;
	dl[p]->size = lst.st_size;
	dl[p]->dev = lst.st_dev;
      }
    } else {
      dl[p]->lnk = NULL;
      dl[p]->mode = st.st_mode;
      dl[p]->uid = st.st_uid;
      dl[p]->gid = st.st_gid;
      dl[p]->size = st.st_size;
      dl[p]->dev = st.st_dev;
    }
    
    dl[p]->name = SCOPY(ent->d_name);
    dl[p]->isdir = ((st.st_mode & S_IFMT) == S_IFDIR);
    dl[p]->issok = ((st.st_mode & S_IFMT) == S_IFSOCK);
    dl[p++]->isexe = (st.st_mode & S_IEXEC) | (st.st_mode & (S_IEXEC>>3)) | (st.st_mode & (S_IEXEC>>6));
  }
  closedir(d);
  *n = p;
  dl[p] = NULL;
  return dl;
}

cmp(a,b)
struct _info **a, **b;
{
  return strcmp((*a)->name,(*b)->name);
}

/*
 * Patmatch() code courtesy of Thomas Moore (dark@mama.indstate.edu)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
patmatch(buf,pat)
char *buf,*pat;
{
  int match = 1,m,n,l,tm;
  char *p,*t,*tp;

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

indent()
{
  int i;

  if (ansilines) {
    if (dirs[0]) printf("\033(0");
    for(i=0;dirs[i];i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) printf("\170   ");
	else printf("    ");
      } else {
	if (dirs[i] == 1) printf("\164\161\161 ");
	else printf("\155\161\161 ");
      }
    }
    if (dirs[0]) printf("\033(B");
  } else {
    for(i=0;dirs[i];i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) printf("|   ");
	else printf("    ");
      }	else {
	if (dirs[i] == 1) printf("|-- ");
	else printf("`-- ");
      }
    }
  }
}

free_dir(d)
struct _info **d;
{
  int i;

  for(i=0;d[i];i++) {
    free(d[i]->name);
    if (d[i]->lnk) free(d[i]->lnk);
    free(d[i]);
  }
  free(d);
}

char *prot(m)
u_short m;
{
  static char buf[11];
  static u_short ifmt[] = {S_IFIFO, S_IFCHR, S_IFDIR, S_IFBLK, S_IFREG, S_IFLNK, S_IFSOCK, 0};
  static char fmt[] = {'p','c','d','b','-','l','s','?'};
  char i;

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
parse_dir_colors()
{
  FILE *fd;
  char buf[1025], **arg, *colors, *term;
  char *s, **c;
  int i;
  struct extensions *e;

  if ((s=getenv("TERM")) == NULL) {
    colorize = FALSE;
    return;
  }

  term = SCOPY(s);

  if ((s=getenv("LS_COLORS")) == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = FALSE;
    return;
  } else {
    colorize = TRUE;
/* You can uncomment the below line and tree will try to ANSI-fy the indentation lines */
/*    ansilines = TRUE; */
  }

  colors = SCOPY(s);

  arg = parse(colors);

  for(i=0;arg[i];i++) {
    c = split(arg[i]);
    switch(cmd(c[0])) {
      case COL_NORMAL:
	if (c[1]) norm_flgs = SCOPY(c[1]);
	break;
      case COL_FILE:
	if (c[1]) file_flgs = SCOPY(c[1]);
	break;
      case COL_DIR:
	if (c[1]) dir_flgs = SCOPY(c[1]);
	break;
      case COL_LINK:
	if (c[1]) link_flgs = SCOPY(c[1]);
	break;
      case COL_FIFO:
	if (c[1]) fifo_flgs = SCOPY(c[1]);
	break;
      case COL_SOCK:
	if (c[1]) sock_flgs = SCOPY(c[1]);
	break;
      case COL_BLK:
	if (c[1]) block_flgs = SCOPY(c[1]);
	break;
      case COL_CHR:
	if (c[1]) char_flgs = SCOPY(c[1]);
	break;
      case COL_EXEC:
	if (c[1]) exec_flgs = SCOPY(c[1]);
	break;
      case DOT_EXTENSION:
	if (c[1]) {
	  e = malloc(sizeof(struct extensions));
	  e->ext = SCOPY(c[0]+1);
	  e->flgs = SCOPY(c[1]);
	  if (ext == NULL) ext = e;
	  else {
	    e->nxt = ext;
	    ext = e;
	  }
	}
      default:
    }
  }

  /* make sure at least norm_flgs is defined.  We're going to assume vt100 support */
  if (!norm_flgs) norm_flgs = SCOPY("00");

/*  if (!termmatch) colorize = FALSE; */
}

char **parse(b)
char *b;
{
  static char *arg[100];
  int i, j;

  for(j=0;*b;b++) {
    while(*b && *b == ':') b++;
    arg[j++] = b;
    while(*b && *b != ':') b++;
    if (!*b) break;
    *b = 0;
  }
  arg[j] = NULL;
  return arg;
}

char **split(b)
char *b;
{
  static char *arg[2];

  arg[0] = b;
  while(*b != '=') b++;
  *b = 0;
  b++;
  arg[1] = b;

  return arg;
}

cmd(s)
char *s;
{
  static struct {
    char *cmd;
    char cmdnum;
  } cmds[] = {
    "no", COL_NORMAL, "fi", COL_FILE, "di", COL_DIR, "ln", COL_LINK, "pi", COL_FIFO,
    "so", COL_SOCK, "bd", COL_BLK, "cd", COL_CHR, "ex", COL_EXEC, NULL, 0
  };
  int i;

  for(i=0;cmds[i].cmdnum;i++)
    if (!strcmp(cmds[i].cmd,s)) return cmds[i].cmdnum;
  if (s[0] == '*') return DOT_EXTENSION;
  return ERROR;
}

color(mode,name)
u_short mode;
char *name;
{
  struct extensions *e;
  int l;

  l = strlen(name);
  for(e=ext;e;e=e->nxt) {
    if (!strcmp(name+(l-strlen(e->ext)),e->ext)) {
      printf("\033[%sm",e->flgs);
      return TRUE;
    }
  }
  switch(mode & S_IFMT) {
    case S_IFIFO:
      if (!fifo_flgs) return FALSE;
      printf("\033[%sm",fifo_flgs);
      return TRUE;
    case S_IFCHR:
      if (!char_flgs) return FALSE;
      printf("\033[%sm",char_flgs);
      return TRUE;
    case S_IFDIR:
      if (!dir_flgs) return FALSE;
      printf("\033[%sm",dir_flgs);
      return TRUE;
    case S_IFBLK:
      if (!block_flgs) return FALSE;
      printf("\033[%sm",block_flgs);
      return TRUE;
    case S_IFLNK:
      if (!link_flgs) return FALSE;
      printf("\033[%sm",link_flgs);
      return TRUE;
    case S_IFSOCK:
      if (!sock_flgs) return FALSE;
      printf("\033[%sm",sock_flgs);
      return TRUE;
    case S_IFREG:
      if (!exec_flgs) return FALSE;
      if ((mode & S_IEXEC) | (mode & (S_IEXEC>>3)) | (mode & (S_IEXEC>>6))) {
	printf("\033[%sm",exec_flgs);
	return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}
