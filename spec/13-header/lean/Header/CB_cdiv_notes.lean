import Header.CorrectB
/-! Complex division of Design B, one file per form (memory). -/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PB
open Isa

theorem cdiv_run_notes (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat) (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3) (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
    (hx : x1 = x3 ∨ x0 = x2) (hy : y1 = y3 ∨ y0 = y2)
    (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W)
    (g0 : y0 < W) (g1 : y1 < W) (g2 : y2 < W) (g3 : y3 < W) (hz : z + 24 < W) :
    ∃ s, exe (cdiv .notes) (regs3 x y z) mem = .done s ∧
      s.trace = cdivTrace .notes (decide (x0 < x2)) (decide (x1 < x3)) (decide (y0 < y2)) (decide (y1 < y3)) ∧
      s.stores = [(z, 0), (z + 8, 0), (z + 16, 0), (z + 24, 0),
        (z + 8 * (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).d.val,
          (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).m)] := by
  rcases lt_cases x0 x2 with ⟨a, a', a''⟩ | ⟨a, a', a''⟩ <;>
    rcases lt_cases x1 x3 with ⟨b, b', b''⟩ | ⟨b, b', b''⟩ <;>
    rcases lt_cases y0 y2 with ⟨c, c', c''⟩ | ⟨c, c', c''⟩ <;>
    rcases lt_cases y1 y3 with ⟨d, d', d''⟩ | ⟨d, d', d''⟩
  all_goals try (have p1 : x3 < x1 := by omega)
  all_goals try (have p2 : y3 < y1 := by omega)
  all_goals rcases (by omega : x1 = x3 ∨ (x1 ≠ x3 ∧ x0 = x2)) with hx13 | ⟨hx13, hx02⟩ <;>
    rcases (by omega : y1 = y3 ∨ (y1 ≠ y3 ∧ y0 = y2)) with hy13 | ⟨hy13, hy02⟩
  all_goals try (exfalso; omega)
  all_goals (try subst hx13) <;> (try subst hx02) <;> (try subst hy13) <;> (try subst hy02)
  all_goals run_prog [cdiv, cdivTrace, cdivColl, cdivCollTrace, cdivCollLen, divColl, divCollTrace, divCollLen,
    ld8, regs3, ex0, ex1, ex2, ex3, ey0, ey1, ey2, ey3, List.range, List.range.loop, wsltu,
    wsub_of_le h0, wsub_of_le h1, wsub_of_le h2, wsub_of_le h3, wsub_of_le g0, wsub_of_le g1, wsub_of_le g2,
    wsub_of_le g3, wneg_wsub h0 h2, wneg_wsub h1 h3, wneg_wsub g0 g2, wneg_wsub g1 g3, Nat.sub_pos_iff_lt,
    toA, A.div, absDiff, wdivu,
    wsub_0_0, wsub_0_1, wsub_0_2, wsub_0_3, wsub_1_0, wsub_1_1, wsub_1_2, wsub_1_3, wsub_2_0, wsub_2_1, wsub_2_2, wsub_2_3, wsub_3_0, wsub_3_1, wsub_3_2, wsub_3_3, mod_W_small, wadd_addr _ hz, *]

end PB
end Header
