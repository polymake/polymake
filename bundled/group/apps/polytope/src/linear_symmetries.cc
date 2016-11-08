#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace polytope {

namespace {
                
void add_action(perl::Object& g,
                const Matrix<Rational>& rays_or_facets,
                const Matrix<Rational>& lin,
                const AnyString& rf_prop,
                const std::string& action_name,
                const std::string& action_desc)
{
   g.take(rf_prop) << perl_action_from_group(sympol_interface::sympol_wrapper::compute_linear_symmetries(rays_or_facets, lin), action_name, action_desc);
}

} // end anonymous namespace

// beware: sympol / permlib only work with Rational coordinates
     
//symmetry group computation vie edge colored graph automorphisms works for non-convex point sets, too. For the proof, see prop. 3.1 in "Polyhedral representation conversion up to symmetries" by Bremner, Sikiric and Schuermann, arxiv 0702239v2.
perl::Object linear_symmetries_matrix(const Matrix<Rational>& M)
{
   Matrix<Rational> Mpty;
   perl::Object g(perl_group_from_group(sympol_interface::sympol_wrapper::compute_linear_symmetries(M, Mpty)));
   g.set_name("LinAut");
   g.set_description() << "linear symmetry group";
   return g;
} 

perl::Object linear_symmetries_impl(perl::Object p)
{
   Matrix<Rational> rays, facets;
   perl::Object g("group::Group");
   g.set_name("LinAut");
   g.set_description() << "linear symmetry group";

   if (p.lookup("RAYS") >> rays)
      add_action(g, rays, p.give("LINEALITY_SPACE"), "RAYS_ACTION", "ray_action", "action of LinAut on rays/vertices");

   if (p.lookup("FACETS") >> facets)
      add_action(g, facets, p.give("LINEAR_SPAN"), "FACETS_ACTION", "facet_action", "action of LinAut on facets");

   return g;
}
   
Matrix<Rational>
representation_conversion_up_to_symmetry(perl::Object p, bool v_to_h, int rayCompMethod)
{
   Matrix<Rational> inequalities, equations;
   Array<Array<int>> gens = p.give(v_to_h
                                   ? Str("GROUP.RAYS_ACTION.STRONG_GENERATORS | GROUP.RAYS_ACTION.GENERATORS")
                                   : Str("GROUP.FACETS_ACTION.STRONG_GENERATORS | GROUP.FACETS_ACTION.GENERATORS"));
   const sympol_interface::SympolRayComputationMethod compMethod = static_cast<sympol_interface::SympolRayComputationMethod>(rayCompMethod);
      
   const Matrix<Rational> rays_or_facets = p.give(v_to_h ? Str("RAYS") : Str("FACETS"));
   const Matrix<Rational> lin_sp = p.give(v_to_h ? Str("LINEALITY_SPACE") : Str("LINEAR_SPAN"));

   // appease sympol, which wants the automorphism group to act on both rays and lineality space
   const int n(rays_or_facets.rows()), m(lin_sp.rows());
   if (m)
      for (auto& g : gens)
         g.append(m, entire(sequence(n, m)));

   const group::PermlibGroup sym_group(gens);
      
   if (!sympol_interface::sympol_wrapper::computeFacets(rays_or_facets, lin_sp, sym_group, compMethod, v_to_h, inequalities, equations))
      throw std::runtime_error("sympol computation of linear symmetry representatives not successful");
      
   return inequalities;
}

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Computes the linear symmetries of a matrix //m//"
                  "# whose rows describe a point configuration via 'sympol'."
                  "# @param Matrix m holds the points as rows whose linear symmetry group is to be computed"
                  "# @return group::Group the linear symmetry group of //m//"
                  "# @example > $ls = linear_symmetries(cube(2)->VERTICES);"
                  "# > print $ls->GENERATORS;"
                  "# | 0 2 1 3"
                  "# | 3 1 2 0"
                  "# | 2 3 0 1",
                  &linear_symmetries_matrix,"linear_symmetries(Matrix<Rational>)");

Function4perl(&linear_symmetries_impl, "linear_symmetries_impl(Cone<Rational>)");

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Computes the dual description of a polytope up to its linear symmetry group."
                  "# @param Cone c the cone (or polytope) whose dual description is to be computed"
                  "# @param group::Group a symmetry group of the cone //c//"
                  "# @param Bool v_to_h true (default) if converting V to H, false if converting H to V" 
                  "# @param Int rayCompMethod specifies sympol's method of ray computation via lrs(0) (default), cdd(1), beneath_and_beyond(2), ppl(3)" 
                  "# @return Pair (Matrix<Rational> vertices/inequalities, Matrix<Rational> lineality/equations)",
                  &representation_conversion_up_to_symmetry,"representation_conversion_up_to_symmetry(Cone<Rational>; $=1, $=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
