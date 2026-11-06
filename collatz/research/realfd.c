/* realfd.c: copy of ringfd.c with the cone E generic for real quadratic rings (real-quadratic.md):
 *   E = {sigma1 > 0, |sigma1| >= |sigma2|, |sigma1(z/eps)| < |sigma2(z/eps)|}, eps = the second unit of the header.
 * Original header follows.
 * ringfd.c: maps on a quadratic ring Z[tau], tau^2 = t tau - n (or Z as the b = 0 part of Z[i] with pi = 2),
 * generalizing fdsearch.c (Z[i] only) for rings-test.md.
 *
 * One odd step: w = alpha z + d(z), divide by pi while divisible, then rep_F.
 * d is read from the class of z mod pi^K, given by the HNF basis (A,0),(c,g) of pi^K R:
 *   y' = b mod g, x' = (a - ((b-y')/g) c) mod A, idx = y' A + x'.
 * Fundamental domains (ftype): A all of R (no rep; literal), R right half-plane, H upper half-plane,
 *   Q, C (Z[i] quadrant, cone), S, T (Z[w] sectors 0<=arg<60, -30<arg<=30), P (class flags),
 *   E (Z[sqrt2] cone sigma1>0, 1 <= |sigma1/sigma2| < eps^2, full unit group), O (positives of Z).
 * rep_F for finite U: try each unit (given in the header).
 * Size: |N| (a^2 on Z).
 * stdin header: "t n p0 p1 A c g F B cap mode N0 nu u0a u0b ..." then, if F == P, a line of A*g flags,
 *   then jobs: "tag ar ai d_0a d_0b ... d_{M-1}a d_{M-1}b" (M = A*g, even classes ignored).
 * modes: 0 full classification on F (every odd z in F with |N| <= B reaches the cycle of rep_F(1), which is periodic);
 *        1 drop below start on F for N0 < |N| <= B;
 *        2 literal (F must be A): every odd z with |N| <= B reaches a cycle containing a unit; reports trivial cycles.
 * gcc -O2 -o realfd realfd.c -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef __int128 L; typedef long long ll;
#define ESC (((L)1)<<60)
static ll T_, N_, P0, P1, PB0, PB1, NP, HA, HC, HG; static char FT;
static ll AR, AI; static ll (*D)[2]; static unsigned char *FL; static ll M;
static int NU; static ll UN[12][2]; static ll cap;

static inline void mulz(L a, L b, L c, L d, L *x, L *y){ *x = a*c - (L)N_*b*d; *y = a*d + b*c + (L)T_*b*d; }
static inline L nrm(L a, L b){ L v = a*a + (L)T_*a*b + (L)N_*b*b; return v < 0 ? -v : v; }
static inline ll idx(L a, L b){
  L y = b % HG; if (y < 0) y += HG;
  L x = (a - ((b - y)/HG)*HC) % HA; if (x < 0) x += HA;
  return (ll)(y*HA + x); }
/* real quadratic cone: |sigma1| >= |sigma2| iff b(2a + t b) >= 0; exact sign of sigma1 = (x + y sqrt(disc))/2 */
static ll EI0, EI1;
static inline int big1(L a, L b){ return b*(2*a+(L)T_*b) >= 0; }
static inline int sgn1(L a, L b){ L x=2*a+(L)T_*b, y=b; L disc=(L)T_*T_-4*(L)N_;
  if (x>=0 && y>=0) return (x||y)?1:0; if (x<=0 && y<=0) return -1;
  if (x>0) return x*x > y*y*disc ? 1 : -1; return y*y*disc > x*x ? 1 : -1; }
/* Z[sqrt2] cone (unused) */
static inline int r2pos(L a, L b){
  if (a>=0 && b>=0) return a||b; if (a<=0 && b<=0) return 0;
  if (a>0) return a*a > 2*b*b; return 2*b*b > a*a; }
static inline int inF(L a, L b){
  switch(FT){
    case 'A': return 1;
    case 'R': { L s = 2*a + (L)T_*b; return s>0 || (s==0 && b>0); }
    case 'H': return b>0 || (b==0 && a>0);
    case 'O': return a>0;
    case 'Q': return a>0 && b>=0;
    case 'C': return a>0 && -a<b && b<=a;
    case 'S': return 0<=b && b<a;
    case 'T': return a+b>0 && 2*b<=a;
    case 'P': return FL[idx(a,b)];
    case 'E': { if(sgn1(a,b)<=0 || !big1(a,b)) return 0; L x,y; mulz(EI0,EI1,a,b,&x,&y); return !big1(x,y); }
  } return 0; }
static inline void rep(L *x, L *y){
  L a=*x, b=*y;
  if (FT=='A') return;
  if (FT=='E'){
    for(;;){
      if (!big1(a,b)){ L na,nb; mulz(UN[1][0],UN[1][1],a,b,&na,&nb); a=na; b=nb; continue; }
      L wa,wb; mulz(EI0,EI1,a,b,&wa,&wb); if (big1(wa,wb)){ a=wa; b=wb; continue; }
      break; }
    if(sgn1(a,b)<0){ a=-a; b=-b; }
    *x=a; *y=b; return; }
  for (int u=0; u<NU; u++){ L c,d; mulz(UN[u][0],UN[u][1],a,b,&c,&d); if(inF(c,d)){*x=c;*y=d;return;} }
  fprintf(stderr,"rep failed %lld %lld\n",(ll)a,(ll)b); exit(1); }
/* divisible by pi: z*pibar has both coordinates divisible by |N(pi)| */
static inline int divpi(L *a, L *b){
  L x,y; mulz(*a,*b,PB0,PB1,&x,&y);
  ll q = NP<0?-NP:NP;
  if (x % q || y % q) return 0;
  *a = x/NP; *b = y/NP; return 1; }
static inline int oddz(L a, L b){ L x=a,y=b; return !divpi(&x,&y); }
/* one odd step: 0 ok, 1 escape, 2 zero */
static inline int S(L *x, L *y){
  L a=*x, b=*y; ll c=idx(a,b);
  L u,v; mulz(AR,AI,a,b,&u,&v); u+=D[c][0]; v+=D[c][1];
  if(u==0&&v==0) return 2;
  while(divpi(&u,&v));
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  rep(&u,&v); *x=u; *y=v; return 0; }
static inline int isunit(L a, L b){ return nrm(a,b)==1; }

/* hash set with generation stamps */
#define HB 22
#define HS (1u<<HB)
static ll *hk; static unsigned *hg; static unsigned gen=0; static ll hcount=0;
static inline unsigned long long hh(L a, L b){ unsigned long long x=(unsigned long long)(ll)a*0x9E3779B97F4A7C15ULL ^ ((unsigned long long)(ll)b+0x632BE59BD9B4E019ULL)*0xC2B2AE3D27D4EB4FULL; return x>>(64-HB); }
static int hfind(L a, L b){ if(a>((L)1<<40)||a<-((L)1<<40)||b>((L)1<<40)||b<-((L)1<<40)) return 0;
  unsigned long long h=hh(a,b); for(;;){ if(hg[h]!=gen) return 0; if(hk[2*h]==(ll)a&&hk[2*h+1]==(ll)b) return 1; h=(h+1)&(HS-1);} }
static void hput(L a, L b){ if(a>((L)1<<40)||a<-((L)1<<40)||b>((L)1<<40)||b<-((L)1<<40)) return; if(hcount>(ll)(HS*0.7)) return;
  unsigned long long h=hh(a,b); for(;;){ if(hg[h]!=gen){ hg[h]=gen; hk[2*h]=(ll)a; hk[2*h+1]=(ll)b; hcount++; return;} if(hk[2*h]==(ll)a&&hk[2*h+1]==(ll)b) return; h=(h+1)&(HS-1);} }

static void pr(L a, L b){ printf("%lld%+lld", (ll)a, (ll)b); }

static ll cycle_of(L x, L y, L *cx, L *cy, int *err){
  L sa=x, sb=y; ll power=1, lam=1; *err=0;
  for(ll n=0;;n++){
    if(n>cap){*err=3;return 0;}
    int r=S(&x,&y); if(r){*err=r;return 0;}
    if(x==sa&&y==sb){*cx=x;*cy=y;return lam;}
    if(power==lam){sa=x;sb=y;power*=2;lam=0;} lam++;
  } }

static ll (*st)[2]; static ll nst;
static int cmpn(const void*p,const void*q){ const ll*a=p,*b=q; L n1=nrm(a[0],a[1]), n2=nrm(b[0],b[1]); return n1<n2?-1:n1>n2; }

int main(void){
  ll B, N0; int mode; char fl[8];
  if(scanf("%lld %lld %lld %lld %lld %lld %lld %7s %lld %lld %d %lld %d",&T_,&N_,&P0,&P1,&HA,&HC,&HG,fl,&B,&cap,&mode,&N0,&NU)!=13) return 1;
  for(int u=0;u<NU;u++) if(scanf("%lld %lld",&UN[u][0],&UN[u][1])!=2) return 1;
  FT=fl[0]; M=HA*HG;
  if (FT=='E'){ if (NU<2){ fprintf(stderr,"E needs eps as second unit\n"); return 1; }
    L ne=(L)UN[1][0]*UN[1][0]+(L)T_*UN[1][0]*UN[1][1]+(L)N_*UN[1][1]*UN[1][1]; /* +-1 */
    EI0=(ll)((UN[1][0]+T_*UN[1][1])*ne); EI1=(ll)(-UN[1][1]*ne); }
  PB0=P0+T_*P1; PB1=-P1; NP=P0*P0+T_*P0*P1+N_*P1*P1;
  D=malloc(sizeof(ll)*2*M); FL=calloc(M,1);
  if(FT=='P') for(ll i=0;i<M;i++){ int f; if(scanf("%d",&f)!=1) return 1; FL[i]=f; }
  hk=malloc(sizeof(ll)*2*HS); hg=calloc(HS,sizeof(unsigned));
  /* starts */
  ll cap0=1<<20; st=malloc(sizeof(ll)*2*cap0); nst=0;
  double Bd=(double)B; ll bb, ab;
  if(N_<0){ ab=(ll)(2.5*sqrt(Bd))+3; bb=(ll)(1.8*sqrt(Bd))+3; }
  else if(HG==1 && T_==0 && N_==1 && P0==2 && P1==0){ ab=(ll)sqrt(Bd)+2; bb=0; } /* Z */
  else { double Dd=4.0*N_-(double)T_*T_; bb=(ll)sqrt(4*Bd/Dd)+2; ab=(ll)sqrt(Bd)+llabs(T_)*bb+2; }
  for(ll b=-bb;b<=bb;b++) for(ll a=-ab;a<=ab;a++){
    if(!oddz(a,b)) continue; L nn=nrm(a,b); if(nn>B) continue; if(!inF(a,b)) continue;
    if(mode==1 && nn<=N0) continue;
    if(nst>=cap0){ cap0*=2; st=realloc(st,sizeof(ll)*2*cap0); }
    st[nst][0]=a; st[nst][1]=b; nst++; }
  qsort(st,nst,sizeof(ll)*2,cmpn);
  fprintf(stderr,"F=%c starts=%lld\n",FT,nst);
  L *pa=malloc(sizeof(L)*(cap+2)), *pb=malloc(sizeof(L)*(cap+2));
  char tag[256];
  while(scanf("%255s %lld %lld",tag,&AR,&AI)==3){
    for(ll c=0;c<M;c++) if(scanf("%lld %lld",&D[c][0],&D[c][1])!=2) return 1;
    gen++; hcount=0;
    if(mode==1){
      ll maxst=0; L ws=0,wt=0; double maxex=0; int bad=0;
      for(ll s=0;s<nst;s++){
        L x=st[s][0], y=st[s][1]; L n0=nrm(x,y), top=n0; ll n=0;
        while(1){ if(n>=cap){bad=3;break;} int r=S(&x,&y); n++; if(r){bad=r;break;}
          L nn=nrm(x,y); if(nn>top) top=nn; if(nn<n0) break; }
        if(bad){ printf("%s FAIL drop start ",tag); pr(st[s][0],st[s][1]); printf(" code %d\n",bad); break; }
        if(n>maxst){maxst=n;ws=st[s][0];wt=st[s][1];}
        double ex=0.5*log2((double)top/(double)n0); if(ex>maxex) maxex=ex; }
      if(!bad){ printf("%s PASS drop %lld starts, max odd steps %lld at ",tag,nst,maxst); pr(ws,wt); printf(", max excursion %.2f bits\n",maxex); }
      fflush(stdout); continue; }
    if(mode==0){
      L ox=1, oy=0; rep(&ox,&oy);
      L cx,cy; int err; ll lam=cycle_of(ox,oy,&cx,&cy,&err);
      if(err){ printf("%s FAIL orbit of 1: code %d\n",tag,err); fflush(stdout); continue; }
      int has1=0; { L x=cx,y=cy; for(ll j=0;j<lam;j++){ if(x==ox&&y==oy) has1=1; S(&x,&y);} }
      if(!has1){ printf("%s FAIL 1 not periodic, reaches cycle len %lld through ",tag,lam); pr(cx,cy); printf("\n"); fflush(stdout); continue; }
      { L x=cx,y=cy; for(ll j=0;j<lam;j++){ hput(x,y); S(&x,&y);} }
      int fail=0; ll maxlen=0;
      for(ll s=0;s<nst && !fail;s++){
        L x=st[s][0], y=st[s][1];
        if(hfind(x,y)) continue;
        ll len=0; pa[0]=x; pb[0]=y; L sa=x,sb=y; ll power=1,lam2=1; int out=0;
        while(1){
          if(len>=cap){out=3;break;}
          int r=S(&x,&y); if(r){out=r;break;}
          len++; pa[len]=x; pb[len]=y;
          if(hfind(x,y)){out=4;break;}
          if(x==sa&&y==sb){out=5;break;}
          if(power==lam2){sa=x;sb=y;power*=2;lam2=0;} lam2++; }
        if(out==4){ for(ll j=0;j<len;j++) hput(pa[j],pb[j]); if(len>maxlen) maxlen=len; continue; }
        fail=1;
        if(out==5){ L ma=x,mb=y,qx=x,qy=y; for(ll j=0;j<lam2;j++){ if(nrm(qx,qy)<nrm(ma,mb)){ma=qx;mb=qy;} S(&qx,&qy);}
          printf("%s FAIL cycle len %lld min ",tag,lam2); pr(ma,mb); printf(" (norm %lld) from start ",(ll)nrm(ma,mb)); pr(st[s][0],st[s][1]); printf("\n"); }
        else { printf("%s FAIL code %d start ",tag,out); pr(st[s][0],st[s][1]); printf("\n"); } }
      if(!fail){ printf("%s PASS cycle of [1] len %lld: ",tag,lam); L x=ox,y=oy; for(ll j=0;j<lam && j<12;j++){ pr(x,y); printf(" "); S(&x,&y);} printf("| max odd steps to cycle %lld\n",maxlen); }
      fflush(stdout); continue; }
    /* mode 2: literal */
    int ntc=0; char buf[4096]; int bl=0; int fail=0; ll maxlen=0;
    for(ll s=0;s<nst && !fail;s++){
      L x=st[s][0], y=st[s][1];
      if(hfind(x,y)) continue;
      ll len=0; pa[0]=x; pb[0]=y; L sa=x,sb=y; ll power=1,lam2=1; int out=0;
      while(1){
        if(len>=cap){out=3;break;}
        int r=S(&x,&y); if(r){out=r;break;}
        len++; pa[len]=x; pb[len]=y;
        if(hfind(x,y)){out=4;break;}
        if(x==sa&&y==sb){out=5;break;}
        if(power==lam2){sa=x;sb=y;power*=2;lam2=0;} lam2++; }
      if(out==4){ for(ll j=0;j<=len;j++) hput(pa[j],pb[j]); if(len>maxlen) maxlen=len; continue; }
      if(out==5){
        int hasu=0; L qx=x,qy=y; for(ll j=0;j<lam2;j++){ if(isunit(qx,qy)) hasu=1; S(&qx,&qy);}
        if(hasu){ ntc++; if(bl<3800){ bl+=sprintf(buf+bl,"{len %lld units:",lam2); qx=x;qy=y; for(ll j=0;j<lam2;j++){ if(isunit(qx,qy)&&bl<3900) bl+=sprintf(buf+bl," %lld%+lld",(ll)qx,(ll)qy); S(&qx,&qy);} bl+=sprintf(buf+bl,"} "); }
          qx=x;qy=y; for(ll j=0;j<lam2;j++){ hput(qx,qy); S(&qx,&qy);} for(ll j=0;j<=len;j++) hput(pa[j],pb[j]); continue; }
        fail=1; L ma=x,mb=y; qx=x;qy=y; for(ll j=0;j<lam2;j++){ if(nrm(qx,qy)<nrm(ma,mb)){ma=qx;mb=qy;} S(&qx,&qy);}
        printf("%s FAIL cycle len %lld min ",tag,lam2); pr(ma,mb); printf(" (norm %lld) from start ",(ll)nrm(ma,mb)); pr(st[s][0],st[s][1]); printf(" trivial so far %d\n",ntc); }
      else { fail=1; printf("%s FAIL code %d start ",tag,out); pr(st[s][0],st[s][1]); printf("\n"); } }
    if(!fail){ buf[bl]=0; printf("%s PASS trivial cycles %d: %s| max odd steps %lld\n",tag,ntc,buf,maxlen); }
    fflush(stdout); }
  return 0; }
