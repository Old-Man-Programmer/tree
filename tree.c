/* $Copyright: $
 * Copyright (c) 1996,1997 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This software is provided as is without any express or implied
 * warranties, including, without limitation, the implied warranties
 * of merchantability and fitness for a particular purpose.
 */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/limits.h>
#include <pwd.h>
#include <grp.h>

static char *version = "$Version: $ tree v1.2 (c) 1996,1997 by Steve Baker, Thomas Moore $";

void *strcpy(), *malloc();
char **parse(), **split(), *getenv();

#define scopy(x)	strcpy(malloc(strlen(x)+1),(x))

#define MAXDIRLEVEL	PATH_MAX/2
#define MINIT		30  /* number of dir entries to initially allocate */
#define MINC		20  /* allocation increment */

enum {FALSE=0, TRUE};
/* sort type */
enum {ALPHA, TIME};

u_char dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
u_char qflag, Nflag, Dflag;
u_char sort = ALPHA;
u_char noindent, force_color, nocolor, xdev;
char *pattern = NULL, *ipattern = NULL;

int cmp();
struct _info **read_dir();
char *prot(), *uidtoname(), *gidtoname(), *do_date();

int dirs[MAXDIRLEVEL];
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
};

/* Faster uid/gid -> name lookup with hash(tm)(r)(c) tables! */
#define HASH(x)		((x)&255)
struct xtable {
  u_short xid;
  char *name;
  struct xtable *nxt;
} *gtable[256], *utable[256];

/* Hacked in DIR_COLORS support ---------------------------------------------- */

enum {
  CMD_COLOR, CMD_OPTIONS, CMD_TERM, CMD_EIGHTBIT, COL_NORMAL, COL_FILE, COL_DIR,
  COL_LINK, COL_FIFO, COL_SOCK, COL_BLK, COL_CHR, COL_EXEC, DOT_EXTENSION, ERROR,
  COL_ORPHAN, COL_MISSING, COL_LEFTCODE, COL_RIGHTCODE, COL_ENDCODE
};

char colorize = FALSE, ansilines = FALSE;
char *term, termmatch = FALSE, istty;

char *norm_flgs = NULL, *file_flgs = NULL, *dir_flgs = NULL, *link_flgs = NULL;
char *fifo_flgs = NULL, *sock_flgs = NULL, *block_flgs = NULL, *char_flgs = NULL;
char *exec_flgs = NULL, *orphan_flgs = NULL, *missing_flgs = NULL, *leftcode = NULL;
char *rightcode = NULL, *endcode = NULL;

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
  int i,j,n,p,q,dtotal,ftotal,colored = FALSE;
  struct stat st;

  q = p = dtotal = ftotal = 0;
  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag = FALSE;
  Dflag = qflag = Nflag = FALSE;
  noindent = force_color = nocolor = xdev = FALSE;

  for(n=i=1;i<argc;i=n) {
    n++;
    if (argv[i][0] == '-') {
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
	  case 'D':
	    Dflag = TRUE;
	    break;
	  case 't':
	    sort = TIME;
	    break;
	  default:
	    fprintf(stderr,"tree: Invalid argument -`%c'.\nusage: tree [-adfgilnpqstuxACDFN] [-P pattern] [-I pattern] [<directory list>]\n",argv[i][j]);
	    exit(1);
	    break;
	}
      }
    } else {
      if (!dirname) dirname = (char **)malloc(sizeof(char *) * (q=MINIT));
      else if (p == (q-1)) dirname = (char **)realloc(dirname,sizeof(char *) * (q+=MINC));
      dirname[p++] = scopy(argv[i]);
    }
  }
  if (p) dirname[p] = NULL;

  parse_dir_colors();

  if (dirname) {
    for(colored=i=0;dirname[i];i++,colored=0) {
      if (colorize && (n=lstat(dirname[i],&st)) >= 0)
	colored = color(st.st_mode,dirname[i],n<0,FALSE);
      printit(dirname[i]);
      if (colored) printf("%s",endcode);
      listdir(dirname[i],&dtotal,&ftotal,0,0);
    }
  } else {
    if (colorize && (n=lstat(".",&st)) >= 0)
      colored = color(st.st_mode,".",n<0,FALSE);
    putchar('.');
    if (colored) printf("%s",endcode);
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
  int n,x, m,e;

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

    if (pflag || sflag || uflag || gflag || Dflag) printf("[");
    if (pflag) printf("%s",prot((*dir)->mode));
    if (pflag && (uflag || gflag || sflag || Dflag)) printf(" ");
    if (uflag) printf("%-8.8s",uidtoname((*dir)->uid));
    if (uflag && (gflag || sflag || Dflag)) printf(" ");
    if (gflag) printf("%-8.8s",gidtoname((*dir)->gid));
    if (gflag && (sflag || Dflag)) printf(" ");
    if (sflag) printf("%9d",(*dir)->size);
    if (sflag && Dflag) printf(" ");
    if (Dflag) printf("%s",do_date((*dir)->mtime));
    if (pflag || sflag || uflag || gflag || Dflag) printf("]  ");

    if (colorize)
      colored = color((*dir)->mode,(*dir)->name,(*dir)->orphan,FALSE);

    if (fflag) sprintf(path,"%s/%s",d,(*dir)->name);
    else sprintf(path,"%s",(*dir)->name);
    printit(path);

    if (colored) printf("%s",endcode);
    
    if (Fflag && !(*dir)->lnk)
      if (!dflag && (*dir)->isdir) putchar('/');
      else if ((*dir)->issok) putchar('=');
      else if ((*dir)->isfifo) putchar('|');
      else if (!(*dir)->isdir && (*dir)->isexe) putchar('*');

    if ((*dir)->lnk) {
      printf(" -> ");
      if (colorize) colored = color((*dir)->lnkmode,(*dir)->lnk,(*dir)->orphan,TRUE);
      printit((*dir)->lnk);
      if (colored) printf("%s",endcode);
      if (Fflag) {
	m = (*dir)->lnkmode & S_IFMT;
	e = ((*dir)->lnkmode & S_IEXEC) | ((*dir)->lnkmode & (S_IEXEC>>3)) | ((*dir)->lnkmode & (S_IEXEC>>6));
	if (!dflag && m == S_IFDIR) putchar('/');
	else if (m == S_IFSOCK) putchar('=');
	else if (m == S_IFIFO) putchar('|');
	else if (!(m == S_IFDIR) && e) putchar('*');
      }
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
  int ne, p = 0, len, rs;

  *n = 1;
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)malloc(sizeof(struct _info *) * (ne = MINIT));

  while(ent = (struct direct *)readdir(d)) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    sprintf(path,"%s/%s",dir,ent->d_name);
    if (lstat(path,&lst) < 0) continue;
    if ((rs = stat(path,&st)) < 0) st.st_mode = 0;

    if ((lst.st_mode & S_IFMT) != S_IFDIR && !(((st.st_mode & S_IFMT) == S_IFLNK) && lflag)) {
      if (pattern && patmatch(ent->d_name,pattern) != 1) continue;
      if (ipattern && patmatch(ent->d_name,ipattern) == 1) continue;
    }

    if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) continue;
    if (pattern && ((st.st_mode & S_IFMT) == S_IFLNK) && !lflag) continue;


    if (p == (ne-1)) dl = (struct _info **)realloc(dl,sizeof(struct _info) * (ne += MINC));
    dl[p] = (struct _info *)malloc(sizeof(struct _info));

    dl[p]->name = scopy(ent->d_name);
    dl[p]->mode = lst.st_mode;
    dl[p]->uid = lst.st_uid;
    dl[p]->gid = lst.st_gid;
    dl[p]->size = lst.st_size;
    dl[p]->dev = lst.st_dev;
    dl[p]->lnk = NULL;
    dl[p]->orphan = FALSE;

    dl[p]->atime = lst.st_atime;
    dl[p]->ctime = lst.st_ctime;
    dl[p]->mtime = lst.st_mtime;

    if ((lst.st_mode & S_IFMT) == S_IFLNK) {
      if ((len=readlink(path,lbuf,PATH_MAX)) < 0) {
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

cmp(a,b)
struct _info **a, **b;
{
  if (sort == ALPHA)
    return strcmp((*a)->name,(*b)->name);
  else
    return (*a)->mtime < (*b)->mtime;
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

static char *month[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define SIXMONTHS (6*31*24*60*60)

char *do_date(t)
time_t t;
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

char *uidtoname(uid)
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
  t = malloc(sizeof(struct xtable));
  if ((ent = getpwuid(uid)) != NULL) t->name = scopy(ent->pw_name);
  else {
    sprintf(ubuf,"%d",uid);
    t->name = scopy(ubuf);
  }
  t->xid = uid;
  t->nxt = p;
  if (o == utable[uent]) utable[uent] = t;
  else o->nxt = t;
  return t->name;
}

char *gidtoname(gid)
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
  t = malloc(sizeof(struct xtable));
  if ((ent = getgrgid(gid)) != NULL) t->name = scopy(ent->gr_name);
  else {
    sprintf(gbuf,"%d",gid);
    t->name = scopy(gbuf);
  }
  t->xid = gid;
  t->nxt = p;
  if (o == gtable[gent]) gtable[gent] = t;
  else o->nxt = t;
  return t->name;
}

printit(s)
char *s;
{
  char c;

  if (Nflag) printf("%s",s);
  else {
    for(;*s;s++)
      if (isprint(*s)) putchar(*s);
      else {
	if (qflag) putchar('?');
	else {
	  c = *s & 127;
	  if (c < ' ') printf("^%c",c+'@');
	  else if (c == 127) printf("^?");
	}
      }
  }
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

  term = scopy(s);

  if ((s=getenv("LS_COLORS")) == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = FALSE;
    return;
  } else {
    colorize = TRUE;
/* You can uncomment the below line and tree will try to ANSI-fy the indentation lines */
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
	  e = malloc(sizeof(struct extensions));
	  e->ext = scopy(c[0]+1);
	  e->flgs = scopy(c[1]);
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
  if (!leftcode) leftcode = scopy("\033[");
  if (!rightcode) rightcode = scopy("m");
  if (!norm_flgs) norm_flgs = scopy("00");

  if (!endcode) {
    sprintf(buf,"%s%s%s",leftcode,norm_flgs,rightcode);
    endcode = scopy(buf);
  }

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
    "so", COL_SOCK, "bd", COL_BLK, "cd", COL_CHR, "ex", COL_EXEC, "mi", COL_MISSING,
    "or", COL_ORPHAN, "lc", COL_LEFTCODE, "rc", COL_RIGHTCODE, "ec", COL_ENDCODE,
    NULL, 0
  };
  int i;

  for(i=0;cmds[i].cmdnum;i++)
    if (!strcmp(cmds[i].cmd,s)) return cmds[i].cmdnum;
  if (s[0] == '*') return DOT_EXTENSION;
  return ERROR;
}

color(mode,name,orphan,islink)
u_short mode;
char *name;
char orphan, islink;
{
  struct extensions *e;
  int l;

  if (orphan) {
    if (islink) {
      if (missing_flgs) {
	printf("%s%s%s",leftcode,missing_flgs,rightcode);
	return TRUE;
      }
    } else {
      if (orphan_flgs) {
	printf("%s%s%s",leftcode,orphan_flgs,rightcode);
	return TRUE;
      }
    }
  }
  l = strlen(name);
  for(e=ext;e;e=e->nxt) {
    if (!strcmp(name+(l-strlen(e->ext)),e->ext)) {
      printf("%s%s%s",leftcode,e->flgs,rightcode);
      return TRUE;
    }
  }
  switch(mode & S_IFMT) {
    case S_IFIFO:
      if (!fifo_flgs) return FALSE;
      printf("%s%s%s",leftcode,fifo_flgs,rightcode);
      return TRUE;
    case S_IFCHR:
      if (!char_flgs) return FALSE;
      printf("%s%s%s",leftcode,char_flgs,rightcode);
      return TRUE;
    case S_IFDIR:
      if (!dir_flgs) return FALSE;
      printf("%s%s%s",leftcode,dir_flgs,rightcode);
      return TRUE;
    case S_IFBLK:
      if (!block_flgs) return FALSE;
      printf("%s%s%s",leftcode,block_flgs,rightcode);
      return TRUE;
    case S_IFLNK:
      if (!link_flgs) return FALSE;
      printf("%s%s%s",leftcode,link_flgs,rightcode);
      return TRUE;
    case S_IFSOCK:
      if (!sock_flgs) return FALSE;
      printf("%s%s%s",leftcode,sock_flgs,rightcode);
      return TRUE;
    case S_IFREG:
      if (!exec_flgs) return FALSE;
      if ((mode & S_IEXEC) | (mode & (S_IEXEC>>3)) | (mode & (S_IEXEC>>6))) {
	printf("%s%s%s",leftcode,exec_flgs,rightcode);
	return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}
