/* gstopall.c: drop-below-start check for a gcz rule file that need not be unit-equivariant.
 * Like gstop.c, but every odd z = a+bi with B1 < N(z) <= B2 is a start (all four quadrants),
 * and an orbit counts as a cycle only if it returns to z itself.
 * If every start drops below its norm, then by induction every orbit of norm <= B2 reaches norm <= B1.
 * usage: gstopall rulefile B1 B2 [cap=10^7]
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef __int128 L; typedef long long ll;
static L AL[256][2], BE[256][2], DE[256][2], DN[256]; static int MOD=4;
#define ESC (((L)1)<<110)
static inline int res(L a, L b){ return (int)((a&(MOD-1))*MOD + (b&(MOD-1))); }
static inline int step(L *pa, L *pb){
  L a=*pa,b=*pb; int r=res(a,b);
  L x=AL[r][0]*a-AL[r][1]*b+BE[r][0], y=AL[r][0]*b+AL[r][1]*a+BE[r][1];
  L dr=DE[r][0], di=DE[r][1], n=DN[r];
  L u=x*dr+y*di, v=y*dr-x*di;
  if(u%n||v%n){fprintf(stderr,"bad rule\n");exit(2);}
  u/=n; v/=n;
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  *pa=u; *pb=v; return 0; }
int main(int argc,char**argv){
  if(argc<4){fprintf(stderr,"usage: gstopall rulefile B1 B2 [cap]\n");return 1;}
  FILE*f=fopen(argv[1],"r"); if(!f){perror("rule");return 1;} if(fscanf(f,"MOD %d",&MOD)!=1) MOD=4;
  for(int k=0;k<MOD*MOD;k++){int ra,rb; ll a1,a2,b1,b2,d1,d2;
    if(fscanf(f,"%d %d %lld %lld %lld %lld %lld %lld",&ra,&rb,&a1,&a2,&b1,&b2,&d1,&d2)!=8) return 1;
    int r=ra*MOD+rb; AL[r][0]=a1;AL[r][1]=a2;BE[r][0]=b1;BE[r][1]=b2;DE[r][0]=d1;DE[r][1]=d2;DN[r]=d1*d1+d2*d2;}
  ll B1=atoll(argv[2]), B2=atoll(argv[3]); ll cap=argc>4?atoll(argv[4]):10000000LL;
  ll R=(ll)floor(sqrt((double)B2))+1;
  ll n=0, fails=0, cycles=0, maxs=0, msa=0, msb=0; double maxe=0; ll mea=0, meb=0; double sums=0;
  for(ll a=-R;a<=R;a++) for(ll b=-R;b<=R;b++){
    ll n0=a*a+b*b; if(n0<=B1||n0>B2||((a+b)&1)==0) continue;
    n++;
    L x=a,y=b; ll s=0; double mx=0; int ok=0;
    while(s<cap){
      if(step(&x,&y)) break;
      s++;
      double dn=(double)x*(double)x+(double)y*(double)y; if(dn>mx) mx=dn;
      if(dn<1e36){ L nn=x*x+y*y; if(nn<(L)n0){ok=1;break;} if(x==a&&y==b){ok=2;break;} }
    }
    if(ok==2){ cycles++; printf("cycle-min %lld %lld steps %lld\n",a,b,s); continue; }
    if(!ok){ fails++; printf("FAIL %lld %lld steps %lld\n",a,b,s); fflush(stdout); continue; }
    sums+=s; if(s>maxs){maxs=s;msa=a;msb=b;}
    double e=log2(mx/(double)n0); if(e>maxe){maxe=e;mea=a;meb=b;}
  }
  printf("range (%lld,%lld] starts %lld fails %lld cycles %lld maxstop %lld at %lld %lld meanstop %.2f maxexc_log2 %.2f at %lld %lld\n",
    B1,B2,n,fails,cycles,maxs,msa,msb,sums/(double)(n>0?n:1),maxe,mea,meb);
  return 0; }
