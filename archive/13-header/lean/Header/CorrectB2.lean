import Header.CB_cdiv_notes
import Header.CB_cdiv_perArm
/-! Design B's complex division, both forms. -/

set_option linter.unusedVariables false

namespace Header
namespace PB
open Isa

/-- **Complex exact division, both forms.** For `x` and `y` on axes, the program stores 0 to the four slots, then
round 1's `A.div` of the two readings to its slot (`cdiv_value` gives its value). The path is set by the four borrows. -/
theorem cdiv_run (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat) (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3) (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
    (f : Form) (hx : x1 = x3 ∨ x0 = x2) (hy : y1 = y3 ∨ y0 = y2)
    (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W)
    (g0 : y0 < W) (g1 : y1 < W) (g2 : y2 < W) (g3 : y3 < W) (hz : z + 24 < W) :
    ∃ s, exe (cdiv f) (regs3 x y z) mem = .done s ∧
      s.trace = cdivTrace f (decide (x0 < x2)) (decide (x1 < x3)) (decide (y0 < y2)) (decide (y1 < y3)) ∧
      s.stores = [(z, 0), (z + 8, 0), (z + 16, 0), (z + 24, 0),
        (z + 8 * (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).d.val,
          (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).m)] := by
  cases f
  · exact cdiv_run_notes x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz
  · exact cdiv_run_perArm x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz

end PB
end Header
