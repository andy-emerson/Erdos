/* zany.c: integer rule x -> (m x + d(x))/2^v with an arbitrary digit table, literal cycles.
 * usage: zany k m B d_1 d_3 ... d_(2^k-1)   (d_c is the digit for odd x with -m x = c mod 2^k;
 *        it must be odd and = c mod 2^k)
 * Follows every odd x with 0 < |x| <= B.  Prints: k m B ncycles unresolved ok | min:len:unit ...
 * ok = 1 when every start reached a cycle and every cycle contains 1 or -1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef __int128 L;
static int K; static L M; static L DG[1<<12]; static long B;
#define ESC (((L)1)<<100)
static inline L S(L x){ L r = (-(M*x)) & (((L)1<<K)-1); L y = M*x + DG[r];
  if (y == 0) return 0; while ((y & 1) == 0) y >>= 1; return y; }
#define MAXC 100000
static L cmin[MAXC]; static long clen[MAXC]; static int cunit[MAXC]; static int nc=0;
static int addcyc(L x){ /* x on a cycle: canonical min |.| (ties: positive) */
  L mn=x, y=x; long len=0; int u=0; do { L a=y<0?-y:y, b=mn<0?-mn:mn; if(a<b||(a==b&&y>mn)) mn=y; if(a==1)u=1; y=S(y); len++; } while(y!=x && len<10000000);
  for(int i=0;i<nc;i++) if(cmin[i]==mn) return i;
  if(nc>=MAXC){fprintf(stderr,"too many cycles\n");exit(3);} cmin[nc]=mn; clen[nc]=len; cunit[nc]=u; return nc++; }
int main(int argc,char**argv){
  K=atoi(argv[1]); M=atoll(argv[2]); B=atol(argv[3]);
  int n=1<<(K-1); if(argc<4+n){fprintf(stderr,"need %d digits\n",n);return 1;}
  for(int j=0;j<n;j++){ long d=atol(argv[4+j]); int c=2*j+1; if((((d-c)%(1<<K))+(1<<K))%(1<<K)){fprintf(stderr,"digit %ld not in class %d\n",d,c);return 1;} DG[c]=d; }
  long W=2*B+1; int *st=malloc(W*sizeof(int)); /* -1 unknown, -2 on current path, -3 unresolved, >=0 cycle id */
  for(long i=0;i<W;i++) st[i]=-1;
  long cap=1000000; L *path=malloc((cap+2)*sizeof(L)); long unres=0;
  for(long x0=-B;x0<=B;x0++){ if(!(x0&1)) continue; if(st[x0+B]!=-1) continue;
    long len=0; L x=x0; int res=-4; L bs=x; long pw=1, lam=0;
    while(1){
      if(x>=-B && x<=B){ int s=st[(long)x+B]; if(s>=0||s==-3){res=s;break;} if(s==-2){res=addcyc(x);break;} st[(long)x+B]=-2; }
      path[len++]=x; if(len>cap){res=-3;break;}
      x=S(x); if(x>ESC||x<-ESC){res=-3;break;}
      if(x==bs){res=addcyc(x);break;} lam++; if(lam==pw){bs=x;pw*=2;lam=0;}
    }
    if(res==-3) unres++;
    for(long j=0;j<len;j++) if(path[j]>=-B&&path[j]<=B) st[(long)path[j]+B]=res;
  }
  int ok = unres==0; for(int i=0;i<nc;i++) if(!cunit[i]) ok=0;
  printf("%d %lld %ld %d %ld %d |",K,(long long)M,B,nc,unres,ok);
  for(int i=0;i<nc&&i<20;i++) printf(" %lld:%ld:%d",(long long)cmin[i],clen[i],cunit[i]);
  printf("\n"); return 0; }
