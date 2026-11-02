#check @Int.mul_tdiv_cancel
#check @Int.mul_emod
#check @Int.emod_emod_of_dvd
#check @Int.toNat_of_nonneg
#check @Int.neg_tdiv
#check @Int.tdiv_neg
#check @Nat.and_two_pow_sub_one_eq_mod
#check @Nat.add_mod
#check @Nat.mul_mod
#check @Int.emod_emod
example (a b : Int) (W : Nat) : True := trivial
example (a b : Int) : ((a % 18446744073709551616).toNat + (b % 18446744073709551616).toNat) % 18446744073709551616 = ((a+b) % 18446744073709551616).toNat := by omega
