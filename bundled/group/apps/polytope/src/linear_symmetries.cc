#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace polytope {

   // beware: sympol / permlib only work with Rational coordinates

   perl::Object linear_symmetries(perl::Object p, bool dual)
   {
      group::PermlibGroup sym_group;
      const bool isPolytope = p.isa("polytope::Polytope");
      if (dual) { //V-representation
         sym_group = sympol_interface::sympol_wrapper::compute_linear_symmetries(p.give("VERTICES | RAYS"), p.give("LINEALITY_SPACE"));
      } else {
         sym_group = sympol_interface::sympol_wrapper::compute_linear_symmetries(p.give("FACETS"), p.give("AFFINE_HULL | LINEAR_SPAN"));
      }
      static const char* groupPoly = "group::GroupOfPolytope";
      static const char* groupCone = "group::GroupOfCone";
      const char* correctGroupType = (isPolytope) ? groupPoly : groupCone;
      perl::Object g(correctGroupType);
      g.set_name("linear_symmetries");
      g.set_description() << "linear symmetries of " << p.description();
      g.take("DOMAIN") << 
         (dual 
            ? polymake::group::OnRays
            : polymake::group::OnFacets
         ); //if dual, then action on vertices(rays), else on facets
      return polymake::group::correct_group_from_permlib_group(g, sym_group);
   }
   
   perl::ListReturn representation_conversion_up_to_symmetry(perl::Object p, perl::Object group, bool dual, int rayCompMethod)
   {
      Matrix<Rational> inequalities, equations;
      perl::ListReturn result;
      group::PermlibGroup sym_group = polymake::group::group_from_perlgroup(group);
      bool success = false;
      const int sym_group_domain = group.give("DOMAIN");
      assert( sym_group_domain );

      sympol_interface::SympolRayComputationMethod compMethod = static_cast<sympol_interface::SympolRayComputationMethod>(rayCompMethod);

      if (dual) {
         if ( sym_group_domain != polymake::group::OnRays )
            throw std::runtime_error("group DOMAIN does not match 'dual' parameter (expected OnRays)");
         success = sympol_interface::sympol_wrapper::computeFacets(p.give("VERTICES | RAYS"), p.give("LINEALITY_SPACE"), sym_group, compMethod, dual, inequalities, equations);

      } else {
         if ( sym_group_domain != polymake::group::OnFacets )
            throw std::runtime_error("group DOMAIN does not match 'dual' parameter (expected OnFacets)");

         success = sympol_interface::sympol_wrapper::computeFacets(p.give("FACETS"), p.give("AFFINE_HULL | LINEAR_SPAN"), sym_group, compMethod, dual, inequalities, equations);
         
      }
      result << success << inequalities << equations;
      return result;
   }

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Computes the linear symmetries of a given polytope //p//"
                  "# via 'sympol'. If the input is a cone, it may compute only a subgroup"
                  "# of the linear symmetry group."
                  "# @param Cone c the cone (or polytope) whose linear symmetry group is to be computed"
                  "# @param Bool dual true if group action on vertices, false if action on facets" 
                  "# @return group::GroupOfCone the linear symmetry group of //p// (or a subgroup if //p// is a cone)",
                  &linear_symmetries,"linear_symmetries(Cone<Rational> $)");

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Computes the dual description of a polytope up to its linear symmetry group."
                  "# @param Cone c the cone (or polytope) whose dual description is to be computed"
                  "# @param group::Group a symmetry group of the cone //c// ([[group::GroupOfCone]] or [[group::GroupOfPolytope]])"
                  "# @param Bool dual true if V to H, false if H to V" 
                  "# @param Bool rayCompMethod specifies sympol's method of ray computation via lrs(0), cdd(1), beneath_and_beyond(2), ppl(3)" 
                  "# @return perl::ListReturn list which contains success as bool, vertices/inequalities and lineality/equations as [[Matrix<Rational>]]",
                  &representation_conversion_up_to_symmetry,"representation_conversion_up_to_symmetry(Cone<Rational>, group::Group $ $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
