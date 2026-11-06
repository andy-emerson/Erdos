/* fdsearch.c: fundamental-domain maps on Z[i]/U.
 * A fundamental domain F (one representative per nonzero class of associates) is chosen by a letter:
 *   Q  first quadrant      a > 0, b >= 0
 *   R  mirrored quadrant   a >= 0, b > 0        (the conjugate of Q, rotated by i)
 *   C  rotated cone        a > 0, -a < b <= a   (-45 < arg <= 45)
 *   P  primary             a = 1, b = 0 or a = 3, b = 2 (mod 4)  (z = 1 mod 2+2i, odd z only)
 * The map is S_F(z) = rep_F((alpha z + d(z))/(1+i)^v) on odd z in F, d by z mod 4 (8 odd classes;
 * a table at modulus (1+i)^k, k <= 4, is given on the classes mod 4).
 * stdin: first line "F B cap mode N0"; mode 0: full classification of every odd z in F with N(z) <= B
 * (Collatz map on the quotient: every start reaches the cycle through 1 and 1 is periodic);
 * mode 1: drop-below-start for N0 < N(z) <= B (to be combined with a mode-0 run to N0).
 * then one job per line: "tag ar ai d0r d0i ... d7r d7i", class index of z = a+bi:
 * idx(a mod 4, b mod 4) over the odd pairs in the order (0,1),(0,3),(1,0),(1,2),(2,1),(2,3),(3,0),(3,2).
 * output: "tag PASS ..." or "tag FAIL reason ...".
 * gcc -O2 -o fdsearch fdsearch.c -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef __int128 L; typedef long long ll;
#define ESC (((L)1)<<60)
static char FT; static ll AR, AI; static ll D[8][2]; static int CI[16];
static ll R, W; static unsigned *stamp; static unsigned gen = 0;
static L *pa, *pb; static ll cap;

static inline int inF(L a, L b){
  switch(FT){
    case 'Q': return a > 0 && b >= 0;
    case 'R': return a >= 0 && b > 0;
    case 'C': return a > 0 && -a < b && b <= a;
    case 'P': { int x=(int)(a&3), y=(int)(b&3); return (x==1&&y==0)||(x==3&&y==2); }
  } return 0; }
static inline void rep(L *x, L *y){
  L a=*x, b=*y;
  for(int r=0;r<4;r++){ if(inF(a,b)){*x=a;*y=b;return;} L t=a; a=-b; b=t; }
  fprintf(stderr,"rep failed\n"); exit(1); }
/* one odd step: 0 ok, 1 escape, 2 zero */
static inline int S(L *x, L *y){
  L a=*x, b=*y; int c=CI[(int)(a&3)*4+(int)(b&3)];
  L u=AR*a-AI*b+D[c][0], v=AR*b+AI*a+D[c][1];
  if(u==0&&v==0) return 2;
  while(((u+v)&1)==0){ L nu=(u+v)/2, nv=(v-u)/2; u=nu; v=nv; }
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  rep(&u,&v); *x=u; *y=v; return 0; }
static inline int ing(L a, L b){ return a>=-R&&a<=R&&b>=-R&&b<=R; }
static inline ll gi(L a, L b){ return (ll)(a+R)*W+(ll)(b+R); }
static inline L nrm(L a, L b){ return a*a+b*b; }

static ll (*st)[2]; static ll nst;
static int cmpn(const void*p,const void*q){ const ll*a=p,*b=q; ll n1=a[0]*a[0]+a[1]*a[1], n2=b[0]*b[0]+b[1]*b[1]; return n1<n2?-1:n1>n2; }

static void pr(L a, L b){ printf("%lld%+lldi", (ll)a, (ll)b); }

/* Brent: iterate from (x,y) until a cycle is found; returns length lam, cycle point in (*cx,*cy) */
static ll cycle_of(L x, L y, L *cx, L *cy, int *err){
  L sa=x, sb=y; ll power=1, lam=1; *err=0;
  for(ll n=0;;n++){
    if(n>cap){*err=3;return 0;}
    int r=S(&x,&y); if(r){*err=r;return 0;}
    if(x==sa&&y==sb){*cx=x;*cy=y;return lam;}
    if(power==lam){sa=x;sb=y;power*=2;lam=0;} lam++;
  } }

int main(void){
  ll B, N0; int mode;
  int oc[8][2]={{0,1},{0,3},{1,0},{1,2},{2,1},{2,3},{3,0},{3,2}};
  for(int i=0;i<16;i++) CI[i]=-1; for(int i=0;i<8;i++) CI[oc[i][0]*4+oc[i][1]]=i;
  char fl[8];
  if(scanf("%7s %lld %lld %d %lld", fl, &B, &cap, &mode, &N0)!=5) return 1;
  FT=fl[0];
  R=(ll)sqrt((double)B)+2; W=2*R+1;
  stamp=calloc((size_t)W*W,sizeof(unsigned));
  pa=malloc(sizeof(L)*(cap+2)); pb=malloc(sizeof(L)*(cap+2));
  st=malloc(sizeof(ll)*2*(size_t)W*W); nst=0;
  for(ll a=-R;a<=R;a++) for(ll b=-R;b<=R;b++){
    if(((a+b)&1)==0) continue; if(a*a+b*b>B) continue; if(!inF(a,b)) continue;
    if(mode==1 && a*a+b*b<=N0) continue;
    st[nst][0]=a; st[nst][1]=b; nst++; }
  qsort(st,nst,sizeof(ll)*2,cmpn);
  fprintf(stderr,"F=%c starts=%lld\n",FT,nst);
  char tag[256];
  while(scanf("%255s %lld %lld",tag,&AR,&AI)==3){
    for(int c=0;c<8;c++) if(scanf("%lld %lld",&D[c][0],&D[c][1])!=2) return 1;
    gen++;
    if(mode==1){
      ll maxst=0; L ws=0,wt=0; double maxex=0; int bad=0;
      for(ll s=0;s<nst;s++){
        L x=st[s][0], y=st[s][1]; L n0=nrm(x,y), top=n0; ll n=0;
        while(1){ if(n>=cap){bad=3;break;} int r=S(&x,&y); n++; if(r){bad=r;break;}
          L nn=nrm(x,y); if(nn>top) top=nn; if(nn<n0) break; }
        if(bad){ printf("%s FAIL drop start ",tag); pr(st[s][0],st[s][1]); printf(" code %d\n",bad); break; }
        if(n>maxst){maxst=n;ws=st[s][0];wt=st[s][1];}
        double ex=0.5*log2((double)top/(double)n0); if(ex>maxex) maxex=ex;
      }
      if(!bad){ printf("%s PASS drop %lld starts, max odd steps %lld at ",tag,nst,maxst); pr(ws,wt); printf(", max excursion %.2f bits\n",maxex); }
      fflush(stdout); continue;
    }
    /* mode 0: cycle of [1] */
    L ox=1, oy=0; rep(&ox,&oy); /* the representative of [1] in F */
    L cx,cy; int err; ll lam=cycle_of(ox,oy,&cx,&cy,&err);
    if(err){ printf("%s FAIL orbit of 1: code %d\n",tag,err); fflush(stdout); continue; }
    { int has1=0; L x=cx,y=cy; for(ll j=0;j<lam;j++){ if(x==ox&&y==oy) has1=1; S(&x,&y);}
      if(!has1){ printf("%s FAIL 1 not periodic, reaches cycle len %lld through ",tag,lam); pr(cx,cy); printf("\n"); fflush(stdout); continue; }
      for(ll j=0;j<lam;j++){ if(ing(x,y)) stamp[gi(x,y)]=gen; S(&x,&y);} }
    ll cyc1=lam; int fail=0; ll maxlen=0;
    for(ll s=0;s<nst && !fail;s++){
      L x=st[s][0], y=st[s][1];
      if(stamp[gi(x,y)]==gen) continue;
      ll len=0; pa[0]=x; pb[0]=y; L sa=x,sb=y; ll power=1,lam2=1; int out=0;
      while(1){
        if(len>=cap){out=3;break;}
        int r=S(&x,&y); if(r){out=r;break;}
        len++; pa[len]=x; pb[len]=y;
        if(ing(x,y)&&stamp[gi(x,y)]==gen){out=4;break;}
        if(x==sa&&y==sb){out=5;break;}
        if(power==lam2){sa=x;sb=y;power*=2;lam2=0;} lam2++;
      }
      if(out==4){ for(ll j=0;j<len;j++) if(ing(pa[j],pb[j])) stamp[gi(pa[j],pb[j])]=gen; if(len>maxlen) maxlen=len; continue; }
      fail=1;
      if(out==5){ L ma=x,mb=y,qx=x,qy=y; for(ll j=0;j<lam2;j++){ if(nrm(qx,qy)<nrm(ma,mb)){ma=qx;mb=qy;} S(&qx,&qy);}
        printf("%s FAIL cycle len %lld min ",tag,lam2); pr(ma,mb); printf(" (norm %lld) from start ",(ll)nrm(ma,mb)); pr(st[s][0],st[s][1]); printf("\n"); }
      else { printf("%s FAIL code %d start ",tag,out); pr(st[s][0],st[s][1]); printf("\n"); }
    }
    if(!fail){ printf("%s PASS cycle of [1] len %lld: ",tag,cyc1); L x=ox,y=oy; for(ll j=0;j<cyc1 && j<12;j++){ pr(x,y); printf(" "); S(&x,&y);} printf("| max odd steps to cycle %lld\n",maxlen); }
    fflush(stdout);
  }
  return 0; }
