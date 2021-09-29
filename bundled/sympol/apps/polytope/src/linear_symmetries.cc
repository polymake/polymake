#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/sympol_config.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/group/permlib.h"
#include "polymake/permutations.h"

namespace polymake { namespace polytope {

namespace {
                
void add_action(BigObject& p,
                BigObject& g,
                const Matrix<Rational>& rays_or_facets,
                const Matrix<Rational>& lin,
                const AnyString& rf_prop,
                const std::string& action_name,
                const std::string& action_desc)
{
   // sympol permutes the generators of rays_and_facets as well as those of lin.
   // Since in polymake we are of the opinion that permuting the generators of a linear subspace doesn't make sense, we strip that part off
   const Array<Array<Int>>
      gens(generators_from_permlib_group(sympol_interface::sympol_wrapper::compute_linear_symmetries(rays_or_facets, lin))),
      subgens(lin.rows()
              ? permutation_subgroup_generators(gens, sequence(0, rays_or_facets.rows()))
              : gens);

   BigObject a("group::PermutationAction", action_name, "GENERATORS", subgens);
   a.set_description() << action_desc;
   p.add("GROUP", g);
   g.take(rf_prop) << a;
}

} // end anonymous namespace

// beware: sympol / permlib only work with Rational coordinates
     
//symmetry group computation vie edge colored graph automorphisms works for non-convex point sets, too. For the proof, see prop. 3.1 in "Polyhedral representation conversion up to symmetries" by Bremner, Sikiric and Schuermann, arxiv 0702239v2.
BigObject linear_symmetries_matrix(const Matrix<Rational>& M)
{
   Matrix<Rational> Mpty(0, M.cols());
   BigObject g(perl_group_from_group(sympol_interface::sympol_wrapper::compute_linear_symmetries(M, Mpty)));
   g.set_name("LinAut");
   g.set_description() << "linear symmetry group";
   return g;
} 

BigObject linear_symmetries_impl(BigObject p)
{
   Matrix<Rational> rays, facets;
   BigObject g("group::Group", "LinAut");
   g.set_description() << "linear symmetry group";
   
   if(p.type().name().find("Rational") == std::string::npos)
      throw std::runtime_error("linear_symmetries() only works with Rational coordinates.");

   if (p.isa("PointConfiguration")) {
      add_action(p, g, p.give("POINTS"), p.give("LINEAR_SPAN"), "POINTS_ACTION", "points_action", "action of LinAut on points");
   } else if (p.isa("VectorConfiguration")) {
      add_action(p, g, p.give("VECTORS"), p.give("LINEAR_SPAN"), "VECTOR_ACTION", "vector_action", "action of LinAut on vectors");
   } else {
      if (p.lookup("RAYS") >> rays)
         add_action(p, g, rays, p.give("LINEALITY_SPACE"), "RAYS_ACTION", "ray_action", "action of LinAut on rays/vertices");

      if (p.lookup("FACETS") >> facets)
         add_action(p, g, facets, p.give("LINEAR_SPAN"), "FACETS_ACTION", "facet_action", "action of LinAut on facets");
   }

   return g;
}
   
Matrix<Rational>
representation_conversion_up_to_symmetry(BigObject p, OptionSet options)
{
   Matrix<Rational> inequalities, equations;
   const bool v_to_h = options["v_to_h"];
   Array<Array<Int>> gens = p.give(v_to_h
                                   ? Str("GROUP.RAYS_ACTION.STRONG_GENERATORS | GROUP.RAYS_ACTION.GENERATORS")
                                   : Str("GROUP.FACETS_ACTION.STRONG_GENERATORS | GROUP.FACETS_ACTION.GENERATORS"));
   const std::string rayCompMethod = options["method"];

   const auto compMethod = (rayCompMethod == "lrs")
      ? sympol_interface::SympolRayComputationMethod::lrs
      : (rayCompMethod == "cdd"
         ? sympol_interface::SympolRayComputationMethod::cdd
         : (rayCompMethod == "beneath_beyond"
            ? sympol_interface::SympolRayComputationMethod::beneath_beyond
            : (rayCompMethod == "ppl"
               ? sympol_interface::SympolRayComputationMethod::ppl
               : sympol_interface::SympolRayComputationMethod::invalid)));
   if (compMethod == sympol_interface::SympolRayComputationMethod::invalid)
      throw std::runtime_error("Did not recognize ray computation method. Valid options are 'lrs', 'cdd', 'beneath_beyond', 'ppl'");

   const Matrix<Rational> rays_or_facets = p.give(v_to_h ? Str("RAYS") : Str("FACETS"));
   const Matrix<Rational> lin_sp = p.give(v_to_h ? Str("LINEALITY_SPACE") : Str("LINEAR_SPAN"));

   // appease sympol, which wants the automorphism group to act on both rays and lineality space
   const Int n = rays_or_facets.rows(), m = lin_sp.rows();
   if (m)
      for (auto& g: gens)
         g.append(m, entire(sequence(n, m)));
   const group::PermlibGroup sym_group(gens);
      
   if (!sympol_interface::sympol_wrapper::computeFacets(rays_or_facets, lin_sp, sym_group, compMethod, v_to_h, inequalities, equations))
      throw std::runtime_error("sympol computation of linear symmetry representatives not successful");
      
   return inequalities;
}


Function4perl(&linear_symmetries_impl, "linear_symmetries_impl($)");

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Computes the dual description of a polytope up to its linear symmetry group."
                  "# @param Cone c the cone (or polytope) whose dual description is to be computed, equipped with a GROUP"
                  "# @option Bool v_to_h 1 (default) if converting V to H, false if converting H to V" 
                  "# @option String method specifies sympol's method of ray computation via 'lrs' (default), 'cdd', 'beneath_beyond', 'ppl'" 
                  "# @return Pair (Matrix<Rational> vertices/inequalities, Matrix<Rational> lineality/equations)",
                  &representation_conversion_up_to_symmetry,"representation_conversion_up_to_symmetry(Cone<Rational>; { v_to_h => 1, method => 'lrs' })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
