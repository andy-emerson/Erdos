/* gcz.c: iterate a Gaussian-integer Collatz-type map on every z with N(z) <= B.
 *
 * The map is given by a table over the 16 residues (a mod 4, b mod 4) of z = a+bi:
 *     T(z) = (alpha_r z + beta_r) / delta_r      (exact division, checked)
 * Rule file: 16 lines "ra rb  alre alim  bere beim  dere deim".
 *
 * For each start: follow the orbit until it reaches an already-resolved table
 * element, closes a cycle (Brent), exceeds the step cap or escapes.  Results are
 * back-filled along the stored path, so every table element is resolved once.
 * Statistics: cycles (canonical min element, length, odd steps, basin count within
 * the table), unresolved starts, max total steps to the cycle, max excursion.
 *
 * usage: gcz rulefile B [cap=100000] [quiet]
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef __int128 ll;  /* coordinates are 128-bit; escape at 2^100 */
static ll AL[64][2], BE[64][2], DE[64][2], DN[64]; static int MOD=4;
static ll R; static ll B; static ll W;
static uint32_t *tot;      /* total steps to reach cycle; UNSET = not done */
static uint16_t *cyc;      /* cycle id */
static float *mx;          /* log2 of max norm along the rest of the orbit */
#define UNSET 0xFFFFFFFFu
#define UNRES 0xFFFFFFFEu
#define ESC (((__int128)1)<<100)

static inline int res(ll a, ll b){ return (int)((a&(MOD-1))*MOD + (b&(MOD-1))); }
static inline int intab(ll a, ll b){ return a>=-R && a<=R && b>=-R && b<=R && a*a+b*b<=B; }
static inline ll idx(ll a, ll b){ return (a+R)*W + (b+R); }
static inline double nrm(ll a, ll b){ return (double)a*(double)a+(double)b*(double)b; }
/* key order: norm, then a, then b */
static inline int keyless(ll a, ll b, ll c, ll d){
  double n1=nrm(a,b), n2=nrm(c,d);
  if(n1!=n2) return n1<n2; if(a!=c) return a<c; return b<d; }

static int step(ll *pa, ll *pb){
  ll a=*pa,b=*pb; int r=res(a,b);
  __int128 x=(__int128)AL[r][0]*a-(__int128)AL[r][1]*b+BE[r][0];
  __int128 y=(__int128)AL[r][0]*b+(__int128)AL[r][1]*a+BE[r][1];
  /* divide by delta: (x+iy)(dr - i di)/N */
  ll dr=DE[r][0], di=DE[r][1], n=DN[r];
  __int128 u=x*dr+y*di, v=y*dr-x*di;
  if(u%n || v%n){ fprintf(stderr,"bad rule: residue %d not divisible at %lld+%lldi\n",r,(long long)a,(long long)b); exit(2);}
  u/=n; v/=n;
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  *pa=u; *pb=v; return 0;
}

#define MAXC 4096
static ll cmin[MAXC][2]; static ll clen[MAXC], codd[MAXC], ccount[MAXC]; static double cmaxn[MAXC];
static int ncyc=0;

int main(int argc,char**argv){
  if(argc<3){fprintf(stderr,"usage\n");return 1;}
  FILE*f=fopen(argv[1],"r"); if(!f){perror("rule");return 1;}
  int nrows=0; { int c; if(fscanf(f,"MOD %d",&MOD)!=1) MOD=4; (void)c; } nrows=MOD*MOD;
  for(int k=0;k<nrows;k++){int ra,rb; long long a1,a2,b1,b2,d1,d2;
    if(fscanf(f,"%d %d %lld %lld %lld %lld %lld %lld",&ra,&rb,&a1,&a2,&b1,&b2,&d1,&d2)!=8){fprintf(stderr,"rule parse\n");return 1;}
    int r=ra*MOD+rb; AL[r][0]=a1;AL[r][1]=a2;BE[r][0]=b1;BE[r][1]=b2;DE[r][0]=d1;DE[r][1]=d2;DN[r]=d1*d1+d2*d2;}
  fclose(f);
  B=atoll(argv[2]); long long cap = argc>3? atoll(argv[3]) : 100000; int quiet = argc>4;
  R=(ll)floor(sqrt((double)B)); W=2*R+1;
  ll T=W*W;
  tot=malloc(T*4); cyc=malloc(T*2); mx=malloc(T*4);
  memset(tot,0xFF,T*4);
  ll *pa=malloc((cap+2)*sizeof(ll)), *pb=malloc((cap+2)*sizeof(ll));
  ll nstart=0,nunres=0; ll maxtot=0, mta=0,mtb=0; double maxexc=0; ll mea=0,meb=0; double sumtot=0;
  ll maxstop=0, msa=0, msb=0; /* stopping time: first step with N below N(start) */
  for(ll a=-R;a<=R;a++) for(ll b=-R;b<=R;b++){
    if(a*a+b*b>B) continue;
    nstart++;
    /* stopping time (separate cheap pass, capped) */
    if(a||b){ ll x=a,y=b; double n0=nrm(a,b); ll s=0; int esc=0;
      while(s<cap){ if(step(&x,&y)){esc=1;break;} s++; if(nrm(x,y)<n0) break; }
      if(!esc && s<cap && s>maxstop){maxstop=s;msa=a;msb=b;} }
    if(tot[idx(a,b)]!=UNSET) continue;
    /* follow orbit */
    ll x=a,y=b; ll len=0; int outcome=0; /* 1 hit done, 2 cycle, 3 unresolved */
    ll sa=x,sb=y; ll power=1,lam=1;
    pa[0]=x;pb[0]=y;
    while(1){
      if(len>=cap){outcome=3;break;}
      if(step(&x,&y)){outcome=3;break;}
      len++; pa[len]=x;pb[len]=y;
      if(intab(x,y) && tot[idx(x,y)]!=UNSET){outcome=1;break;}
      if(x==sa&&y==sb){outcome=2;break;}
      if(power==lam){sa=x;sb=y;power*=2;lam=0;}
      lam++;
    }
    if(outcome==1 && tot[idx(x,y)]==UNRES) outcome=3;
    if(outcome==3){ for(ll j=0;j<len;j++) if(intab(pa[j],pb[j])) tot[idx(pa[j],pb[j])]=UNRES; continue; }
    ll L=len; /* path pa[0..L] */
    ll ent=L; /* index of first element that is on the cycle (outcome 2) */
    int cid; ll basetot; float basemx;
    if(outcome==2){
      /* cycle of length lam containing (x,y) = pa[L]; it is pa[L-lam+1..L] */
      ll cl=lam; ll ma=x,mb=y; ll odd=0; double cmx=0; ll cx=x,cy=y;
      for(ll j=0;j<cl;j++){ if(keyless(cx,cy,ma,mb)){ma=cx;mb=cy;} if((cx+cy)&1) odd++; if(nrm(cx,cy)>cmx)cmx=nrm(cx,cy); step(&cx,&cy);}
      cid=-1; for(int c=0;c<ncyc;c++) if(cmin[c][0]==ma&&cmin[c][1]==mb){cid=c;break;}
      if(cid<0){ if(ncyc>=MAXC){fprintf(stderr,"too many cycles\n");return 3;} cid=ncyc++; cmin[cid][0]=ma;cmin[cid][1]=mb;clen[cid]=cl;codd[cid]=odd;ccount[cid]=0;cmaxn[cid]=cmx; }
      /* first index on cycle: smallest j with pa[j] in cycle; cycle elements are pa[L-cl+1..L] */
      ent=L-cl+1;
      while(ent>0){ /* is pa[ent-1] on cycle? */
        int on=0; for(ll j=L-cl+1;j<=L;j++) if(pa[j]==pa[ent-1]&&pb[j]==pb[ent-1]){on=1;break;}
        if(!on) break; ent--; }
      /* mark cycle elements (one period starting at ent) */
      float lm=(float)log2(cmx>0?cmx:1);
      for(ll j=ent;j<ent+cl;j++) if(intab(pa[j],pb[j])){ ll t=idx(pa[j],pb[j]); tot[t]=0; cyc[t]=cid; mx[t]=lm; }
      basetot=0; basemx=lm;
    } else { ll t=idx(x,y); cid=cyc[t]; basetot=tot[t]; basemx=mx[t]; ent=L; }
    ll curtot=basetot; float curmx=basemx;
    for(ll j=ent-1;j>=0;j--){ curtot++; float ln=(float)log2(nrm(pa[j],pb[j])>0?nrm(pa[j],pb[j]):1); if(ln>curmx)curmx=ln;
      if(intab(pa[j],pb[j])){ ll t=idx(pa[j],pb[j]); if(tot[t]==UNSET){ tot[t]=(uint32_t)curtot; cyc[t]=cid; mx[t]=curmx; } } }
  }
  /* statistics over all starts */
  for(ll a=-R;a<=R;a++) for(ll b=-R;b<=R;b++){
    if(a*a+b*b>B) continue; ll t=idx(a,b);
    if(tot[t]==UNRES||tot[t]==UNSET){nunres++;continue;}
    ccount[cyc[t]]++; sumtot+=tot[t];
    if(tot[t]>maxtot){maxtot=tot[t];mta=a;mtb=b;}
    double n0=nrm(a,b); if(n0>0){ double e=mx[t]-log2(n0); if(e>maxexc){maxexc=e;mea=a;meb=b;} }
  }
  printf("B %lld starts %lld unresolved %lld cycles %d\n",(long long)B,(long long)nstart,(long long)nunres,ncyc);
  printf("maxtot %lld at %lld %lld ; meantot %.2f ; maxstop %lld at %lld %lld ; maxexc_log2 %.2f at %lld %lld\n",(long long)maxtot,(long long)mta,(long long)mtb,sumtot/(double)(nstart-nunres>0?nstart-nunres:1),(long long)maxstop,(long long)msa,(long long)msb,maxexc,(long long)mea,(long long)meb);
  /* sort cycles by count */
  int ord[MAXC]; for(int c=0;c<ncyc;c++)ord[c]=c;
  for(int i=0;i<ncyc;i++)for(int j=i+1;j<ncyc;j++) if(ccount[ord[j]]>ccount[ord[i]]){int t=ord[i];ord[i]=ord[j];ord[j]=t;}
  int lim = quiet? (ncyc<40?ncyc:40) : ncyc;
  for(int k=0;k<lim;k++){int c=ord[k]; printf("cycle min %lld %lld len %lld odd %lld count %lld maxnorm %.0f\n",(long long)cmin[c][0],(long long)cmin[c][1],(long long)clen[c],(long long)codd[c],(long long)ccount[c],cmaxn[c]);}
  return 0;
}
