from alg import *
import sys
R=int(sys.argv[1])
mx=0; arg=None; tot=0; n=0; fails=0
for a0 in range(-R,R+1):
  for a1 in range(-R,R+1):
    for b0 in range(-R,R+1):
      for b1 in range(-R,R+1):
        if (b0+b1)%2==0 or (a0,a1)==(0,0): continue
        st=[]
        try: sym_S((a0,a1),(b0,b1),st,cap=2000)
        except RuntimeError: fails+=1; print("cycle?",(a0,a1),(b0,b1)); continue
        n+=1; tot+=st[0]
        if st[0]>mx: mx=st[0]; arg=((a0,a1),(b0,b1))
print(R,"pairs",n,"mean",tot/n,"max",mx,arg,"nonterm",fails)
