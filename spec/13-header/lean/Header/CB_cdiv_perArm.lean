import Header.CB_cdiv_run_perArm_rr
import Header.CB_cdiv_run_perArm_ri
import Header.CB_cdiv_run_perArm_ir
import Header.CB_cdiv_run_perArm_ii
/-! Complex division of Design B, one file per form (memory). -/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PB
open Isa

theorem cdiv_run_perArm (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat) (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3) (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
    (hx : x1 = x3 ∨ x0 = x2) (hy : y1 = y3 ∨ y0 = y2)
    (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W)
    (g0 : y0 < W) (g1 : y1 < W) (g2 : y2 < W) (g3 : y3 < W) (hz : z + 24 < W) :
    ∃ s, exe (cdiv .perArm) (regs3 x y z) mem = .done s ∧
      s.trace = cdivTrace .perArm (decide (x0 < x2)) (decide (x1 < x3)) (decide (y0 < y2)) (decide (y1 < y3)) ∧
      s.stores = [(z, 0), (z + 8, 0), (z + 16, 0), (z + 24, 0),
        (z + 8 * (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).d.val,
          (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).m)] := by
  rcases (by omega : x1 = x3 ∨ (x1 ≠ x3 ∧ x0 = x2)) with hxa | hxa <;>
    rcases (by omega : y1 = y3 ∨ (y1 ≠ y3 ∧ y0 = y2)) with hya | hya
  · exact cdiv_run_perArm_rr x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz hxa hya
  · exact cdiv_run_perArm_ri x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz hxa hya
  · exact cdiv_run_perArm_ir x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz hxa hya
  · exact cdiv_run_perArm_ii x y z mem x0 x1 x2 x3 y0 y1 y2 y3 ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3 hx hy h0 h1 h2 h3 g0 g1 g2 g3 hz hxa hya

end PB
end Header
