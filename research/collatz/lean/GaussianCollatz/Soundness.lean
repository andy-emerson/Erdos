import GaussianCollatz.Basic
namespace GInt
theorem drop_below_start_sound (T : GInt → GInt)
    (hdiv : ∀ z, pi ∣ z → T z = divPi z)
    (hunit : ∀ u z, IsUnit u → T (u * z) = u * T z)
    (N0 B : Nat)
    (hcheck : ∀ z : GInt, Odd z → 0 < z.re → 0 ≤ z.im → N0 < N z → N z ≤ B →
      ∃ n, N (iter T n z) < N z) :
    ∀ z : GInt, N z ≤ B → ∃ n, N (iter T n z) ≤ N0 := sorry
end GInt
