/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#include <Singular/libsingular.h>
#include <kernel/combinatorics/stairc.h>
#include <coeffs/mpr_complex.h>

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

   void create_singIdeal(const Array<Polynomial<> >& gens) {
      int npoly = gens.size();
      singIdeal = idInit(npoly,1);
      int j = 0;
      // Converting monomials as described in libsing-test2.cc.
      for(Entire<Array<Polynomial<> > >::const_iterator mypoly = entire(gens); !mypoly.at_end(); ++mypoly, ++j) {
         poly p = convert_Polynomial_to_poly(*mypoly,IDRING(singRing));
         //cout << "poly: " << p_String(p,singRing,singRing) << endl;
         singIdeal->m[j]=p_Copy(p,currRing);
      }
   }

public:
  
   // Constructing singIdeal from the generators:
   template<typename OrderType>
   SingularIdeal_impl(const Array<Polynomial<> >& gens, const OrderType& order)
   {
      pm::Ring<> polymakeRing = gens[0].get_ring();
      SingularTermOrderData<OrderType > TO = SingularTermOrderData<OrderType >(polymakeRing, order);
      singRing = check_ring(polymakeRing, TO);
      if(!gens.size())
         throw std::runtime_error("Ideal has no generators.");
      create_singIdeal(gens);
   }
   
   SingularIdeal_impl(const Array<Polynomial<> >& gens, idhdl r){
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
            id_Delete(&singIdeal,currRing);
      }
   }

   // Compute a groebner basis of a Polymake ideal using Singular
   void groebner() 
   {
      check_ring(singRing); 
      ::ideal res = kStd(singIdeal,NULL,testHomog,NULL);
      id_Delete(&singIdeal,currRing);
      singIdeal = res;
   }

   // Compute the dimension of an ideal.
   int dim() {
      check_ring(singRing); 
      return scDimInt(singIdeal, NULL);
   }
   

   SingularIdeal_wrap* initial_ideal() const {
      check_ring(singRing); 
      ::ideal res = id_Head(singIdeal,IDRING(singRing));
      SingularIdeal_wrap* initial = new SingularIdeal_impl(res,singRing);
      id_Delete(&res,currRing);
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
      // FIXME cleanup iiRETURNEXPR ?
      iiRETURNEXPR.CleanUp();
      iiRETURNEXPR.Init();
      return radical_wrap;
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
         // FIXME cleanup returndata ?
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

   Polynomial<> reduce(const Polynomial<>& p, const Ring<>& r) const {
      check_ring(singRing);
      poly q = kNF(singIdeal, NULL, convert_Polynomial_to_poly(p,IDRING(singRing)));
      return convert_poly_to_Polynomial(q,r);
   }

   /*
    * Returns n+1 Polynomials, where the n+1 Polynomial is equal to the result of reduce and the previous are the coeffients of the division.
    */
   Array< Polynomial<> > division(const Polynomial<>& p, const Ring<>& r) const {
      Array< Polynomial<> > m_generator(1, p);
      check_ring(singRing);
      SingularIdeal_impl m( m_generator, singRing );
      ::ideal R;
      matrix U;
      ::ideal t = idLift(singIdeal, m.singIdeal, &R, FALSE, FALSE /*Maybe true if std*/, TRUE, &U);
      matrix T = id_Module2formatedMatrix(t, IDELEMS(singIdeal), 1, IDRING(singRing));
      std::vector< Polynomial<> > polys;
      int rows = MATROWS(T);
      for(int j = 1; j <= rows; j++) {
        if(MATELEM(T, j, 1)) {
          polys.push_back(convert_poly_to_Polynomial(MATELEM(T, j, 1), r));
        } else {
          polys.push_back(Polynomial<>(0,r));
        }
      }
      polys.push_back(convert_poly_to_Polynomial(R->m[0],r));
      return Array< Polynomial<> >(polys);
   }

   // Converting singIdeal generators to an array of Polymake polynomials.
   Array<Polynomial<> > polynomials(const Ring<>& r) const
   {
      check_ring(singRing);
      int numgen = IDELEMS(singIdeal);
      std::vector<Polynomial<> > polys;
      for(int j = 0; j<numgen; j++) {
         if(singIdeal->m[j] != NULL){
            polys.push_back(convert_poly_to_Polynomial(singIdeal->m[j],r));
         }
      }
      return Array<Polynomial<> >(polys);
   }
   
   static SingularIdeal_impl* quotient(const SingularIdeal_impl* I, const SingularIdeal_impl* J);
};


SingularIdeal_impl* SingularIdeal_impl::quotient(const SingularIdeal_impl* I, const SingularIdeal_impl* J){
   // The first true indicates, that we receive a standard basis of I,
   // the second one that we want the output to be an ideal.
   ::ideal quot = idQuot(idCopy(I->singIdeal), idCopy(J->singIdeal), true, true);
   SingularIdeal_impl* quotient_impl = new SingularIdeal_impl(quot,I->singRing);
   id_Delete(&quot,currRing);
   return quotient_impl;
}



perl::Object quotient(perl::Object I, perl::Object J)
{
   Ring<> ri;
   Ring<> rj;
   I.give("RING") >> ri;
   J.give("RING") >> rj;
   if (ri.id() != rj.id())
      throw std::runtime_error("Ideals of different rings");

   check_ring(ri);
   
   const Array<Polynomial<> > gensI = I.give("GROEBNER.BASIS");
   const Matrix<int> order = I.give("GROEBNER.ORDER_MATRIX");
   SingularTermOrderData<Matrix<int> > TO = SingularTermOrderData<Matrix<int> >(ri, order);
   const idhdl sri = check_ring(ri, TO);
   const Array<Polynomial<> > gensJ = J.give("GENERATORS");
   
   SingularIdeal_impl implI(gensI,sri);
   SingularIdeal_impl implJ(gensJ,sri);

   SingularIdeal_impl* quotimpl = SingularIdeal_impl::quotient(&implI,&implJ);

   perl::Object res("Ideal");
   res.take("RING") << ri;
   res.take("GENERATORS") << quotimpl->polynomials(ri);
   delete quotimpl;
   return res;
}

} // end namespace singular

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<> >& gens, const Vector<int>& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<> >& gens, const Matrix<int>& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}

SingularIdeal_wrap* SingularIdeal_wrap::create(const Array<Polynomial<> >& gens, const std::string& ord) 
{
   return new singular::SingularIdeal_impl(gens,ord);
}



UserFunction4perl("# @category Singular interface"
                  "# Computes an ideal quotient via SINGULAR"
                  "# @param Ideal I"
                  "# @param Ideal J"
                  "# @return Ideal",
                  &singular::quotient, "quotient(Ideal, Ideal)");


} // end namespace ideal
} // end namespace polymake
