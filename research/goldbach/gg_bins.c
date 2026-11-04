/* gg.c: verify Gaussian Goldbach variants for all even z = a+bi with N(z) <= NMAX.
 *
 * Variants (Holben-Jordan 1968, Knill 2016):
 *   E: N(z) > 2: z = p1+p2, Gaussian primes, angle(pj, z) < 45 deg (strict).
 *   F: N(z) > 10: same with angle < 30 deg (strict and non-strict coincide).
 *   Q: z = a+bi, a>1, b>1: z = p1+p2 with p1, p2 in the open first quadrant.
 * D (no restriction) follows from E for N(z) > 2 (checked separately for N <= 2).
 *
 * E and F are invariant under the 8 symmetries (units, conjugation), so only
 * 0 <= b <= a is enumerated. Q is invariant under a <-> b, so 2 <= b <= a.
 *
 * Work proceeds in norm shells [S, S+W). Rational primality of norms is read from
 * a segmented odd-only sieve covering the shell plus a margin; anything outside
 * falls back to a small-prime table or deterministic Miller-Rabin, so correctness
 * never depends on the margin.
 *
 * For E/F the candidate p1 runs over all Gaussian primes (all associates) sorted
 * by norm, so the first hit gives the minimal N(p1) over all representations
 * (by the p1 <-> p2 symmetry this is min over reps of min(N(p1), N(p2))).
 * For Q, p1 = x+yi runs over diagonals x+y = k = 2, 3, ..., so the first hit gives
 * the minimal L1 size k over representations.
 *
 * usage: gg NMAX variants(E,F,Q letters) [Wlog2=27] [PLIST_MAXNORM=2000000] [NMIN=0]
 * (targets with NMIN <= N(z) <= NMAX; GGDUMP=1 in the environment prints every target)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef int64_t i64; typedef uint64_t u64; typedef __int128 i128; typedef unsigned __int128 u128;

/* ---------- integer sqrt ---------- */
static u64 isqrt64(u64 n){ u64 r=(u64)sqrtl((long double)n); while(r*r>n) r--; while((r+1)*(r+1)<=n) r++; return r; }

/* ---------- Miller-Rabin, deterministic for 64-bit ---------- */
static u64 mulmod(u64 a,u64 b,u64 m){ return (u64)((u128)a*b%m); }
static u64 powmod(u64 a,u64 e,u64 m){ u64 r=1; a%=m; while(e){ if(e&1) r=mulmod(r,a,m); a=mulmod(a,a,m); e>>=1;} return r; }
static int mr(u64 n){
  if(n<2) return 0; static const u64 sp[]={2,3,5,7,11,13,17,19,23,29,31,37};
  for(int i=0;i<12;i++){ if(n%sp[i]==0) return n==sp[i]; }
  u64 d=n-1; int s=0; while(!(d&1)){d>>=1;s++;}
  for(int i=0;i<12;i++){ u64 x=powmod(sp[i],d,n); if(x==1||x==n-1) continue; int ok=0;
    for(int r=1;r<s;r++){ x=mulmod(x,x,n); if(x==n-1){ok=1;break;} } if(!ok) return 0; }
  return 1;
}

/* ---------- small prime table (odd-only bits) up to SMALLLIM ---------- */
static u64 SMALLLIM; static uint8_t *smallbits; /* bit set = composite, index (n-1)/2 */
static void small_sieve(u64 lim){
  SMALLLIM=lim; size_t nb=(lim/2)+1; smallbits=calloc(nb/8+1,1);
  smallbits[0]|=1; /* 1 not prime */
  for(u64 p=3;p*p<=lim;p+=2) if(!(smallbits[(p>>1)>>3]&(1<<((p>>1)&7))))
    for(u64 q=p*p;q<=lim;q+=2*p) smallbits[(q>>1)>>3]|=1<<((q>>1)&7);
}
static inline int small_isprime(u64 n){ if(n==2) return 1; if(!(n&1)) return 0; return !(smallbits[(n>>1)>>3]&(1<<((n>>1)&7))); }

/* ---------- segment sieve over [segLo, segHi), odd-only ---------- */
static u64 segLo=0, segHi=0; static uint8_t *segbits=NULL; static size_t segcap=0;
static u64 *bp; static size_t nbp; /* odd base primes */
static void make_base(u64 lim){ size_t cap=1024; bp=malloc(cap*8); nbp=0;
  for(u64 p=3;p<=lim;p+=2) if(small_isprime(p)){ if(nbp==cap){cap*=2;bp=realloc(bp,cap*8);} bp[nbp++]=p; } }
static void seg_sieve(u64 lo,u64 hi){
  if(lo<3) lo=3; if(lo%2==0) lo--; /* lo odd */
  segLo=lo; segHi=hi; size_t n=(hi-lo)/2+1; size_t bytes=n/8+1;
  if(bytes>segcap){ free(segbits); segbits=malloc(bytes); segcap=bytes; }
  memset(segbits,0,bytes);
  u64 r=isqrt64(hi)+1;
  for(size_t i=0;i<nbp && bp[i]<=r;i++){ u64 p=bp[i]; u64 s=p*p; if(s<lo){ s=(lo+p-1)/p*p; if(!(s&1)) s+=p; }
    for(u64 q=s;q<hi;q+=2*p){ u64 k=(q-lo)>>1; segbits[k>>3]|=1<<(k&7);} }
}
static long n_mr=0;
static inline int isprime(u64 n){
  if(n<=SMALLLIM) return small_isprime(n);
  if(!(n&1)) return 0;
  if(n>=segLo && n<segHi){ u64 k=(n-segLo)>>1; return !(segbits[k>>3]&(1<<(k&7))); }
  n_mr++; return mr(n);
}
static inline int gprime(i64 x,i64 y){
  if(x==0){ u64 v=y<0?-y:y; return (v&3)==3 && isprime(v); }
  if(y==0){ u64 v=x<0?-x:x; return (v&3)==3 && isprime(v); }
  return isprime((u64)(x*x+y*y));
}

/* ---------- Gaussian prime list sorted by norm ---------- */
typedef struct { int32_t x,y; i64 n; } gp_t;
static gp_t *GP; static size_t nGP; static i64 GPMAX;
static int cmpgp(const void*a,const void*b){ const gp_t*p=a,*q=b; if(p->n!=q->n) return p->n<q->n?-1:1;
  if(p->x!=q->x) return p->x<q->x?-1:1; return p->y<q->y?-1:(p->y>q->y); }
static void make_gp(i64 M){
  GPMAX=M; size_t cap=1<<16; GP=malloc(cap*sizeof(gp_t)); nGP=0; i64 R=(i64)isqrt64(M);
  for(i64 x=-R;x<=R;x++) for(i64 y=-R;y<=R;y++){ i64 n=x*x+y*y; if(n>M||n<2) continue;
    if(gprime(x,y)){ if(nGP==cap){cap*=2;GP=realloc(GP,cap*sizeof(gp_t));} GP[nGP++]=(gp_t){(int32_t)x,(int32_t)y,n}; } }
  qsort(GP,nGP,sizeof(gp_t),cmpgp);
}

/* ---------- per-variant search ---------- */
/* returns minimal N(p1) or -1 if not found in list; strict45: 1 strict, 0 non-strict */
static i64 searchE(i64 a,i64 b,int strict,size_t *idx){
  i64 nz=a*a+b*b;
  for(size_t i=0;i<nGP;i++){ i64 px=GP[i].x,py=GP[i].y;
    i64 dot=px*a+py*b, cr=px*b-py*a; if(cr<0) cr=-cr; i64 dot2=nz-dot;
    if(strict){ if(!(dot>cr && dot2>cr)) continue; } else { if(!(dot>=cr && dot2>=cr && dot>0 && dot2>0)) continue; }
    if(gprime(a-px,b-py)){ if(idx)*idx=i; return GP[i].n; } }
  return -1;
}
static i64 searchF(i64 a,i64 b,size_t *idx){
  i64 nz=a*a+b*b;
  for(size_t i=0;i<nGP;i++){ i64 px=GP[i].x,py=GP[i].y;
    i64 dot=px*a+py*b; if(dot<=0) continue; i64 dot2=nz-dot; if(dot2<=0) continue;
    i128 cr=(i128)(px*b-py*a); i128 c3=3*cr*cr;
    if(!(c3<(i128)dot*dot && c3<(i128)dot2*dot2)) continue;
    if(gprime(a-px,b-py)){ if(idx)*idx=i; return GP[i].n; } }
  return -1;
}
/* Q: minimal k = x+y over p1=x+yi, 1<=x<=a-1, 1<=y<=b-1, both p1 and z-p1 prime (open quadrant) */
static i64 searchQ(i64 a,i64 b,i64 *ox,i64 *oy){
  for(i64 k=2;k<=a+b-2;k++){
    i64 ylo=k-(a-1); if(ylo<1) ylo=1; i64 yhi=k-1; if(yhi>b-1) yhi=b-1;
    for(i64 y=ylo;y<=yhi;y++){ i64 x=k-y;
      if(isprime((u64)(x*x+y*y)) && isprime((u64)((a-x)*(a-x)+(b-y)*(b-y)))){ *ox=x;*oy=y; return k; } } }
  return -1;
}

/* ---------- stats ---------- */
#define TOPK 12
typedef struct { i64 v,a,b,px,py; } rec_t;
typedef struct { const char*name; u64 count, fails, nonstrict_only; rec_t top[TOPK]; int ntop; u64 hist[64]; rec_t bin[64]; } stat_t;
static void addtop(stat_t*s,i64 v,i64 a,i64 b,i64 px,i64 py){
  int j=s->ntop; if(j<TOPK) s->ntop++; else { if(v<=s->top[TOPK-1].v) return; j=TOPK-1; }
  while(j>0 && s->top[j-1].v<v){ s->top[j]=s->top[j-1]; j--; } s->top[j]=(rec_t){v,a,b,px,py};
}
static void addbin(stat_t*s,i64 v,i64 a,i64 b,i64 px,i64 py){ i64 n=a*a+b*b; int k=0; while(n>1){n>>=1;k++;} if(v>s->bin[k].v || (v==s->bin[k].v && a*a+b*b<s->bin[k].a*s->bin[k].a+s->bin[k].b*s->bin[k].b)) s->bin[k]=(rec_t){v,a,b,px,py}; }
static void addhist(stat_t*s,i64 v){ int k=0; while(v>1){v>>=1;k++;} s->hist[k]++; }

int main(int argc,char**argv){
  if(argc<3){ fprintf(stderr,"usage: gg NMAX variants [Wlog2] [PMAX]\n"); return 1; }
  u64 NMAX=strtoull(argv[1],0,10); const char*var=argv[2];
  int wl=argc>3?atoi(argv[3]):27; i64 PM=argc>4?atoll(argv[4]):2000000; u64 NMIN=argc>5?strtoull(argv[5],0,10):0;
  int doE=!!strchr(var,'E'), doF=!!strchr(var,'F'), doQ=!!strchr(var,'Q');
  clock_t t0=clock(); int dump=getenv("GGDUMP")!=NULL; int nosieve=getenv("GGNOSIEVE")!=NULL;
  u64 baselim=isqrt64((NMAX+4*(isqrt64(NMAX)+2)*(isqrt64(PM)+2)+PM)*2)+100;
  u64 sl=PM*4+100; if(sl<baselim) sl=baselim; if(sl<(1u<<24)) sl=1u<<24;
  small_sieve(sl); make_base(baselim);
  make_gp(PM);
  fprintf(stderr,"small table %llu, base primes %zu, gaussian primes %zu (norm<=%lld)\n",(unsigned long long)SMALLLIM,nbp,nGP,(long long)PM);
  stat_t sE={"E"},sF={"F"},sQ={"Q"};
  u64 W=1ULL<<wl;
  for(u64 S=NMIN;S<=NMAX;S+=W){
    u64 T=S+W; if(T>NMAX+1) T=NMAX+1; /* shell norms [S,T) */
    u64 rz=isqrt64(T)+1; u64 marg=2*rz*(isqrt64(PM)+1)+PM+16;
    if(!nosieve) seg_sieve(S>marg?S-marg:0, T+marg);
    u64 amin=isqrt64(S/2); u64 amax=isqrt64(T-1);
    for(u64 a=amin;a<=amax;a++){
      u64 a2=a*a; if(a2>=T) break;
      u64 blo= (S>a2)? isqrt64(S-a2-1)+1 : 0; if(S>a2 && blo*blo+a2<S) blo++;
      if(S==0) blo=0;
      u64 bhi=isqrt64(T-1-a2); if(bhi>a) bhi=a;
      if(blo>bhi) continue;
      if((blo+a)&1) blo++;
      for(u64 b=blo;b<=bhi;b+=2){
        i64 A=a,B=b; i64 nz=A*A+B*B; i64 dE=-9,dF=-9,dQ=-9;
        if(doE && nz>2){ sE.count++; size_t ix; i64 v=searchE(A,B,1,&ix);
          if(v<0){ i64 w=searchE(A,B,0,&ix); if(w<0){ dE=-1; sE.fails++; printf("FAIL E %lld %lld\n",(long long)A,(long long)B);} else { dE=-2; sE.nonstrict_only++; printf("E nonstrict-only %lld %lld\n",(long long)A,(long long)B);} }
          else { dE=v; addhist(&sE,v); addtop(&sE,v,A,B,GP[ix].x,GP[ix].y); addbin(&sE,v,A,B,GP[ix].x,GP[ix].y); } }
        if(doF && nz>10){ sF.count++; size_t ix; i64 v=searchF(A,B,&ix);
          if(v<0){ dF=-1; sF.fails++; printf("FAIL F %lld %lld\n",(long long)A,(long long)B); }
          else { dF=v; addhist(&sF,v); addtop(&sF,v,A,B,GP[ix].x,GP[ix].y); addbin(&sF,v,A,B,GP[ix].x,GP[ix].y); } }
        if(doQ && B>=2){ sQ.count++; i64 x,y; i64 v=searchQ(A,B,&x,&y);
          if(v<0){ dQ=-1; sQ.fails++; printf("FAIL Q %lld %lld\n",(long long)A,(long long)B); }
          else { dQ=v; addhist(&sQ,v); addtop(&sQ,v,A,B,x,y); addbin(&sQ,v,A,B,x,y); } }
        if(dump) printf("Z %lld %lld %lld %lld %lld\n",(long long)A,(long long)B,(long long)dE,(long long)dF,(long long)dQ);
      }
    }
    fprintf(stderr,"  shell to %llu done, %.1fs\n",(unsigned long long)T,(double)(clock()-t0)/CLOCKS_PER_SEC);
  }
  stat_t*ss[3]={&sE,&sF,&sQ}; int on[3]={doE,doF,doQ};
  printf("NMIN %llu NMAX %llu\n",(unsigned long long)NMIN,(unsigned long long)NMAX);
  for(int k=0;k<3;k++) if(on[k]){ stat_t*s=ss[k];
    printf("variant %s: targets %llu (fundamental domain), fails %llu, nonstrict_only %llu\n",s->name,(unsigned long long)s->count,(unsigned long long)s->fails,(unsigned long long)s->nonstrict_only);
    printf("  measure: %s\n", k<2? "min N(p1)":"min L1 = x+y of p1");
    for(int i=0;i<s->ntop;i++) printf("  top %d: z=%lld+%lldi N=%lld  value %lld  p1=%lld+%lldi\n",i+1,(long long)s->top[i].a,(long long)s->top[i].b,(long long)(s->top[i].a*s->top[i].a+s->top[i].b*s->top[i].b),(long long)s->top[i].v,(long long)s->top[i].px,(long long)s->top[i].py);
    for(int i=0;i<64;i++) if(s->bin[i].v) printf("  bin N in [2^%d,2^%d): max %lld at z=%lld+%lldi p1=%lld%+lldi\n",i,i+1,(long long)s->bin[i].v,(long long)s->bin[i].a,(long long)s->bin[i].b,(long long)s->bin[i].px,(long long)s->bin[i].py);
    printf("  hist log2(value):"); for(int i=0;i<64;i++) if(s->hist[i]) printf(" [%d]%llu",i,(unsigned long long)s->hist[i]); printf("\n"); }
  printf("MR calls %ld, cpu %.1fs\n",n_mr,(double)(clock()-t0)/CLOCKS_PER_SEC);
  return 0;
}
