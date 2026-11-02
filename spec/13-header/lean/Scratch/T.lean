import Header.Counts
open Header Header.Isa Header.Reg
def rows : List String := registry.flatMap fun (d, op, p, paths) => paths.map fun (pn, tr) =>
  let c := counts p tr
  s!"  (\"{d}\", \"{op}\", \"{pn}\", ⟨{c.instrs}, {c.branches}, {c.taken}, {c.jumps}, {c.loads}, {c.stores}⟩, {famCount p tr .x86}, {famCount p tr .arm}, {famCount p tr .rv}, {branchLog p (tr ++ [p.length])})"
#eval IO.println (",\n".intercalate rows)
