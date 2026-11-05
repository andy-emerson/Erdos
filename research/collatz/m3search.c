/* m3search.c: Collatz maps on Z[i] at modulus (1+i)^3 with arbitrary (non-commuting) digit tables.
 * For a multiplier alpha and a candidate list of digits for each of the four odd classes mod (1+i)^3
 * (class c = the class of the unit U[c], U = 1, i, -1, -i), every table d[0..3] in the product is tested:
 * every odd z with N(z) <= B, in increasing norm, is iterated under S(z) = (alpha z + d(z))/(1+i)^v
 * until it hits a point already known to reach a trivial cycle (a cycle containing a unit), closes a
 * cycle (literal, Brent), reaches 0, or exceeds the step cap / 2^100.  A table passes if every start
 * reaches a trivial cycle.
 * input file: "ar ai B cap red" (red = 1: test only the least table of each orbit under the
 * conjugacy z -> uz, which sends d[c] to u^-1 d[c+s], u = i^s; the orbit size is printed) then 4 lines "n d1r d1i d2r d2i ...".
 * output: one line per passing or unresolved table, then a summary.
 * gcc -O2 -o m3search m3search.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef __int128 L; typedef long long ll;
#define ESC (((L)1)<<100)
static ll AR, AI; static int CLS[16];
static ll UR[4]={1,0,-1,0}, UI[4]={0,1,0,-1};
static ll cand[4][4096][2]; static int ncand[4];
static ll D[4][2];
static ll R; static int W; static unsigned *stamp; static unsigned gen=0;
static L *pa,*pb; static ll cap;

static int val(ll x, ll y){ int v=0; if(!x&&!y) return 99; while(((x+y)&1)==0){ ll nx=(x+y)/2, ny=(y-x)/2; x=nx;y=ny; v++; } return v; }
static inline int cls(L a, L b){ return CLS[(int)(a&3)*4+(int)(b&3)]; }
/* one odd step; returns 0 ok, 1 escape, 2 zero */
static inline int S(L *x, L *y){
  L a=*x,b=*y; int c=cls(a,b);
  L u=AR*a-AI*b+D[c][0], v=AR*b+AI*a+D[c][1];
  if(u==0&&v==0) return 2;
  while(((u+v)&1)==0){ L nu=(u+v)/2, nv=(v-u)/2; u=nu; v=nv; }
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  *x=u;*y=v; return 0; }
static inline int ing(L a, L b){ return a>=-R&&a<=R&&b>=-R&&b<=R; }
static inline ll gi(L a, L b){ return (ll)(a+R)*W+(ll)(b+R); }
static inline int isunit(L a, L b){ return a*a+b*b==1; }

/* starts sorted by norm */
static ll (*st)[2]; static ll nst;
static int cmpn(const void*p,const void*q){ const ll*a=p,*b=q; ll n1=a[0]*a[0]+a[1]*a[1], n2=b[0]*b[0]+b[1]*b[1]; return n1<n2?-1:n1>n2; }

/* test current table: returns 0 pass, 1 nontrivial cycle, 2 zero, 3 unresolved; *who = start index */
static int test(ll *who, ll *cyc_a, ll *cyc_b, ll *cyclen){
  gen++;
  for(ll s=0;s<nst;s++){
    L x=st[s][0], y=st[s][1];
    if(stamp[gi(x,y)]==gen) continue;
    ll len=0; pa[0]=x; pb[0]=y; L sa=x,sb=y; ll power=1,lam=1; int out=0;
    while(1){
      if(len>=cap){out=3;break;}
      int r=S(&x,&y); if(r==1){out=3;break;} if(r==2){out=2;break;}
      len++; pa[len]=x; pb[len]=y;
      if(ing(x,y)&&stamp[gi(x,y)]==gen){out=4;break;}
      if(x==sa&&y==sb){out=5;break;}
      if(power==lam){sa=x;sb=y;power*=2;lam=0;} lam++;
    }
    if(out==5){ /* cycle of length lam through x */
      L cx=x,cy=y; int tr=0; L ma=x,mb=y;
      for(ll j=0;j<lam;j++){ if(isunit(cx,cy)) tr=1; if(cx*cx+cy*cy<ma*ma+mb*mb){ma=cx;mb=cy;} S(&cx,&cy); }
      if(!tr){ *who=s; *cyc_a=(ll)ma; *cyc_b=(ll)mb; *cyclen=lam; return 1; }
    } else if(out!=4){ *who=s; return out; }
    for(ll j=0;j<=len;j++) if(ing(pa[j],pb[j])) stamp[gi(pa[j],pb[j])]=gen;
  }
  return 0; }

int main(int argc,char**argv){
  if(argc<2){fprintf(stderr,"usage: m3search infile\n");return 1;}
  FILE*f=fopen(argv[1],"r"); ll B;
  int red=0; if(fscanf(f,"%lld %lld %lld %lld %d",&AR,&AI,&B,&cap,&red)!=5) return 1;
  for(int c=0;c<4;c++){ if(fscanf(f,"%d",&ncand[c])!=1) return 1; for(int j=0;j<ncand[c];j++) if(fscanf(f,"%lld %lld",&cand[c][j][0],&cand[c][j][1])!=2) return 1; }
  for(int a=0;a<4;a++) for(int b=0;b<4;b++){ CLS[a*4+b]=-1; if(((a+b)&1)==0) continue;
    for(int c=0;c<4;c++) if(val(a-UR[c],b-UI[c])>=3) CLS[a*4+b]=c; }
  R=1; while((R+1)*(R+1)<=B) R++; W=2*R+1;
  stamp=calloc((size_t)W*W,sizeof(unsigned)); pa=malloc((cap+2)*sizeof(L)); pb=malloc((cap+2)*sizeof(L));
  st=malloc(sizeof(*st)*(size_t)W*W); nst=0;
  for(ll a=-R;a<=R;a++) for(ll b=-R;b<=R;b++) if(((a+b)&1)&&a*a+b*b<=B){ st[nst][0]=a; st[nst][1]=b; nst++; }
  qsort(st,nst,sizeof(*st),cmpn);
  ll nwt=0,npw=0,ntab=0,npass=0,nfc=0,nz=0,nun=0; ll worst=0; ll worst_t[4][2]={{0}};
  ll hist[64]={0}; /* histogram of log2 N of the failing start */
  int idx[4]={0,0,0,0};
  if(!ncand[0]||!ncand[1]||!ncand[2]||!ncand[3]) goto done;
  while(1){
    for(int c=0;c<4;c++){ D[c][0]=cand[c][idx[c]][0]; D[c][1]=cand[c][idx[c]][1];
      L t=AR*UR[c]-AI*UI[c]+D[c][0], t2=AR*UI[c]+AI*UR[c]+D[c][1]; if(val((ll)t,(ll)t2)<3){fprintf(stderr,"bad digit\n");return 2;} }
    int orb=1, skip=0;
    if(red){ /* conjugates d'[c] = i^-s d[c+s] */
      int distinct[4]={1,0,0,0};
      for(int s=1;s<4;s++){ ll E[4][2]; for(int c=0;c<4;c++){ ll x=D[(c+s)%4][0], y=D[(c+s)%4][1];
          for(int k=0;k<s;k++){ ll nx=y, ny=-x; x=nx; y=ny; } E[c][0]=x; E[c][1]=y; }
        int cmp=0; for(int c=0;c<4&&!cmp;c++){ if(E[c][0]!=D[c][0]) cmp=E[c][0]<D[c][0]?-1:1; else if(E[c][1]!=D[c][1]) cmp=E[c][1]<D[c][1]?-1:1; }
        if(cmp<0) skip=1;
        if(cmp!=0) distinct[s]=1; }
      orb = distinct[2]? 4 : (distinct[1]? 2 : 1); /* orbit size: stabilizer {1}, {1,-1} or all */
      if(distinct[1]&&!distinct[2]) orb=2; }
    if(skip) goto next;
    ll who=0,ca=0,cb=0,cl=0; int r=test(&who,&ca,&cb,&cl); ntab++; nwt+=orb;
    if(r==0){ npass++; npw+=orb; printf("PASS %lld %lld | %lld %lld , %lld %lld , %lld %lld , %lld %lld | orbit %d\n",AR,AI,D[0][0],D[0][1],D[1][0],D[1][1],D[2][0],D[2][1],D[3][0],D[3][1],orb); }
    else if(r==3){ nun++; printf("UNRES %lld %lld | %lld %lld , %lld %lld , %lld %lld , %lld %lld | start %lld %lld\n",AR,AI,D[0][0],D[0][1],D[1][0],D[1][1],D[2][0],D[2][1],D[3][0],D[3][1],st[who][0],st[who][1]); }
    else { if(r==1) nfc++; else nz++;
      ll n0=st[who][0]*st[who][0]+st[who][1]*st[who][1]; int lg=0; while((1LL<<(lg+1))<=n0) lg++; hist[lg]++;
      if(n0>worst){ worst=n0; for(int c=0;c<4;c++){worst_t[c][0]=D[c][0];worst_t[c][1]=D[c][1];} } }
   next:;
    int c=0; while(c<4){ idx[c]++; if(idx[c]<ncand[c]) break; idx[c]=0; c++; } if(c==4) break;
  }
 done:
  printf("SUMMARY alpha %lld %lld B %lld tables %lld pass %lld fail_cycle %lld fail_zero %lld unresolved %lld worst_first_fail_norm %lld table %lld %lld , %lld %lld , %lld %lld , %lld %lld\n",
    AR,AI,B,ntab,npass,nfc,nz,nun,worst,worst_t[0][0],worst_t[0][1],worst_t[1][0],worst_t[1][1],worst_t[2][0],worst_t[2][1],worst_t[3][0],worst_t[3][1]);
  printf("WEIGHTED tables %lld pass %lld\n",nwt,npw);
  printf("HIST"); for(int i=0;i<40;i++) if(hist[i]) printf(" %d:%lld",i,hist[i]); printf("\n");
  return 0; }
