/* Copyright (c) 1997-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/ideal/internal/singularInclude.h"

#include <kernel/combinatorics/stairc.h>
#include <singular/polys/matpol.h>
#include <coeffs/mpr_complex.h>
#include <kernel/linear_algebra/MinorInterface.h>

#include "polymake/ideal/singularIdeal.h"
#include "polymake/ideal/internal/singularTermOrderData.h"
#include "polymake/ideal/internal/singularRingManager.h"
#include "polymake/ideal/internal/singularUtils.h"

namespace polymake {
namespace ideal {
namespace singular {


class SingularIdeal_impl : public SingularIdeal_wrap {
private:
   ::ideal singIdeal;
   idhdl singRing;

   void create_singIdeal(const Array<Polynomial<>>& gens) {
      int npoly = gens.size();
      singIdeal = idInit(npoly,1);
      int j = 0;
      // Converting monomials as described in libsing-test2.cc.
      for (const auto& mypoly : gens) {
         singIdeal->m[j] = convert_Polynomial_to_poly(mypoly, IDRING(singRing));
         ++j;
      }
   }

public:
  
   // Constructing singIdeal from the generators:
   template<typename OrderType>
   SingularIdeal_impl(const Array<Polynomial<>>& gens, const OrderType& order)
   {
      int n_vars = gens[0].n_vars();
      SingularTermOrderData<OrderType > TO = SingularTermOrderData<OrderType >(n_vars, order);
      singRing = check_ring(n_vars, TO);
      if (!gens.size())
         throw std::runtime_error("Ideal has no generators.");
      create_singIdeal(gens);
   }
   
   SingularIdeal_impl(const Array<Polynomial<>>& gens, idhdl r){
      singRing = check_ring(r);
      create_singIdeal(gens);
   }

   SingularIdeal_impl(const ::ideal i, const idhdl r)
   {
      singIdeal=idCopy(i);
      singRing=r;
   }

   SingularIdeal_impl(const SingularIdeal_impl* sI)
   {
      singIdeal = idCopy(sI->singIdeal);
      singRing = sI->singRing;
   }

   SingularIdeal_wrap* copy() const
   {
      return new SingularIdeal_impl(singIdeal,singRing);
   }

   ~SingularIdeal_impl()
   {
      if(singRing!=NULL) {
         check_ring(singRing);
         if(singIdeal!=NULL)
            id_Delete(&singIdeal,IDRING(singRing));
      }
   }

   // Compute a groebner basis of a Polymake ideal using Singular
   void groebner() 
   {
      check_ring(singRing); 
      ::ideal res = kStd(singIdeal,NULL,testHomog,NULL);
      id_Delete(&singIdeal,IDRING(singRing));
      singIdeal = res;
   }

   // Compute the dimension of an ideal.
   int dim() {
      check_ring(singRing); 
      return scDimInt(singIdeal, NULL);
   }
   
   // Computes a monomial in the initial ideal via saturation (returns 0 if no monnomial is contained)
   Polynomial<> contains_monomial() const
   {
      check_ring(singRing);
      ring r = IDRING(singRing);
      ::ideal M = idInit(1);
      M->m[0] = p_Init(r);
      for (int i=1; i<=rVar(r); i++)
         p_SetExp(M->m[0],i,1,r);
      p_SetCoeff(M->m[0],n_Init(1,r->cf),r);
      p_Setm(M->m[0],r);
      ::ideal J = id_Copy(singIdeal,r);
      bool b;
      int k = 0;

      do
      {
         ::ideal Jstd = kStd(J,NULL,testHomog,NULL);
         for (int i=0; i<IDELEMS(Jstd); i++)
         {
            poly g = Jstd->m[i];
            if (g != NULL && pNext(g) == NULL)
            {
               for (int j=1; j<=rVar(r); j++)
                  p_AddExp(g,j,k,r);
               p_Setm(g,r);
               Polynomial<> monom = convert_poly_to_Polynomial(g);
               id_Delete(&M,r);
               id_Delete(&J,r);
               id_Delete(&Jstd,r);
               return monom;
            }
         }
         ::ideal JquotM = idQuot(Jstd,M,true,true);
         ::ideal JquotMredJ = kNF(Jstd,NULL,JquotM);
         b = idIs0(JquotMredJ);
         id_Delete(&Jstd,r);
         id_Delete(&J,r);
         J = JquotM;
         id_Delete(&JquotMredJ,r);
         k++;
      } while (!b);

      id_Delete(&M,r);
      id_Delete(&J,r);
      return Polynomial<>(rVar(r));
   }


   SingularIdeal_wrap* initial_ideal() const {
      check_ring(singRing); 
      ::ideal res = id_Head(singIdeal,IDRING(singRing));
      SingularIdeal_wrap* initial = new SingularIdeal_impl(res,singRing);
      id_Delete(&res,IDRING(singRing));
      return initial;
   }

   // Compute the radical of an ideal using primdec.lib from Singular
   SingularIdeal_wrap* radical() const {
      check_ring(singRing); 
      sleftv arg;
      memset(&arg,0,sizeof(arg));
      load_library("primdec.lib");
      idhdl radical=get_singular_function("radical");
      
      arg.rtyp=IDEAL_CMD;
      arg.data=(void *)idCopy(singIdeal);
      // call radical
      BOOLEAN res=iiMake_proc(radical,NULL,&arg);
      if (res) {
         errorreported = 0;
         iiRETURNEXPR.Init();
         throw std::runtime_error("radical returned an error");
      }
      SingularIdeal_wrap* radical_wrap = new SingularIdeal_impl((::ideal) (iiRETURNEXPR.Data()), singRing);
      iiRETURNEXPR.CleanUp();
      iiRETURNEXPR.Init();
      return radical_wrap;
   }

   SingularIdeal_wrap* saturation(const Array<Polynomial<>>& rhs) const {
      check_ring(singRing);
      load_library("elim.lib");
      idhdl sathdl = get_singular_function("sat");
      
      ::ideal J = idInit(rhs.size(),1);
      // Converting monomials as described in libsing-test2.cc.
      for (int j = 0; j<rhs.size(); ++j) {
         J->m[j] = convert_Polynomial_to_poly(rhs[j], IDRING(singRing));
      }


      sleftv arg;
      memset(&arg,0,sizeof(arg));
      arg.rtyp=IDEAL_CMD;
      arg.data=(void *)idCopy(singIdeal);
      arg.next=(leftv)omAlloc0(sizeof(sleftv));
      arg.next->rtyp=IDEAL_CMD;
      arg.next->data=(void *)idCopy(J);
      // call primdecSY
      BOOLEAN res=iiMake_proc(sathdl,NULL,&arg);
      if(!res && (iiRETURNEXPR.Typ() == LIST_CMD)){
         lists L = (lists)iiRETURNEXPR.Data();
         SingularIdeal_wrap* result;
         if(L->m[0].Typ() == IDEAL_CMD){
            result = new SingularIdeal_impl((::ideal) (L->m[0].Data()),singRing);
         } else {
            throw std::runtime_error("Something went wrong for the primary decomposition");
         }
         iiRETURNEXPR.CleanUp();
         iiRETURNEXPR.Init();
         return result;
      } else {
         iiRETURNEXPR.Init();
         throw std::runtime_error("Something went wrong for the saturation");
      }

   }

   Array<SingularIdeal_wrap*> primary_decomposition() const {
      check_ring(singRing);
      load_library("primdec.lib");
      idhdl primdecSY = get_singular_function("primdecSY");
      sleftv arg;
      memset(&arg,0,sizeof(arg));
      arg.rtyp=IDEAL_CMD;
      arg.data=(void *)idCopy(singIdeal);
      // call primdecSY
      BOOLEAN res=iiMake_proc(primdecSY,NULL,&arg);
      if(!res && (iiRETURNEXPR.Typ() == LIST_CMD)){
         lists L = (lists)iiRETURNEXPR.Data();
         Array<SingularIdeal_wrap*> result(L->nr+1);
         for(int j=0; j<=L->nr; j++){
            lists LL = (lists)L->m[j].Data();
            if(LL->m[0].Typ() == IDEAL_CMD){
               result[j] = new SingularIdeal_impl((::ideal) (LL->m[0].Data()),singRing);
            } else {
               throw std::runtime_error("Something went wrong for the primary decomposition");
            }
         }
         iiRETURNEXPR.CleanUp();
         iiRETURNEXPR.Init();
         return result;
      } else {
         iiRETURNEXPR.Init();
         throw std::runtime_error("Something went wrong for the primary decomposition");
      }
   }

   //Array< Array< std::pair<double,double> > > solve() const {
   Matrix< std::pair<double,double> > solve() const {
      check_ring(singRing);
      load_library("solve.lib");
      idhdl solve = get_singular_function("solve");
      sleftv arg;
      memset(&arg,0,sizeof(arg));
      arg.rtyp=IDEAL_CMD;
      arg.data=(void *)idCopy(singIdeal);
      // tell singular to skip printing the solutions
      arg.next=(leftv)omAlloc0(sizeof(sleftv));
      arg.next->rtyp=STRING_CMD;
      arg.next->data=omStrDup("nodisplay");
      // do not print the setring / SOL comment
      int plevel = printlevel;
      printlevel=-1;
      // call solve
      BOOLEAN res=iiMake_proc(solve,NULL,&arg);
      printlevel=plevel;
      if(!res && (iiRETURNEXPR.Typ() == RING_CMD)){
         // retrieve returned ring
         ring solring = (ring)iiRETURNEXPR.Data();
         // avoid redefinition message
         BITSET oldverb;
         SI_SAVE_OPT2(oldverb);
         si_opt_2 &= ~Sy_bit(V_REDEFINE);
         // switch to the new returned ring
         idhdl solRingHdl=enterid("solveRing", 0, RING_CMD, &IDROOT, FALSE);
         IDRING(solRingHdl)=solring;
         SI_RESTORE_OPT2(oldverb);
         rSetHdl(solRingHdl);
         // retrieve solution list SOL from the interpreter
         idhdl sol = ggetid("SOL");
         if (sol->typ != LIST_CMD)
            throw std::runtime_error("solve: could not find solution array SOL");
         lists L = (lists) IDDATA(sol);
         Matrix< std::pair<double,double> > result(L->nr+1, L->m[0].Typ() == LIST_CMD ? ((lists)L->m[0].Data())->nr+1 : 1);
   	   for(int j=0; j<=L->nr; j++){
            if (L->m[j].Typ() == LIST_CMD) {
               lists LL = (lists)L->m[j].Data();
               for(int k=0; k<=LL->nr; k++) {
                  // here we fetch the solutions as complex numbers (gmp_complex) and convert to 
                  // a pair of doubles:
                  // cnum->real() and cnum->imag() are gmp_float which can converted to double
                  // alternatively we could access the internal mpf_t float types ( mpfp() )
                  // which could be converted to mpfr_t (aka AccurateFloat)
                  // see "include/singular/mpr_complex.h"
                  gmp_complex* cnum = (gmp_complex*) LL->m[k].Data();
                  result(j,k) = std::make_pair(cnum->real(),cnum->imag());
               }
            } else if (L->m[j].Typ() == NUMBER_CMD) {
               // in the univariate case the list is only 1-dim
               // return value is Matrix with 1 column
               gmp_complex* cnum = (gmp_complex*) L->m[j].Data();
               result(j,0) = std::make_pair(cnum->real(),cnum->imag());
            }
         }
         return result;
      } else {
         throw std::runtime_error("solve: no ring returned");
      }
   }

   Array<Polynomial<>> reduce(const Array<Polynomial<>>& ideal) const {
      check_ring(singRing);
      SingularIdeal_impl toBeReduced(ideal, singRing);
      ::ideal q = kNF(singIdeal, NULL, toBeReduced.singIdeal);
      SingularIdeal_impl reduced(q, singRing);
      id_Delete(&q,IDRING(singRing));
      return reduced.polynomials();
   }

   Polynomial<> reduce(const Polynomial<>& p) const {
      check_ring(singRing);
      poly p_s = convert_Polynomial_to_poly(p,IDRING(singRing));
      poly q_s = kNF(singIdeal, NULL, p_s);
      Polynomial<> q_p(convert_poly_to_Polynomial(q_s));
      p_Delete(&q_s,IDRING(singRing));
      p_Delete(&p_s,IDRING(singRing));
      return q_p;
   }

   /*
    * Returns n+1 Polynomials, where the n+1 Polynomial is equal to the result of reduce and the previous are the coeffients of the division.
    */
   Array<Polynomial<>> division(const Polynomial<>& p, const bool is_std = 0) const {
      check_ring(singRing);
      ::ideal m = idInit(1,1);
      m->m[0] = convert_Polynomial_to_poly(p,IDRING(singRing));
      ::ideal R;
      matrix U;
      ::ideal t = idLift(singIdeal, m, &R, FALSE, is_std ? TRUE : FALSE, TRUE, &U);
      matrix T = id_Module2formatedMatrix(t, IDELEMS(singIdeal), 1, IDRING(singRing));
      const int rows = MATROWS(T);
      Array<Polynomial<>> polys(rows+1);
      for (int j = 1; j <= rows; j++) {
        if (MATELEM(T, j, 1)) {
          polys[j-1]=convert_poly_to_Polynomial(MATELEM(T, j, 1));
        } else {
          polys[j-1]=Polynomial<>(rVar(IDRING(singRing)));
        }
      }
      polys[rows]=convert_poly_to_Polynomial(R->m[0]);
      mp_Delete(&T,IDRING(singRing));
      mp_Delete(&U,IDRING(singRing));
      id_Delete(&R,IDRING(singRing));
      return polys;
   }

   // Converting singIdeal generators to an array of Polymake polynomials.
   Array<Polynomial<>> polynomials() const
   {
      check_ring(singRing);
      int numgen = IDELEMS(singIdeal);
      std::vector<Polynomial<>> polys;
      for(int j = 0; j<numgen; ++j) {
         if (singIdeal->m[j] != NULL) {
            polys.push_back(convert_poly_to_Polynomial(singIdeal->m[j]));
         }
      }
      return Array<Polynomial<>>(polys);
   }
   
   static SingularIdeal_impl* quotient(const SingularIdeal_impl* I, const SingularIdeal_impl* J);
};


SingularIdeal_impl* SingularIdeal_impl::quotient(const SingularIdeal_impl* I, const SingularIdeal_impl* J){
   // The first true indicates, that we receive a standard basis of I,
   // the second one that we want the output to be an ideal.
   ::ideal quot = idQuot(idCopy(I->singIdeal), idCopy(J->singIdeal), true, true);
   SingularIdeal_impl* quotient_impl = new SingularIdeal_impl(quot,I->singRing);
   id_Delete(&quot,IDRING(I->singRing));
   return quotient_impl;
}



perl::Object quotient(perl::Object I, perl::Object J)
{
   int ni = I.give("N_VARIABLES");
   int nj = J.give("N_VARIABLES");
   if (ni != nj)
      throw std::runtime_error("Ideals of different rings");

   check_ring(ni);
   
   const Array<Polynomial<>> gensI = I.give("GROEBNER.BASIS");
   const Matrix<int> order = I.give("GROEBNER.ORDER_MATRIX");
   SingularTermOrderData<Matrix<int> > TO = SingularTermOrderData<Matrix<int> >(ni, order);
   const idhdl sri = check_ring(ni, TO);
   const Array<Polynomial<>> gensJ = J.give("GENERATORS");
   
   SingularIdeal_impl implI(gensI,sri);
   SingularIdeal_impl implJ(gensJ,sri);

   SingularIdeal_impl* quotimpl = SingularIdeal_impl::quotient(&implI,&implJ);

   perl::Object res("Ideal");
   res.take("N_VARIABLES") << ni;
   res.take("GENERATORS") << quotimpl->polynomials();
   delete quotimpl;
   return res;
}

std::pair<SingularIdeal_wrap*, int> build_slack_ideal_minors(const Matrix<Rational>& slackMatrix, int dim) {
   init_singular();
   int nvars = 0;
   for(const auto& row : rows(slackMatrix)){
      for(const auto& e : row){
         nvars += e != 0;
      }
   }
   idhdl singRingHdl = check_ring(nvars);
   ring singRing = IDRING(singRingHdl);
   matrix variableSlackMatrix = mp_InitI(slackMatrix.rows(), slackMatrix.cols(), 0, singRing);
   int varcount = 0;
   for(int i=0; i<slackMatrix.rows(); i++){
      for(int j=0; j<slackMatrix.cols(); j++){
         if(slackMatrix(i,j) != 0){
            poly m = rGetVar(varcount+1, singRing);
            MATELEM(variableSlackMatrix,i+1,j+1) = p_Copy(m, singRing);
            varcount++;
         }
      }
   }

   int minorrk = dim+2;
   // The parameters may still be changed.
   ::ideal singMinor = getMinorIdeal(variableSlackMatrix, minorrk, 0, "Bareiss", NULL, true);
   return std::pair<SingularIdeal_wrap*, int>(new SingularIdeal_impl(singMinor, singRingHdl), nvars);
}

perl::Object slack_ideal_non_saturated(perl::Object P) {
   const Matrix<Rational> slackMatrix = P.give("SLACK_MATRIX");
   int dim = P.give("CONE_DIM");
   dim--;
   std::pair<SingularIdeal_wrap*, int> minor_wrap = build_slack_ideal_minors(slackMatrix, dim);
   perl::Object result("Ideal");
   result.take("N_VARIABLES") << minor_wrap.second;
   result.take("GENERATORS") << minor_wrap.first->polynomials();
   delete minor_wrap.first;
   return result;
}

} // end namespace singular

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<>>& gens, const Vector<int>& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<>>& gens, const Matrix<int>& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<>>& gens, const std::string& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}



UserFunction4perl("# @category Singular interface"
                  "# Computes an ideal quotient via SINGULAR"
                  "# @param Ideal I"
                  "# @param Ideal J"
                  "# @return Ideal",
                  &singular::quotient, "quotient(Ideal, Ideal)");

UserFunction4perl("# @category Singular interface"
                  "# Computes the non-saturated slack ideal of a polytope, as described in"
                  "# > João Gouveia, Antonio Macchia, Rekha R. Thomas, Amy Wiebe:"
                  "# > The Slack Realization Space of a Polytope"
                  "# > (https://arxiv.org/abs/1708.04739)",
                  &singular::slack_ideal_non_saturated, "slack_ideal_non_saturated($)");

} // end namespace ideal
} // end namespace polymake
