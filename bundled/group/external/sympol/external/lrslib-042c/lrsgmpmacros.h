#ifndef LRS_MACROS
#define LRS_MACROS

/**********************************/
/*         MACROS                 */
/* dependent on mp implementation */
/**********************************/

#define addint(a, b, c)         mpz_add((c),(a),(b))
#define changesign(a)           mpz_neg((a),(a))
#define copy(a, b)              mpz_set(a,b)         
#define decint(a, b)            mpz_sub((a),(a),(b))
#define divint(a, b, c)         mpz_tdiv_qr((c),(a),(a),(b))
#define exactdivint(a, b, c)    mpz_divexact((c),(a),(b))    /*known there is no remainder */          
#define getfactorial(a, b)      mpz_fac_ui( (a), (b))
#define greater(a, b)           (mpz_cmp((a),(b))>0 ? ONE : ZERO)
#define gcd(a,b)                mpz_gcd((a),(a),(b))
#define itomp(in, a)            mpz_set_si( (a) , (in) )
#define mptoi(a)                mpz_get_si( (a) )
#define mptodouble(a)           mpz_get_d ( (a) )
#define mulint(a, b, c)         mpz_mul((c),(a),(b))
#define one(a)                  (mpz_cmp_si((a),ONE) == 0 ? ONE : ZERO)
#define negative(a)             (mpz_sgn(a) < 0 ? ONE : ZERO)
#define normalize(a)            (void) 0
#define positive(a)             (mpz_sgn(a) > 0 ? ONE : ZERO)
#define sign(a)                 (mpz_sgn(a) < 0 ? NEG : POS)
#define subint(a, b, c)         mpz_sub((c),(a),(b))
#define zero(a)                 (mpz_sgn(a) == 0 ? ONE : ZERO)

#endif

