example (x y : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
  (r : x0 < x2) (h0 : x0 < 18446744073709551616) (h1 : x1 < 18446744073709551616) (h2 : x2 < 18446744073709551616) (h3 : x3 < 18446744073709551616) (i : ¬ x1 < x3): x2 ≤ x0 := by omega
