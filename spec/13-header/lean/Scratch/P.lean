import Header.Counts
open Header Header.Isa Header.Reg
#eval do
  for (d, op, p, paths) in registry do
    for (pn, tr) in paths do
      let c := counts p tr
      IO.println s!"{d} | {op} | {pn} | {c.instrs} | {c.branches} | {c.taken} | {c.jumps} | {c.loads} | {c.stores} | {famCount p tr .x86} | {famCount p tr .arm} | {famCount p tr .rv} | {branchLog p (tr ++ [p.length])} | {loadsFirst p tr} | {Forward p}"
