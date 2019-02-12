#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/sympol_config.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/group/permlib.h"
#include "polymake/permutations.h"

namespace polymake { namespace polytope {

namespace {
                
void add_action(perl::Object& p,
                perl::Object& g,
                const Matrix<Rational>& rays_or_facets,
                const Matrix<Rational>& lin,
                const AnyString& rf_prop,
                const std::string& action_name,
                const std::string& action_desc)
{
   // sympol permutes the generators of rays_and_facets as well as those of lin.
   // Since in polymake we are of the opinion that permuting the generators of a linear subspace doesn't make sense, we strip that part off
   const Array<Array<int>>
      gens(generators_from_permlib_group(sympol_interface::sympol_wrapper::compute_linear_symmetries(rays_or_facets, lin))),
      subgens(lin.rows()
              ? permutation_subgroup_generators(gens, sequence(0, rays_or_facets.rows()))
              : gens);

   perl::Object a("group::PermutationAction");
   a.set_description() << action_desc;
   a.set_name(action_name);
   a.take("GENERATORS") << subgens;
   
   p.add("GROUP", g);
   p.take("GROUP." + rf_prop) << a;
}

} // end anonymous namespace

// beware: sympol / permlib only work with Rational coordinates
     
//symmetry group computation vie edge colored graph automorphisms works for non-convex point sets, too. For the proof, see prop. 3.1 in "Polyhedral representation conversion up to symmetries" by Bremner, Sikiric and Schuermann, arxiv 0702239v2.
perl::Object linear_symmetries_matrix(const Matrix<Rational>& M)
{
   Matrix<Rational> Mpty(0, M.cols());
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
representation_conversion_up_to_symmetry(perl::Object p, perl::OptionSet options)
{
   Matrix<Rational> inequalities, equations;
   const bool v_to_h = options["v_to_h"];
   Array<Array<int>> gens = p.give(v_to_h
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
   const int n(rays_or_facets.rows()), m(lin_sp.rows());
   if (m)
      for (auto& g: gens)
         g.append(m, entire(sequence(n, m)));
   const group::PermlibGroup sym_group(gens);
      
   if (!sympol_interface::sympol_wrapper::computeFacets(rays_or_facets, lin_sp, sym_group, compMethod, v_to_h, inequalities, equations))
      throw std::runtime_error("sympol computation of linear symmetry representatives not successful");
      
   return inequalities;
}

UserFunction4perl("# CREDIT sympol\n\n"
                  "# @category Symmetry"
                  "# Use sympol to compute the linear symmetries of"
                  "# - the rows of a rational matrix //m//, or"
                  "# - the RAYS|VERTICES, FACETS, or POINTS of a rational cone or polytope //C// (whatever is available, in this order), or"
                  "# - the VECTORS|POINTS of a rational vector or point configuration //P//."
                  "# Except for matrices, the action of the symmetry group is stored inside the parent object."
                  "# In the case of cones, sympol might compute only a subset of the linear symmetry group."
                  "# Sympol, and therefore this function, can only handle rational objects."
                  "# @param Matrix m | Cone //C// | VectorConfiguration //P//"
                  "# @return group::Group the linear symmetry group, together with a PERMUTATION_ACTION, VERTEX_ACTION, FACETS_ACTION, or VECTOR_ACTION"
                  "# @example"
                  "# > $ls = linear_symmetries(cube(2)->VERTICES);"
                  "# > print $ls->PERMUTATION_ACTION->GENERATORS;"
                  "# | 0 2 1 3"
                  "# | 3 1 2 0"
                  "# | 2 3 0 1"
                  "# "
                  "# > print linear_symmetries(cube(3)->VERTICES)->PERMUTATION_ACTION->GENERATORS;"
                  "# | 0 4 2 6 1 5 3 7"
                  "# | 0 1 4 5 2 3 6 7"
                  "# | 7 6 5 4 3 2 1 0"
                  "# | 2 6 0 4 3 7 1 5"
                  "# "
                  "# > print linear_symmetries(cube(3))->FACETS_ACTION->GENERATORS;"
                  "# | 1 0 2 3 4 5"
                  "# | 0 1 3 2 4 5"
                  "# | 2 3 0 1 4 5"
                  "# | 0 1 2 3 5 4"
                  "# | 0 1 4 5 2 3",
                  &linear_symmetries_matrix, "linear_symmetries(Matrix<Rational>)");

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
