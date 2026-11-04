/* gstop.c: stopping-time verification for a unit-equivariant map given as a gcz rule file.
 * For every z = a+bi with a > 0, b >= 0 (one associate per class), z odd, B1 < N(z) <= B2:
 * iterate the raw map T until N drops below N(z).  If that happens for every z in (0, B2] the
 * induction "every z reaches a smaller norm" proves every orbit of norm <= B2 ends in a cycle
 * whose least element has norm <= B1 (those were classified by gcz).
 * Reports: starts, failures (cap or escape, printed), cycles met (orbit returns to an associate
 * of z without dropping), max stopping time (raw T steps) and max excursion log2(Nmax/N(z)).
 * usage: gstop rulefile B1 B2 [cap=10^7]
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef __int128 L; typedef long long ll;
static L AL[64][2], BE[64][2], DE[64][2], DN[64]; static int MOD=4;
#define ESC (((L)1)<<110)
static inline int res(L a, L b){ return (int)((a&(MOD-1))*MOD + (b&(MOD-1))); }
static inline int step(L *pa, L *pb){
  L a=*pa,b=*pb; int r=res(a,b);
  L x=AL[r][0]*a-AL[r][1]*b+BE[r][0], y=AL[r][0]*b+AL[r][1]*a+BE[r][1];
  L dr=DE[r][0], di=DE[r][1], n=DN[r];
  L u=x*dr+y*di, v=y*dr-x*di;
  if(n==2){ u/=2; v/=2; } else if(n!=1){ u/=n; v/=n; }
  if(u>ESC||u<-ESC||v>ESC||v<-ESC) return 1;
  *pa=u; *pb=v; return 0; }
static inline double nd(L a, L b){ return (double)a*(double)a+(double)b*(double)b; }
int main(int argc,char**argv){
  FILE*f=fopen(argv[1],"r"); if(fscanf(f,"MOD %d",&MOD)!=1) MOD=4;
  for(int k=0;k<MOD*MOD;k++){int ra,rb; ll a1,a2,b1,b2,d1,d2;
    if(fscanf(f,"%d %d %lld %lld %lld %lld %lld %lld",&ra,&rb,&a1,&a2,&b1,&b2,&d1,&d2)!=8) return 1;
    int r=ra*MOD+rb; AL[r][0]=a1;AL[r][1]=a2;BE[r][0]=b1;BE[r][1]=b2;DE[r][0]=d1;DE[r][1]=d2;DN[r]=d1*d1+d2*d2;}
  ll B1=atoll(argv[2]), B2=atoll(argv[3]); ll cap=argc>4?atoll(argv[4]):10000000LL;
  ll amax=(ll)floor(sqrt((double)B2));
  ll n=0, fails=0, cycles=0, maxs=0, msa=0, msb=0; double maxe=0; ll mea=0, meb=0; double sums=0;
  for(ll a=1;a<=amax;a++){
    ll b2max=B2-a*a; ll bmax=(ll)floor(sqrt((double)b2max)); while(bmax*bmax>b2max)bmax--; while((bmax+1)*(bmax+1)<=b2max)bmax++;
    ll bmin=0; ll lo=B1-a*a; if(lo>=0){ bmin=(ll)floor(sqrt((double)lo)); while(bmin*bmin<=lo)bmin++; while(bmin>0&&(bmin-1)*(bmin-1)>lo)bmin--; }
    for(ll b=bmin;b<=bmax;b++){
      if(((a+b)&1)==0) continue;
      n++;
      L x=a,y=b; L n0=(L)a*a+(L)b*b; ll s=0; double mx=0; int ok=0;
      while(s<cap){
        if(step(&x,&y)){break;}
        s++;
        L nn=x*x+y*y;  /* exact while |x|,|y| < 2^63 */
        double dn=nd(x,y); if(dn>mx) mx=dn;
        if(dn<1e36){ if(nn<n0){ok=1;break;}
          if(nn==n0 && ((x==a&&y==b)||(x==-b&&y==a)||(x==-a&&y==-b)||(x==b&&y==-a))){ok=2;break;} }
      }
      if(ok==2){ cycles++; printf("cycle-min %lld %lld steps %lld\n",a,b,s); continue; }
      if(!ok){ fails++; printf("FAIL %lld %lld steps %lld\n",a,b,s); fflush(stdout); continue; }
      sums+=s; if(s>maxs){maxs=s;msa=a;msb=b;}
      double e=log2(mx/(double)n0); if(e>maxe){maxe=e;mea=a;meb=b;}
    }
  }
  printf("range (%lld,%lld] starts %lld fails %lld cycles %lld maxstop %lld at %lld %lld meanstop %.2f maxexc_log2 %.2f at %lld %lld\n",
    B1,B2,n,fails,cycles,maxs,msa,msb,sums/(double)(n>0?n:1),maxe,mea,meb);
  return 0; }
