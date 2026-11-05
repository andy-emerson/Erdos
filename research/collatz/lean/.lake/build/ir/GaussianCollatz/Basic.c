// Lean compiler output
// Module: GaussianCollatz.Basic
// Imports: public import Init public meta import Init
#include <lean/lean.h>
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-label"
#elif defined(__GNUC__) && !defined(__CLANG__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#ifdef __cplusplus
extern "C" {
#endif
lean_object* lean_nat_to_int(lean_object*);
lean_object* lean_string_length(lean_object*);
uint8_t lean_int_dec_lt(lean_object*, lean_object*);
lean_object* l_Int_repr(lean_object*);
lean_object* l_Repr_addAppParen(lean_object*, lean_object*);
lean_object* lean_int_add(lean_object*, lean_object*);
lean_object* lean_int_ediv(lean_object*, lean_object*);
lean_object* lean_int_sub(lean_object*, lean_object*);
lean_object* lean_int_mul(lean_object*, lean_object*);
lean_object* lean_int_emod(lean_object*, lean_object*);
uint8_t lean_int_dec_eq(lean_object*, lean_object*);
uint8_t lean_nat_dec_eq(lean_object*, lean_object*);
lean_object* lean_nat_sub(lean_object*, lean_object*);
lean_object* lean_int_neg(lean_object*);
lean_object* lean_nat_abs(lean_object*);
lean_object* lean_nat_mul(lean_object*, lean_object*);
lean_object* lean_nat_add(lean_object*, lean_object*);
LEAN_EXPORT uint8_t lp_GaussianCollatz_instDecidableEqGInt_decEq(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_instDecidableEqGInt_decEq___boxed(lean_object*, lean_object*);
LEAN_EXPORT uint8_t lp_GaussianCollatz_instDecidableEqGInt(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_instDecidableEqGInt___boxed(lean_object*, lean_object*);
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 3, .m_capacity = 3, .m_length = 2, .m_data = "{ "};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__0 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__0_value;
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__1_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 3, .m_capacity = 3, .m_length = 2, .m_data = "re"};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__1 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__1_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__2_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__1_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__2 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__2_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__3_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*2 + 0, .m_other = 2, .m_tag = 5}, .m_objs = {((lean_object*)(((size_t)(0) << 1) | 1)),((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__2_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__3 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__3_value;
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__4_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 5, .m_capacity = 5, .m_length = 4, .m_data = " := "};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__4 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__4_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__5_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__4_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__5 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__5_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__6_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*2 + 0, .m_other = 2, .m_tag = 5}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__3_value),((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__5_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__6 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__6_value;
static lean_once_cell_t lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7;
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__8_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 3, .m_capacity = 3, .m_length = 2, .m_data = " }"};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__8 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__8_value;
static lean_once_cell_t lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9;
static lean_once_cell_t lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__11_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__0_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__11 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__11_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__12_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__8_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__12 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__12_value;
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__13_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 2, .m_capacity = 2, .m_length = 1, .m_data = ","};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__13 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__13_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__14_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__13_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__14 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__14_value;
static const lean_string_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__15_value = {.m_header = {.m_rc = 0, .m_cs_sz = 0, .m_other = 0, .m_tag = 249}, .m_size = 3, .m_capacity = 3, .m_length = 2, .m_data = "im"};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__15 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__15_value;
static const lean_ctor_object lp_GaussianCollatz_instReprGInt_repr___redArg___closed__16_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_ctor_object) + sizeof(void*)*1 + 0, .m_other = 1, .m_tag = 3}, .m_objs = {((lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__15_value)}};
static const lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__16 = (const lean_object*)&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__16_value;
static lean_once_cell_t lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17;
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr___boxed(lean_object*, lean_object*);
static const lean_closure_object lp_GaussianCollatz_instReprGInt___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_closure_object) + sizeof(void*)*0, .m_other = 0, .m_tag = 245}, .m_fun = (void*)lp_GaussianCollatz_instReprGInt_repr___boxed, .m_arity = 2, .m_num_fixed = 0, .m_objs = {} };
static const lean_object* lp_GaussianCollatz_instReprGInt___closed__0 = (const lean_object*)&lp_GaussianCollatz_instReprGInt___closed__0_value;
LEAN_EXPORT const lean_object* lp_GaussianCollatz_instReprGInt = (const lean_object*)&lp_GaussianCollatz_instReprGInt___closed__0_value;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instAdd___lam__0(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instAdd___lam__0___boxed(lean_object*, lean_object*);
static const lean_closure_object lp_GaussianCollatz_GInt_instAdd___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_closure_object) + sizeof(void*)*0, .m_other = 0, .m_tag = 245}, .m_fun = (void*)lp_GaussianCollatz_GInt_instAdd___lam__0___boxed, .m_arity = 2, .m_num_fixed = 0, .m_objs = {} };
static const lean_object* lp_GaussianCollatz_GInt_instAdd___closed__0 = (const lean_object*)&lp_GaussianCollatz_GInt_instAdd___closed__0_value;
LEAN_EXPORT const lean_object* lp_GaussianCollatz_GInt_instAdd = (const lean_object*)&lp_GaussianCollatz_GInt_instAdd___closed__0_value;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instNeg___lam__0(lean_object*);
static const lean_closure_object lp_GaussianCollatz_GInt_instNeg___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_closure_object) + sizeof(void*)*0, .m_other = 0, .m_tag = 245}, .m_fun = (void*)lp_GaussianCollatz_GInt_instNeg___lam__0, .m_arity = 1, .m_num_fixed = 0, .m_objs = {} };
static const lean_object* lp_GaussianCollatz_GInt_instNeg___closed__0 = (const lean_object*)&lp_GaussianCollatz_GInt_instNeg___closed__0_value;
LEAN_EXPORT const lean_object* lp_GaussianCollatz_GInt_instNeg = (const lean_object*)&lp_GaussianCollatz_GInt_instNeg___closed__0_value;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instSub___lam__0(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instSub___lam__0___boxed(lean_object*, lean_object*);
static const lean_closure_object lp_GaussianCollatz_GInt_instSub___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_closure_object) + sizeof(void*)*0, .m_other = 0, .m_tag = 245}, .m_fun = (void*)lp_GaussianCollatz_GInt_instSub___lam__0___boxed, .m_arity = 2, .m_num_fixed = 0, .m_objs = {} };
static const lean_object* lp_GaussianCollatz_GInt_instSub___closed__0 = (const lean_object*)&lp_GaussianCollatz_GInt_instSub___closed__0_value;
LEAN_EXPORT const lean_object* lp_GaussianCollatz_GInt_instSub = (const lean_object*)&lp_GaussianCollatz_GInt_instSub___closed__0_value;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instMul___lam__0(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instMul___lam__0___boxed(lean_object*, lean_object*);
static const lean_closure_object lp_GaussianCollatz_GInt_instMul___closed__0_value = {.m_header = {.m_rc = 0, .m_cs_sz = sizeof(lean_closure_object) + sizeof(void*)*0, .m_other = 0, .m_tag = 245}, .m_fun = (void*)lp_GaussianCollatz_GInt_instMul___lam__0___boxed, .m_arity = 2, .m_num_fixed = 0, .m_objs = {} };
static const lean_object* lp_GaussianCollatz_GInt_instMul___closed__0 = (const lean_object*)&lp_GaussianCollatz_GInt_instMul___closed__0_value;
LEAN_EXPORT const lean_object* lp_GaussianCollatz_GInt_instMul = (const lean_object*)&lp_GaussianCollatz_GInt_instMul___closed__0_value;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instOfNat(lean_object*);
static lean_once_cell_t lp_GaussianCollatz_GInt_I___closed__0_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_GInt_I___closed__0;
static lean_once_cell_t lp_GaussianCollatz_GInt_I___closed__1_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_GInt_I___closed__1;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_I;
static lean_once_cell_t lp_GaussianCollatz_GInt_pi___closed__0_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_GInt_pi___closed__0;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_pi;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_N(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_N___boxed(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instDvd;
static lean_once_cell_t lp_GaussianCollatz_GInt_pow___closed__0_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_GInt_pow___closed__0;
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_pow(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_pow___boxed(lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___redArg(lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___redArg___boxed(lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter(lean_object*, lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___boxed(lean_object*, lean_object*, lean_object*, lean_object*);
static lean_once_cell_t lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0_once = LEAN_ONCE_CELL_INITIALIZER;
static lean_object* lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0;
LEAN_EXPORT uint8_t lp_GaussianCollatz_GInt_instDecidableDvdPi(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instDecidableDvdPi___boxed(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_divPi(lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_iter(lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___redArg(lean_object*, lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___redArg___boxed(lean_object*, lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter(lean_object*, lean_object*, lean_object*, lean_object*, lean_object*);
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___boxed(lean_object*, lean_object*, lean_object*, lean_object*, lean_object*);
LEAN_EXPORT uint8_t lp_GaussianCollatz_instDecidableEqGInt_decEq(lean_object* v_x_1_, lean_object* v_x_2_){
_start:
{
lean_object* v_re_3_; lean_object* v_im_4_; lean_object* v_re_5_; lean_object* v_im_6_; uint8_t v___x_7_; 
v_re_3_ = lean_ctor_get(v_x_1_, 0);
v_im_4_ = lean_ctor_get(v_x_1_, 1);
v_re_5_ = lean_ctor_get(v_x_2_, 0);
v_im_6_ = lean_ctor_get(v_x_2_, 1);
v___x_7_ = lean_int_dec_eq(v_re_3_, v_re_5_);
if (v___x_7_ == 0)
{
return v___x_7_;
}
else
{
uint8_t v___x_8_; 
v___x_8_ = lean_int_dec_eq(v_im_4_, v_im_6_);
return v___x_8_;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_instDecidableEqGInt_decEq___boxed(lean_object* v_x_9_, lean_object* v_x_10_){
_start:
{
uint8_t v_res_11_; lean_object* v_r_12_; 
v_res_11_ = lp_GaussianCollatz_instDecidableEqGInt_decEq(v_x_9_, v_x_10_);
lean_dec_ref(v_x_10_);
lean_dec_ref(v_x_9_);
v_r_12_ = lean_box(v_res_11_);
return v_r_12_;
}
}
LEAN_EXPORT uint8_t lp_GaussianCollatz_instDecidableEqGInt(lean_object* v_x_13_, lean_object* v_x_14_){
_start:
{
uint8_t v___x_15_; 
v___x_15_ = lp_GaussianCollatz_instDecidableEqGInt_decEq(v_x_13_, v_x_14_);
return v___x_15_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_instDecidableEqGInt___boxed(lean_object* v_x_16_, lean_object* v_x_17_){
_start:
{
uint8_t v_res_18_; lean_object* v_r_19_; 
v_res_18_ = lp_GaussianCollatz_instDecidableEqGInt(v_x_16_, v_x_17_);
lean_dec_ref(v_x_17_);
lean_dec_ref(v_x_16_);
v_r_19_ = lean_box(v_res_18_);
return v_r_19_;
}
}
static lean_object* _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7(void){
_start:
{
lean_object* v___x_33_; lean_object* v___x_34_; 
v___x_33_ = lean_unsigned_to_nat(6u);
v___x_34_ = lean_nat_to_int(v___x_33_);
return v___x_34_;
}
}
static lean_object* _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9(void){
_start:
{
lean_object* v___x_36_; lean_object* v___x_37_; 
v___x_36_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__0));
v___x_37_ = lean_string_length(v___x_36_);
return v___x_37_;
}
}
static lean_object* _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10(void){
_start:
{
lean_object* v___x_38_; lean_object* v___x_39_; 
v___x_38_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__9);
v___x_39_ = lean_nat_to_int(v___x_38_);
return v___x_39_;
}
}
static lean_object* _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17(void){
_start:
{
lean_object* v___x_50_; lean_object* v___x_51_; 
v___x_50_ = lean_unsigned_to_nat(0u);
v___x_51_ = lean_nat_to_int(v___x_50_);
return v___x_51_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr___redArg(lean_object* v_x_52_){
_start:
{
lean_object* v_re_53_; lean_object* v_im_54_; lean_object* v___x_56_; uint8_t v_isShared_57_; uint8_t v_isSharedCheck_106_; 
v_re_53_ = lean_ctor_get(v_x_52_, 0);
v_im_54_ = lean_ctor_get(v_x_52_, 1);
v_isSharedCheck_106_ = !lean_is_exclusive(v_x_52_);
if (v_isSharedCheck_106_ == 0)
{
v___x_56_ = v_x_52_;
v_isShared_57_ = v_isSharedCheck_106_;
goto v_resetjp_55_;
}
else
{
lean_inc(v_im_54_);
lean_inc(v_re_53_);
lean_dec(v_x_52_);
v___x_56_ = lean_box(0);
v_isShared_57_ = v_isSharedCheck_106_;
goto v_resetjp_55_;
}
v_resetjp_55_:
{
lean_object* v___x_58_; lean_object* v___x_59_; lean_object* v___x_60_; uint8_t v___y_62_; lean_object* v___y_63_; lean_object* v___y_64_; lean_object* v___y_78_; lean_object* v___x_98_; lean_object* v___x_99_; uint8_t v___x_100_; 
v___x_58_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__5));
v___x_59_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__6));
v___x_60_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__7);
v___x_98_ = lean_unsigned_to_nat(0u);
v___x_99_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_100_ = lean_int_dec_lt(v_re_53_, v___x_99_);
if (v___x_100_ == 0)
{
lean_object* v___x_101_; lean_object* v___x_102_; 
v___x_101_ = l_Int_repr(v_re_53_);
lean_dec(v_re_53_);
v___x_102_ = lean_alloc_ctor(3, 1, 0);
lean_ctor_set(v___x_102_, 0, v___x_101_);
v___y_78_ = v___x_102_;
goto v___jp_77_;
}
else
{
lean_object* v___x_103_; lean_object* v___x_104_; lean_object* v___x_105_; 
v___x_103_ = l_Int_repr(v_re_53_);
lean_dec(v_re_53_);
v___x_104_ = lean_alloc_ctor(3, 1, 0);
lean_ctor_set(v___x_104_, 0, v___x_103_);
v___x_105_ = l_Repr_addAppParen(v___x_104_, v___x_98_);
v___y_78_ = v___x_105_;
goto v___jp_77_;
}
v___jp_61_:
{
lean_object* v___x_66_; 
if (v_isShared_57_ == 0)
{
lean_ctor_set_tag(v___x_56_, 4);
lean_ctor_set(v___x_56_, 1, v___y_64_);
lean_ctor_set(v___x_56_, 0, v___x_60_);
v___x_66_ = v___x_56_;
goto v_reusejp_65_;
}
else
{
lean_object* v_reuseFailAlloc_76_; 
v_reuseFailAlloc_76_ = lean_alloc_ctor(4, 2, 0);
lean_ctor_set(v_reuseFailAlloc_76_, 0, v___x_60_);
lean_ctor_set(v_reuseFailAlloc_76_, 1, v___y_64_);
v___x_66_ = v_reuseFailAlloc_76_;
goto v_reusejp_65_;
}
v_reusejp_65_:
{
lean_object* v___x_67_; lean_object* v___x_68_; lean_object* v___x_69_; lean_object* v___x_70_; lean_object* v___x_71_; lean_object* v___x_72_; lean_object* v___x_73_; lean_object* v___x_74_; lean_object* v___x_75_; 
v___x_67_ = lean_alloc_ctor(6, 1, 1);
lean_ctor_set(v___x_67_, 0, v___x_66_);
lean_ctor_set_uint8(v___x_67_, sizeof(void*)*1, v___y_62_);
v___x_68_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_68_, 0, v___y_63_);
lean_ctor_set(v___x_68_, 1, v___x_67_);
v___x_69_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__10);
v___x_70_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__11));
v___x_71_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_71_, 0, v___x_70_);
lean_ctor_set(v___x_71_, 1, v___x_68_);
v___x_72_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__12));
v___x_73_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_73_, 0, v___x_71_);
lean_ctor_set(v___x_73_, 1, v___x_72_);
v___x_74_ = lean_alloc_ctor(4, 2, 0);
lean_ctor_set(v___x_74_, 0, v___x_69_);
lean_ctor_set(v___x_74_, 1, v___x_73_);
v___x_75_ = lean_alloc_ctor(6, 1, 1);
lean_ctor_set(v___x_75_, 0, v___x_74_);
lean_ctor_set_uint8(v___x_75_, sizeof(void*)*1, v___y_62_);
return v___x_75_;
}
}
v___jp_77_:
{
lean_object* v___x_79_; uint8_t v___x_80_; lean_object* v___x_81_; lean_object* v___x_82_; lean_object* v___x_83_; lean_object* v___x_84_; lean_object* v___x_85_; lean_object* v___x_86_; lean_object* v___x_87_; lean_object* v___x_88_; lean_object* v___x_89_; lean_object* v___x_90_; lean_object* v___x_91_; uint8_t v___x_92_; 
v___x_79_ = lean_alloc_ctor(4, 2, 0);
lean_ctor_set(v___x_79_, 0, v___x_60_);
lean_ctor_set(v___x_79_, 1, v___y_78_);
v___x_80_ = 0;
v___x_81_ = lean_alloc_ctor(6, 1, 1);
lean_ctor_set(v___x_81_, 0, v___x_79_);
lean_ctor_set_uint8(v___x_81_, sizeof(void*)*1, v___x_80_);
v___x_82_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_82_, 0, v___x_59_);
lean_ctor_set(v___x_82_, 1, v___x_81_);
v___x_83_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__14));
v___x_84_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_84_, 0, v___x_82_);
lean_ctor_set(v___x_84_, 1, v___x_83_);
v___x_85_ = lean_box(1);
v___x_86_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_86_, 0, v___x_84_);
lean_ctor_set(v___x_86_, 1, v___x_85_);
v___x_87_ = ((lean_object*)(lp_GaussianCollatz_instReprGInt_repr___redArg___closed__16));
v___x_88_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_88_, 0, v___x_86_);
lean_ctor_set(v___x_88_, 1, v___x_87_);
v___x_89_ = lean_alloc_ctor(5, 2, 0);
lean_ctor_set(v___x_89_, 0, v___x_88_);
lean_ctor_set(v___x_89_, 1, v___x_58_);
v___x_90_ = lean_unsigned_to_nat(0u);
v___x_91_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_92_ = lean_int_dec_lt(v_im_54_, v___x_91_);
if (v___x_92_ == 0)
{
lean_object* v___x_93_; lean_object* v___x_94_; 
v___x_93_ = l_Int_repr(v_im_54_);
lean_dec(v_im_54_);
v___x_94_ = lean_alloc_ctor(3, 1, 0);
lean_ctor_set(v___x_94_, 0, v___x_93_);
v___y_62_ = v___x_80_;
v___y_63_ = v___x_89_;
v___y_64_ = v___x_94_;
goto v___jp_61_;
}
else
{
lean_object* v___x_95_; lean_object* v___x_96_; lean_object* v___x_97_; 
v___x_95_ = l_Int_repr(v_im_54_);
lean_dec(v_im_54_);
v___x_96_ = lean_alloc_ctor(3, 1, 0);
lean_ctor_set(v___x_96_, 0, v___x_95_);
v___x_97_ = l_Repr_addAppParen(v___x_96_, v___x_90_);
v___y_62_ = v___x_80_;
v___y_63_ = v___x_89_;
v___y_64_ = v___x_97_;
goto v___jp_61_;
}
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr(lean_object* v_x_107_, lean_object* v_prec_108_){
_start:
{
lean_object* v___x_109_; 
v___x_109_ = lp_GaussianCollatz_instReprGInt_repr___redArg(v_x_107_);
return v___x_109_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_instReprGInt_repr___boxed(lean_object* v_x_110_, lean_object* v_prec_111_){
_start:
{
lean_object* v_res_112_; 
v_res_112_ = lp_GaussianCollatz_instReprGInt_repr(v_x_110_, v_prec_111_);
lean_dec(v_prec_111_);
return v_res_112_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instAdd___lam__0(lean_object* v_z_115_, lean_object* v_w_116_){
_start:
{
lean_object* v_re_117_; lean_object* v_im_118_; lean_object* v_re_119_; lean_object* v_im_120_; lean_object* v___x_122_; uint8_t v_isShared_123_; uint8_t v_isSharedCheck_129_; 
v_re_117_ = lean_ctor_get(v_z_115_, 0);
v_im_118_ = lean_ctor_get(v_z_115_, 1);
v_re_119_ = lean_ctor_get(v_w_116_, 0);
v_im_120_ = lean_ctor_get(v_w_116_, 1);
v_isSharedCheck_129_ = !lean_is_exclusive(v_w_116_);
if (v_isSharedCheck_129_ == 0)
{
v___x_122_ = v_w_116_;
v_isShared_123_ = v_isSharedCheck_129_;
goto v_resetjp_121_;
}
else
{
lean_inc(v_im_120_);
lean_inc(v_re_119_);
lean_dec(v_w_116_);
v___x_122_ = lean_box(0);
v_isShared_123_ = v_isSharedCheck_129_;
goto v_resetjp_121_;
}
v_resetjp_121_:
{
lean_object* v___x_124_; lean_object* v___x_125_; lean_object* v___x_127_; 
v___x_124_ = lean_int_add(v_re_117_, v_re_119_);
lean_dec(v_re_119_);
v___x_125_ = lean_int_add(v_im_118_, v_im_120_);
lean_dec(v_im_120_);
if (v_isShared_123_ == 0)
{
lean_ctor_set(v___x_122_, 1, v___x_125_);
lean_ctor_set(v___x_122_, 0, v___x_124_);
v___x_127_ = v___x_122_;
goto v_reusejp_126_;
}
else
{
lean_object* v_reuseFailAlloc_128_; 
v_reuseFailAlloc_128_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_128_, 0, v___x_124_);
lean_ctor_set(v_reuseFailAlloc_128_, 1, v___x_125_);
v___x_127_ = v_reuseFailAlloc_128_;
goto v_reusejp_126_;
}
v_reusejp_126_:
{
return v___x_127_;
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instAdd___lam__0___boxed(lean_object* v_z_130_, lean_object* v_w_131_){
_start:
{
lean_object* v_res_132_; 
v_res_132_ = lp_GaussianCollatz_GInt_instAdd___lam__0(v_z_130_, v_w_131_);
lean_dec_ref(v_z_130_);
return v_res_132_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instNeg___lam__0(lean_object* v_z_135_){
_start:
{
lean_object* v_re_136_; lean_object* v_im_137_; lean_object* v___x_139_; uint8_t v_isShared_140_; uint8_t v_isSharedCheck_146_; 
v_re_136_ = lean_ctor_get(v_z_135_, 0);
v_im_137_ = lean_ctor_get(v_z_135_, 1);
v_isSharedCheck_146_ = !lean_is_exclusive(v_z_135_);
if (v_isSharedCheck_146_ == 0)
{
v___x_139_ = v_z_135_;
v_isShared_140_ = v_isSharedCheck_146_;
goto v_resetjp_138_;
}
else
{
lean_inc(v_im_137_);
lean_inc(v_re_136_);
lean_dec(v_z_135_);
v___x_139_ = lean_box(0);
v_isShared_140_ = v_isSharedCheck_146_;
goto v_resetjp_138_;
}
v_resetjp_138_:
{
lean_object* v___x_141_; lean_object* v___x_142_; lean_object* v___x_144_; 
v___x_141_ = lean_int_neg(v_re_136_);
lean_dec(v_re_136_);
v___x_142_ = lean_int_neg(v_im_137_);
lean_dec(v_im_137_);
if (v_isShared_140_ == 0)
{
lean_ctor_set(v___x_139_, 1, v___x_142_);
lean_ctor_set(v___x_139_, 0, v___x_141_);
v___x_144_ = v___x_139_;
goto v_reusejp_143_;
}
else
{
lean_object* v_reuseFailAlloc_145_; 
v_reuseFailAlloc_145_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_145_, 0, v___x_141_);
lean_ctor_set(v_reuseFailAlloc_145_, 1, v___x_142_);
v___x_144_ = v_reuseFailAlloc_145_;
goto v_reusejp_143_;
}
v_reusejp_143_:
{
return v___x_144_;
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instSub___lam__0(lean_object* v_z_149_, lean_object* v_w_150_){
_start:
{
lean_object* v_re_151_; lean_object* v_im_152_; lean_object* v_re_153_; lean_object* v_im_154_; lean_object* v___x_156_; uint8_t v_isShared_157_; uint8_t v_isSharedCheck_163_; 
v_re_151_ = lean_ctor_get(v_z_149_, 0);
v_im_152_ = lean_ctor_get(v_z_149_, 1);
v_re_153_ = lean_ctor_get(v_w_150_, 0);
v_im_154_ = lean_ctor_get(v_w_150_, 1);
v_isSharedCheck_163_ = !lean_is_exclusive(v_w_150_);
if (v_isSharedCheck_163_ == 0)
{
v___x_156_ = v_w_150_;
v_isShared_157_ = v_isSharedCheck_163_;
goto v_resetjp_155_;
}
else
{
lean_inc(v_im_154_);
lean_inc(v_re_153_);
lean_dec(v_w_150_);
v___x_156_ = lean_box(0);
v_isShared_157_ = v_isSharedCheck_163_;
goto v_resetjp_155_;
}
v_resetjp_155_:
{
lean_object* v___x_158_; lean_object* v___x_159_; lean_object* v___x_161_; 
v___x_158_ = lean_int_sub(v_re_151_, v_re_153_);
lean_dec(v_re_153_);
v___x_159_ = lean_int_sub(v_im_152_, v_im_154_);
lean_dec(v_im_154_);
if (v_isShared_157_ == 0)
{
lean_ctor_set(v___x_156_, 1, v___x_159_);
lean_ctor_set(v___x_156_, 0, v___x_158_);
v___x_161_ = v___x_156_;
goto v_reusejp_160_;
}
else
{
lean_object* v_reuseFailAlloc_162_; 
v_reuseFailAlloc_162_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_162_, 0, v___x_158_);
lean_ctor_set(v_reuseFailAlloc_162_, 1, v___x_159_);
v___x_161_ = v_reuseFailAlloc_162_;
goto v_reusejp_160_;
}
v_reusejp_160_:
{
return v___x_161_;
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instSub___lam__0___boxed(lean_object* v_z_164_, lean_object* v_w_165_){
_start:
{
lean_object* v_res_166_; 
v_res_166_ = lp_GaussianCollatz_GInt_instSub___lam__0(v_z_164_, v_w_165_);
lean_dec_ref(v_z_164_);
return v_res_166_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instMul___lam__0(lean_object* v_z_169_, lean_object* v_w_170_){
_start:
{
lean_object* v_re_171_; lean_object* v_im_172_; lean_object* v_re_173_; lean_object* v_im_174_; lean_object* v___x_176_; uint8_t v_isShared_177_; uint8_t v_isSharedCheck_187_; 
v_re_171_ = lean_ctor_get(v_z_169_, 0);
v_im_172_ = lean_ctor_get(v_z_169_, 1);
v_re_173_ = lean_ctor_get(v_w_170_, 0);
v_im_174_ = lean_ctor_get(v_w_170_, 1);
v_isSharedCheck_187_ = !lean_is_exclusive(v_w_170_);
if (v_isSharedCheck_187_ == 0)
{
v___x_176_ = v_w_170_;
v_isShared_177_ = v_isSharedCheck_187_;
goto v_resetjp_175_;
}
else
{
lean_inc(v_im_174_);
lean_inc(v_re_173_);
lean_dec(v_w_170_);
v___x_176_ = lean_box(0);
v_isShared_177_ = v_isSharedCheck_187_;
goto v_resetjp_175_;
}
v_resetjp_175_:
{
lean_object* v___x_178_; lean_object* v___x_179_; lean_object* v___x_180_; lean_object* v___x_181_; lean_object* v___x_182_; lean_object* v___x_183_; lean_object* v___x_185_; 
v___x_178_ = lean_int_mul(v_re_171_, v_re_173_);
v___x_179_ = lean_int_mul(v_im_172_, v_im_174_);
v___x_180_ = lean_int_sub(v___x_178_, v___x_179_);
lean_dec(v___x_179_);
lean_dec(v___x_178_);
v___x_181_ = lean_int_mul(v_re_171_, v_im_174_);
lean_dec(v_im_174_);
v___x_182_ = lean_int_mul(v_im_172_, v_re_173_);
lean_dec(v_re_173_);
v___x_183_ = lean_int_add(v___x_181_, v___x_182_);
lean_dec(v___x_182_);
lean_dec(v___x_181_);
if (v_isShared_177_ == 0)
{
lean_ctor_set(v___x_176_, 1, v___x_183_);
lean_ctor_set(v___x_176_, 0, v___x_180_);
v___x_185_ = v___x_176_;
goto v_reusejp_184_;
}
else
{
lean_object* v_reuseFailAlloc_186_; 
v_reuseFailAlloc_186_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_186_, 0, v___x_180_);
lean_ctor_set(v_reuseFailAlloc_186_, 1, v___x_183_);
v___x_185_ = v_reuseFailAlloc_186_;
goto v_reusejp_184_;
}
v_reusejp_184_:
{
return v___x_185_;
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instMul___lam__0___boxed(lean_object* v_z_188_, lean_object* v_w_189_){
_start:
{
lean_object* v_res_190_; 
v_res_190_ = lp_GaussianCollatz_GInt_instMul___lam__0(v_z_188_, v_w_189_);
lean_dec_ref(v_z_188_);
return v_res_190_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instOfNat(lean_object* v_n_193_){
_start:
{
lean_object* v___x_194_; lean_object* v___x_195_; lean_object* v___x_196_; 
v___x_194_ = lean_nat_to_int(v_n_193_);
v___x_195_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_196_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v___x_196_, 0, v___x_194_);
lean_ctor_set(v___x_196_, 1, v___x_195_);
return v___x_196_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_I___closed__0(void){
_start:
{
lean_object* v___x_197_; lean_object* v___x_198_; 
v___x_197_ = lean_unsigned_to_nat(1u);
v___x_198_ = lean_nat_to_int(v___x_197_);
return v___x_198_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_I___closed__1(void){
_start:
{
lean_object* v___x_199_; lean_object* v___x_200_; lean_object* v___x_201_; 
v___x_199_ = lean_obj_once(&lp_GaussianCollatz_GInt_I___closed__0, &lp_GaussianCollatz_GInt_I___closed__0_once, _init_lp_GaussianCollatz_GInt_I___closed__0);
v___x_200_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_201_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v___x_201_, 0, v___x_200_);
lean_ctor_set(v___x_201_, 1, v___x_199_);
return v___x_201_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_I(void){
_start:
{
lean_object* v___x_202_; 
v___x_202_ = lean_obj_once(&lp_GaussianCollatz_GInt_I___closed__1, &lp_GaussianCollatz_GInt_I___closed__1_once, _init_lp_GaussianCollatz_GInt_I___closed__1);
return v___x_202_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_pi___closed__0(void){
_start:
{
lean_object* v___x_203_; lean_object* v___x_204_; 
v___x_203_ = lean_obj_once(&lp_GaussianCollatz_GInt_I___closed__0, &lp_GaussianCollatz_GInt_I___closed__0_once, _init_lp_GaussianCollatz_GInt_I___closed__0);
v___x_204_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v___x_204_, 0, v___x_203_);
lean_ctor_set(v___x_204_, 1, v___x_203_);
return v___x_204_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_pi(void){
_start:
{
lean_object* v___x_205_; 
v___x_205_ = lean_obj_once(&lp_GaussianCollatz_GInt_pi___closed__0, &lp_GaussianCollatz_GInt_pi___closed__0_once, _init_lp_GaussianCollatz_GInt_pi___closed__0);
return v___x_205_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_N(lean_object* v_z_206_){
_start:
{
lean_object* v_re_207_; lean_object* v_im_208_; lean_object* v___x_209_; lean_object* v___x_210_; lean_object* v___x_211_; lean_object* v___x_212_; lean_object* v___x_213_; 
v_re_207_ = lean_ctor_get(v_z_206_, 0);
v_im_208_ = lean_ctor_get(v_z_206_, 1);
v___x_209_ = lean_nat_abs(v_re_207_);
v___x_210_ = lean_nat_mul(v___x_209_, v___x_209_);
lean_dec(v___x_209_);
v___x_211_ = lean_nat_abs(v_im_208_);
v___x_212_ = lean_nat_mul(v___x_211_, v___x_211_);
lean_dec(v___x_211_);
v___x_213_ = lean_nat_add(v___x_210_, v___x_212_);
lean_dec(v___x_212_);
lean_dec(v___x_210_);
return v___x_213_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_N___boxed(lean_object* v_z_214_){
_start:
{
lean_object* v_res_215_; 
v_res_215_ = lp_GaussianCollatz_GInt_N(v_z_214_);
lean_dec_ref(v_z_214_);
return v_res_215_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_instDvd(void){
_start:
{
lean_object* v___x_216_; 
v___x_216_ = lean_box(0);
return v___x_216_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_pow___closed__0(void){
_start:
{
lean_object* v___x_217_; lean_object* v___x_218_; lean_object* v___x_219_; 
v___x_217_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_218_ = lean_obj_once(&lp_GaussianCollatz_GInt_I___closed__0, &lp_GaussianCollatz_GInt_I___closed__0_once, _init_lp_GaussianCollatz_GInt_I___closed__0);
v___x_219_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v___x_219_, 0, v___x_218_);
lean_ctor_set(v___x_219_, 1, v___x_217_);
return v___x_219_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_pow(lean_object* v_z_220_, lean_object* v_x_221_){
_start:
{
lean_object* v_zero_222_; uint8_t v_isZero_223_; 
v_zero_222_ = lean_unsigned_to_nat(0u);
v_isZero_223_ = lean_nat_dec_eq(v_x_221_, v_zero_222_);
if (v_isZero_223_ == 1)
{
lean_object* v___x_224_; 
lean_dec_ref(v_z_220_);
v___x_224_ = lean_obj_once(&lp_GaussianCollatz_GInt_pow___closed__0, &lp_GaussianCollatz_GInt_pow___closed__0_once, _init_lp_GaussianCollatz_GInt_pow___closed__0);
return v___x_224_;
}
else
{
lean_object* v_one_225_; lean_object* v_n_226_; lean_object* v___x_227_; lean_object* v_re_228_; lean_object* v_im_229_; lean_object* v_re_230_; lean_object* v_im_231_; lean_object* v___x_233_; uint8_t v_isShared_234_; uint8_t v_isSharedCheck_244_; 
v_one_225_ = lean_unsigned_to_nat(1u);
v_n_226_ = lean_nat_sub(v_x_221_, v_one_225_);
lean_inc_ref(v_z_220_);
v___x_227_ = lp_GaussianCollatz_GInt_pow(v_z_220_, v_n_226_);
lean_dec(v_n_226_);
v_re_228_ = lean_ctor_get(v___x_227_, 0);
lean_inc(v_re_228_);
v_im_229_ = lean_ctor_get(v___x_227_, 1);
lean_inc(v_im_229_);
lean_dec_ref(v___x_227_);
v_re_230_ = lean_ctor_get(v_z_220_, 0);
v_im_231_ = lean_ctor_get(v_z_220_, 1);
v_isSharedCheck_244_ = !lean_is_exclusive(v_z_220_);
if (v_isSharedCheck_244_ == 0)
{
v___x_233_ = v_z_220_;
v_isShared_234_ = v_isSharedCheck_244_;
goto v_resetjp_232_;
}
else
{
lean_inc(v_im_231_);
lean_inc(v_re_230_);
lean_dec(v_z_220_);
v___x_233_ = lean_box(0);
v_isShared_234_ = v_isSharedCheck_244_;
goto v_resetjp_232_;
}
v_resetjp_232_:
{
lean_object* v___x_235_; lean_object* v___x_236_; lean_object* v___x_237_; lean_object* v___x_238_; lean_object* v___x_239_; lean_object* v___x_240_; lean_object* v___x_242_; 
v___x_235_ = lean_int_mul(v_re_228_, v_re_230_);
v___x_236_ = lean_int_mul(v_im_229_, v_im_231_);
v___x_237_ = lean_int_sub(v___x_235_, v___x_236_);
lean_dec(v___x_236_);
lean_dec(v___x_235_);
v___x_238_ = lean_int_mul(v_re_228_, v_im_231_);
lean_dec(v_im_231_);
lean_dec(v_re_228_);
v___x_239_ = lean_int_mul(v_im_229_, v_re_230_);
lean_dec(v_re_230_);
lean_dec(v_im_229_);
v___x_240_ = lean_int_add(v___x_238_, v___x_239_);
lean_dec(v___x_239_);
lean_dec(v___x_238_);
if (v_isShared_234_ == 0)
{
lean_ctor_set(v___x_233_, 1, v___x_240_);
lean_ctor_set(v___x_233_, 0, v___x_237_);
v___x_242_ = v___x_233_;
goto v_reusejp_241_;
}
else
{
lean_object* v_reuseFailAlloc_243_; 
v_reuseFailAlloc_243_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_243_, 0, v___x_237_);
lean_ctor_set(v_reuseFailAlloc_243_, 1, v___x_240_);
v___x_242_ = v_reuseFailAlloc_243_;
goto v_reusejp_241_;
}
v_reusejp_241_:
{
return v___x_242_;
}
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_pow___boxed(lean_object* v_z_245_, lean_object* v_x_246_){
_start:
{
lean_object* v_res_247_; 
v_res_247_ = lp_GaussianCollatz_GInt_pow(v_z_245_, v_x_246_);
lean_dec(v_x_246_);
return v_res_247_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___redArg(lean_object* v_x_248_, lean_object* v_h__1_249_, lean_object* v_h__2_250_){
_start:
{
lean_object* v_zero_251_; uint8_t v_isZero_252_; 
v_zero_251_ = lean_unsigned_to_nat(0u);
v_isZero_252_ = lean_nat_dec_eq(v_x_248_, v_zero_251_);
if (v_isZero_252_ == 1)
{
lean_object* v___x_253_; lean_object* v___x_254_; 
lean_dec(v_h__2_250_);
v___x_253_ = lean_box(0);
v___x_254_ = lean_apply_1(v_h__1_249_, v___x_253_);
return v___x_254_;
}
else
{
lean_object* v_one_255_; lean_object* v_n_256_; lean_object* v___x_257_; 
lean_dec(v_h__1_249_);
v_one_255_ = lean_unsigned_to_nat(1u);
v_n_256_ = lean_nat_sub(v_x_248_, v_one_255_);
v___x_257_ = lean_apply_1(v_h__2_250_, v_n_256_);
return v___x_257_;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___redArg___boxed(lean_object* v_x_258_, lean_object* v_h__1_259_, lean_object* v_h__2_260_){
_start:
{
lean_object* v_res_261_; 
v_res_261_ = lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___redArg(v_x_258_, v_h__1_259_, v_h__2_260_);
lean_dec(v_x_258_);
return v_res_261_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter(lean_object* v_motive_262_, lean_object* v_x_263_, lean_object* v_h__1_264_, lean_object* v_h__2_265_){
_start:
{
lean_object* v_zero_266_; uint8_t v_isZero_267_; 
v_zero_266_ = lean_unsigned_to_nat(0u);
v_isZero_267_ = lean_nat_dec_eq(v_x_263_, v_zero_266_);
if (v_isZero_267_ == 1)
{
lean_object* v___x_268_; lean_object* v___x_269_; 
lean_dec(v_h__2_265_);
v___x_268_ = lean_box(0);
v___x_269_ = lean_apply_1(v_h__1_264_, v___x_268_);
return v___x_269_;
}
else
{
lean_object* v_one_270_; lean_object* v_n_271_; lean_object* v___x_272_; 
lean_dec(v_h__1_264_);
v_one_270_ = lean_unsigned_to_nat(1u);
v_n_271_ = lean_nat_sub(v_x_263_, v_one_270_);
v___x_272_ = lean_apply_1(v_h__2_265_, v_n_271_);
return v___x_272_;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter___boxed(lean_object* v_motive_273_, lean_object* v_x_274_, lean_object* v_h__1_275_, lean_object* v_h__2_276_){
_start:
{
lean_object* v_res_277_; 
v_res_277_ = lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_pow_match__1_splitter(v_motive_273_, v_x_274_, v_h__1_275_, v_h__2_276_);
lean_dec(v_x_274_);
return v_res_277_;
}
}
static lean_object* _init_lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0(void){
_start:
{
lean_object* v___x_278_; lean_object* v___x_279_; 
v___x_278_ = lean_unsigned_to_nat(2u);
v___x_279_ = lean_nat_to_int(v___x_278_);
return v___x_279_;
}
}
LEAN_EXPORT uint8_t lp_GaussianCollatz_GInt_instDecidableDvdPi(lean_object* v_z_280_){
_start:
{
lean_object* v_re_281_; lean_object* v_im_282_; lean_object* v___x_283_; lean_object* v___x_284_; lean_object* v___x_285_; lean_object* v___x_286_; uint8_t v___x_287_; 
v_re_281_ = lean_ctor_get(v_z_280_, 0);
v_im_282_ = lean_ctor_get(v_z_280_, 1);
v___x_283_ = lean_int_sub(v_re_281_, v_im_282_);
v___x_284_ = lean_obj_once(&lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0, &lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0_once, _init_lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0);
v___x_285_ = lean_int_emod(v___x_283_, v___x_284_);
lean_dec(v___x_283_);
v___x_286_ = lean_obj_once(&lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17, &lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17_once, _init_lp_GaussianCollatz_instReprGInt_repr___redArg___closed__17);
v___x_287_ = lean_int_dec_eq(v___x_285_, v___x_286_);
lean_dec(v___x_285_);
return v___x_287_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_instDecidableDvdPi___boxed(lean_object* v_z_288_){
_start:
{
uint8_t v_res_289_; lean_object* v_r_290_; 
v_res_289_ = lp_GaussianCollatz_GInt_instDecidableDvdPi(v_z_288_);
lean_dec_ref(v_z_288_);
v_r_290_ = lean_box(v_res_289_);
return v_r_290_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_divPi(lean_object* v_z_291_){
_start:
{
lean_object* v_re_292_; lean_object* v_im_293_; lean_object* v___x_295_; uint8_t v_isShared_296_; uint8_t v_isSharedCheck_305_; 
v_re_292_ = lean_ctor_get(v_z_291_, 0);
v_im_293_ = lean_ctor_get(v_z_291_, 1);
v_isSharedCheck_305_ = !lean_is_exclusive(v_z_291_);
if (v_isSharedCheck_305_ == 0)
{
v___x_295_ = v_z_291_;
v_isShared_296_ = v_isSharedCheck_305_;
goto v_resetjp_294_;
}
else
{
lean_inc(v_im_293_);
lean_inc(v_re_292_);
lean_dec(v_z_291_);
v___x_295_ = lean_box(0);
v_isShared_296_ = v_isSharedCheck_305_;
goto v_resetjp_294_;
}
v_resetjp_294_:
{
lean_object* v___x_297_; lean_object* v___x_298_; lean_object* v___x_299_; lean_object* v___x_300_; lean_object* v___x_301_; lean_object* v___x_303_; 
v___x_297_ = lean_int_add(v_re_292_, v_im_293_);
v___x_298_ = lean_obj_once(&lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0, &lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0_once, _init_lp_GaussianCollatz_GInt_instDecidableDvdPi___closed__0);
v___x_299_ = lean_int_ediv(v___x_297_, v___x_298_);
lean_dec(v___x_297_);
v___x_300_ = lean_int_sub(v_im_293_, v_re_292_);
lean_dec(v_re_292_);
lean_dec(v_im_293_);
v___x_301_ = lean_int_ediv(v___x_300_, v___x_298_);
lean_dec(v___x_300_);
if (v_isShared_296_ == 0)
{
lean_ctor_set(v___x_295_, 1, v___x_301_);
lean_ctor_set(v___x_295_, 0, v___x_299_);
v___x_303_ = v___x_295_;
goto v_reusejp_302_;
}
else
{
lean_object* v_reuseFailAlloc_304_; 
v_reuseFailAlloc_304_ = lean_alloc_ctor(0, 2, 0);
lean_ctor_set(v_reuseFailAlloc_304_, 0, v___x_299_);
lean_ctor_set(v_reuseFailAlloc_304_, 1, v___x_301_);
v___x_303_ = v_reuseFailAlloc_304_;
goto v_reusejp_302_;
}
v_reusejp_302_:
{
return v___x_303_;
}
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz_GInt_iter(lean_object* v_f_306_, lean_object* v_x_307_, lean_object* v_x_308_){
_start:
{
lean_object* v_zero_309_; uint8_t v_isZero_310_; 
v_zero_309_ = lean_unsigned_to_nat(0u);
v_isZero_310_ = lean_nat_dec_eq(v_x_307_, v_zero_309_);
if (v_isZero_310_ == 1)
{
lean_dec(v_x_307_);
lean_dec_ref(v_f_306_);
return v_x_308_;
}
else
{
lean_object* v_one_311_; lean_object* v_n_312_; lean_object* v___x_313_; 
v_one_311_ = lean_unsigned_to_nat(1u);
v_n_312_ = lean_nat_sub(v_x_307_, v_one_311_);
lean_dec(v_x_307_);
lean_inc_ref(v_f_306_);
v___x_313_ = lean_apply_1(v_f_306_, v_x_308_);
v_x_307_ = v_n_312_;
v_x_308_ = v___x_313_;
goto _start;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___redArg(lean_object* v_x_315_, lean_object* v_x_316_, lean_object* v_h__1_317_, lean_object* v_h__2_318_){
_start:
{
lean_object* v_zero_319_; uint8_t v_isZero_320_; 
v_zero_319_ = lean_unsigned_to_nat(0u);
v_isZero_320_ = lean_nat_dec_eq(v_x_315_, v_zero_319_);
if (v_isZero_320_ == 1)
{
lean_object* v___x_321_; 
lean_dec(v_h__2_318_);
v___x_321_ = lean_apply_1(v_h__1_317_, v_x_316_);
return v___x_321_;
}
else
{
lean_object* v_one_322_; lean_object* v_n_323_; lean_object* v___x_324_; 
lean_dec(v_h__1_317_);
v_one_322_ = lean_unsigned_to_nat(1u);
v_n_323_ = lean_nat_sub(v_x_315_, v_one_322_);
v___x_324_ = lean_apply_2(v_h__2_318_, v_n_323_, v_x_316_);
return v___x_324_;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___redArg___boxed(lean_object* v_x_325_, lean_object* v_x_326_, lean_object* v_h__1_327_, lean_object* v_h__2_328_){
_start:
{
lean_object* v_res_329_; 
v_res_329_ = lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___redArg(v_x_325_, v_x_326_, v_h__1_327_, v_h__2_328_);
lean_dec(v_x_325_);
return v_res_329_;
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter(lean_object* v_motive_330_, lean_object* v_x_331_, lean_object* v_x_332_, lean_object* v_h__1_333_, lean_object* v_h__2_334_){
_start:
{
lean_object* v_zero_335_; uint8_t v_isZero_336_; 
v_zero_335_ = lean_unsigned_to_nat(0u);
v_isZero_336_ = lean_nat_dec_eq(v_x_331_, v_zero_335_);
if (v_isZero_336_ == 1)
{
lean_object* v___x_337_; 
lean_dec(v_h__2_334_);
v___x_337_ = lean_apply_1(v_h__1_333_, v_x_332_);
return v___x_337_;
}
else
{
lean_object* v_one_338_; lean_object* v_n_339_; lean_object* v___x_340_; 
lean_dec(v_h__1_333_);
v_one_338_ = lean_unsigned_to_nat(1u);
v_n_339_ = lean_nat_sub(v_x_331_, v_one_338_);
v___x_340_ = lean_apply_2(v_h__2_334_, v_n_339_, v_x_332_);
return v___x_340_;
}
}
}
LEAN_EXPORT lean_object* lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter___boxed(lean_object* v_motive_341_, lean_object* v_x_342_, lean_object* v_x_343_, lean_object* v_h__1_344_, lean_object* v_h__2_345_){
_start:
{
lean_object* v_res_346_; 
v_res_346_ = lp_GaussianCollatz___private_GaussianCollatz_Basic_0__GInt_iter_match__1_splitter(v_motive_341_, v_x_342_, v_x_343_, v_h__1_344_, v_h__2_345_);
lean_dec(v_x_342_);
return v_res_346_;
}
}
lean_object* initialize_Init(uint8_t builtin);
lean_object* initialize_Init(uint8_t builtin);
void lean_initialize_runtime_module();
static bool _G_initialized = false;
LEAN_EXPORT lean_object* initialize_GaussianCollatz_GaussianCollatz_Basic(uint8_t builtin) {
lean_object * res;
if (_G_initialized) return lean_io_result_mk_ok(lean_box(0));
_G_initialized = true;
lean_initialize_runtime_module();
res = initialize_Init(builtin);
if (lean_io_result_is_error(res)) return res;
lean_dec_ref(res);
res = initialize_Init(builtin);
if (lean_io_result_is_error(res)) return res;
lean_dec_ref(res);
lp_GaussianCollatz_GInt_I = _init_lp_GaussianCollatz_GInt_I();
lean_mark_persistent(lp_GaussianCollatz_GInt_I);
lp_GaussianCollatz_GInt_pi = _init_lp_GaussianCollatz_GInt_pi();
lean_mark_persistent(lp_GaussianCollatz_GInt_pi);
lp_GaussianCollatz_GInt_instDvd = _init_lp_GaussianCollatz_GInt_instDvd();
lean_mark_persistent(lp_GaussianCollatz_GInt_instDvd);
return lean_io_result_mk_ok(lean_box(0));
}
#ifdef __cplusplus
}
#endif
