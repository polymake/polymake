
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "polymake/polytope/sympol_config.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/polytope/sympol_raycomputation_beneathbeyond.h"
#include "polymake/polytope/sympol_raycomputation_ppl.h"
#include "sympol/polyhedron.h"
#include "sympol/yal/reportlevel.h"
#include "sympol/symmetrygroupconstruction/computesymmetries.h"
#include "sympol/raycomputationlrs.h"
#include "sympol/raycomputationcdd.h"
#include "sympol/recursionstrategyidmadmlevel.h"
#include "sympol/facesuptosymmetrylist.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace polymake { namespace polytope { namespace sympol_interface {

   group::PermlibGroup sympol_wrapper::compute_linear_symmetries (const Matrix<Rational>& inequalities, const Matrix<Rational>& equations) {
      bool is_homogeneous = false;
      sympol::Polyhedron* sympolPoly = assembleSympolPolyhedron(inequalities, equations, false, is_homogeneous);

      sympol::ComputeSymmetries computeSymmetries(false, false);
      boost::shared_ptr<permlib::PermutationGroup> symmetryGroup = computeSymmetries.compute(*sympolPoly);

      delete sympolPoly;
      sympol::PolyhedronDataStorage::cleanupStorage();
                                  
      return group::PermlibGroup(symmetryGroup);

   }

   bool sympol_wrapper::computeFacets(const Matrix<Rational>& inequalities, const Matrix<Rational>& equations, const group::PermlibGroup& symmetry_group, 
              SympolRayComputationMethod rayCompMethod, int idmLevel, int admLevel, bool dual,
              Matrix<Rational>& out_inequalities, Matrix<Rational>& out_equations)
   {
      bool is_homogeneous = false;
      sympol::Polyhedron* sympolPoly = assembleSympolPolyhedron(inequalities, equations, dual, is_homogeneous);
      const unsigned int homogenity_offset = is_homogeneous
         ? 0
         : 1;
      
      boost::shared_ptr<permlib::PermutationGroup> symmetryGroup = symmetry_group.get_permlib_group();
      if ( symmetry_group.degree() != inequalities.rows() + equations.rows() )
        throw std::runtime_error("group DEGREE does not match size of input");
      
      sympol::RayComputation* rayComp = NULL;
      switch (rayCompMethod) {
        case cdd:
          rayComp = new sympol::RayComputationCDD();
          break;
        case lrs:
          rayComp = new sympol::RayComputationLRS();
          break;
        case beneath_beyond:
          rayComp = new RayComputationBeneathBeyond();
          break;
#ifdef POLYMAKE_WITH_PPL
        case ppl:
          rayComp = new RayComputationPPL();
          break;
#endif
      default:
         break;
      }
      if ( ! rayComp ) {
        throw std::runtime_error("Invalid ray computation method");
      }
      rayComp->initialize();
      sympol::RecursionStrategy* rs = NULL;
      if (idmLevel == 0 && admLevel == 0)
        rs = new sympol::RecursionStrategyDirect();
      else if (0 <= idmLevel && idmLevel <= admLevel)
        rs = new sympol::RecursionStrategyIDMADMLevel(idmLevel, admLevel);
      else
        throw std::runtime_error("Invalid recursion strategy. It must hold that 0 <= idmLevel <= admLevel");


      sympol::FacesUpToSymmetryList rd(*symmetryGroup, false, false);
      
      // compute dual description
      bool succ = rs->enumerateRaysUpToSymmetry(rayComp, *sympolPoly, *symmetryGroup, rd);

      unsigned int size = rd.size();
      if (!is_homogeneous && rd.firstVertexIndex() >= 0)
         --size;
      out_inequalities = Matrix<Rational>(size, sympolPoly->dimension()-homogenity_offset);

      // fill list of inequalities/vertices
      unsigned int r = 0, c;
      for (sympol::FacesUpToSymmetryList::FaceIt it = rd.begin(); it != rd.end(); ++it) {
         if ( !is_homogeneous && !(*it)->ray->isRay() )
            continue;
         assert( int(r) < out_inequalities.rows() );
         for (c = homogenity_offset; c < sympolPoly->dimension(); ++c) {
            assert( c < (*it)->ray->size() );
            out_inequalities[r][c-homogenity_offset].copy_from((*(*it)->ray)[c]);
         }
         ++r;
      }
      
      // compute and fill list of equations/lineality
      std::list<sympol::QArrayPtr> linearities;
      rayComp->getLinearities(*sympolPoly, linearities);
      out_equations = Matrix<Rational>(linearities.size(), sympolPoly->dimension()-homogenity_offset);
      r = 0;
      for (std::list<sympol::QArrayPtr>::const_iterator it = linearities.begin(); it != linearities.end(); ++it) {
         assert( int(r) < out_equations.rows() );
         for (c = homogenity_offset; c < sympolPoly->dimension(); ++c) {
            assert( c < (*it)->size() );
            out_equations[r][c-homogenity_offset].copy_from((**it)[c]);
         }
         ++r;
      }

      delete rs;
      rayComp->finish();
      delete rayComp;
      delete sympolPoly;
      sympol::PolyhedronDataStorage::cleanupStorage();
                                  
      return succ;
   }


   sympol::Polyhedron* sympol_wrapper::assembleSympolPolyhedron(const Matrix<Rational>& inequalities, const Matrix<Rational>& equations, bool dual, bool& is_homogeneous) {
      std::list<sympol::QArray> qarr = matrix2QArray(inequalities / equations, is_homogeneous);
      yal::ReportLevel::set(static_cast<yal::LogLevel>(yal::WARNING));
      // yal::ReportLevel::set(static_cast<yal::LogLevel>(yal::ERROR));
      sympol::PolyhedronDataStorage* polyStorage = sympol::PolyhedronDataStorage::createStorage(inequalities.cols() + (is_homogeneous?0:1), qarr.size());
      polyStorage->m_aQIneq.insert(polyStorage->m_aQIneq.end(), qarr.begin(), qarr.end());

      std::set<unsigned long> set_eqIndices;
      for (int i = 0; i < equations.rows(); ++i)
         set_eqIndices.insert(i + inequalities.rows());
      
      sympol::Polyhedron* p = new sympol::Polyhedron(
         polyStorage, 
         dual  
            ? sympol::Polyhedron::V
            : sympol::Polyhedron::H,
         set_eqIndices, std::set<unsigned long>()
      );
      
      if (!is_homogeneous)
         p->setHomogenized();
      return p;
   }
   
   std::list<sympol::QArray> sympol_wrapper::matrix2QArray(const Matrix<Rational>& A, bool& is_homogeneous)
   {
      const int n=A.cols();
      is_homogeneous = true;
      
      for (Entire< Rows< Matrix<Rational> > >::const_iterator r=entire(rows(A)); !r.at_end(); ++r) {
         if ((*r)[0] != 0) {
            is_homogeneous = false;
            break;
         }
      }
      
      const int homogenity_offset = is_homogeneous
         ? 0
         : 1;

      std::list<sympol::QArray> rowList;
      int idx = 0;
      
      for (Entire< Rows< Matrix<Rational> > >::const_iterator r=entire(rows(A)); !r.at_end(); ++r){
         sympol::QArray row(n+homogenity_offset, idx++);
         for (int j=0; j < n; ++j) {
            mpq_set(row[j+homogenity_offset],(*r)[j].get_rep());
         }
         rowList.push_back(row);
      }
      
      return rowList;
   }


} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
