/* Copyright (c) 1997-2022
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

#include "polymake/client.h"
#include "polymake/socketstream.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/schlegel_common.h"
#include "polymake/common/SimpleGeometryParser.h"
#include "polymake/common/SharedMemoryMatrix.h"

#include <pthread.h>
#include <signal.h>

namespace polymake { namespace polytope {
namespace {

template <typename Vector> inline
Matrix<typename Vector::element_type>
rotation(const GenericVector<Vector>& H)
{
   return null_space_oriented(H, -1) / H;
}

}

class SchlegelWindow {
protected:
   pthread_t thr;
   socketstream js;
   Int d, proj_facet;
   Matrix<double> Vertices, NeighborFacets, inv_Rotation;
   common::SharedMemoryMatrix<double> Points;
   Vector<double> ViewRay, FacetPoint, LastDragged;
   IncidenceMatrix<> VIF;
   std::string geom_name;
   common::SimpleGeometryParser::param_map params;
   common::SimpleGeometryParser::iparam_map iparams;
   double zoom;
   Int dragged_point;
   enum drag_response_t { drag_OK, drag_ignore, drag_boundary };
   drag_response_t drag_response;
   bool constrained;

   static const std::string p_zoom;

   void run();
   static void* run_it(void *me);
   void compute_points();
   double inverse_zoom();
public:
   SchlegelWindow(const Matrix<double>& V, const Matrix<double>& F, const IncidenceMatrix<>& VIF_arg, const Graph<>& FG,
                  const Vector<double>& FacetPoint_arg, const Vector<double>& InnerPoint,
                  Int facet_index, double zoom_arg);

   int port() const { return js.port(); }

   void start_thread();
   void shutdown();

   const std::string& get_name() const { return geom_name; }
   const common::SimpleGeometryParser::param_map& get_params() const { return params; }
   const common::SimpleGeometryParser::iparam_map& get_iparams() const { return iparams; }

   const Matrix<double>& get_points() const { return Points; }

   void set_param(const std::string& key, const double val)
   {
      if (key==p_zoom) {
         if (val != zoom) {
            zoom=val;
            compute_points();
            params[key]=val;
            drag_response=drag_OK;
            dragged_point=-1;
         }
      }
   }
   void set_point(Int i);
   void restart(common::SimpleGeometryParser& parser);

   BigObject store() const;

   void set_facet(const Set<Int>&) { cerr << "SchlegelWindow got an 'f' command!" << endl; }
   void set_points() { cerr << "SchlegelWindow got an 'P' command!" << endl; }

   static const bool has_shared_matrix=true;
   int get_shared_matrix_id() const { return Points.get_shmid(); }
};

SchlegelWindow::SchlegelWindow(const Matrix<double>& V, const Matrix<double>& F, const IncidenceMatrix<>& VIF_arg, const Graph<>& FG,
                               const Vector<double>& FacetPoint_arg, const Vector<double>& InnerPoint,
                               Int facet_index, double zoom_arg)
   : d(V.cols()-1), proj_facet(facet_index), Points(V.rows(),d-1),
     ViewRay(d+1), FacetPoint(unit_vector<double>(d+1,0)), LastDragged(d-1), VIF(VIF_arg),
     zoom(zoom_arg), dragged_point(-1)
{
   // orthogonal transformation: translate the origin to FacetPoint_arg and rotate F[proj_facet] to (0,0,...,1)
   Matrix<double> R=rotation(0.0 | F[proj_facet].slice(range_from(1)));
   orthogonalize(entire(rows(R.minor(range(1,d-1), All).top())));
   normalize(entire(rows(R.minor(range(1,d), All).top())));
   R.col(0).slice(range_from(1)) = (-FacetPoint_arg * T(R)).slice(range_from(1));

   Vertices=V * T(R);
   inv_Rotation=inv(R);
   NeighborFacets=F.minor(FG.adjacent_nodes(proj_facet),All) * inv_Rotation;
   ViewRay.slice(range_from(1)) = (-InnerPoint * T(R)).slice(range_from(1));
}

void SchlegelWindow::start_thread()
{
   if (pthread_create(&thr, nullptr, &run_it, this))
      throw std::runtime_error("error creating schlegel_interactive thread");
}

void SchlegelWindow::shutdown()
{
   pthread_join(thr, nullptr);
   js.discard_out();
}

void* SchlegelWindow::run_it(void *param)
{
   sigset_t block;
   sigemptyset(&block);
   sigaddset(&block, SIGPIPE);
   pthread_sigmask(SIG_BLOCK, &block, nullptr);

   try {
      SchlegelWindow *me=reinterpret_cast<SchlegelWindow*>(param);
      me->run();
   } catch (const std::exception& ex) {
      cerr << "schlegel_interactive terminated with error: " << ex.what() << endl;
   }
   return nullptr;
}

const std::string SchlegelWindow::p_zoom("zoom");

void SchlegelWindow::run()
{
   common::SimpleGeometryParser parser;

   // establish connection to Java GUI
   if (!getline(js, geom_name)) return;
   if (geom_name.substr(0,5) == "read ")
      geom_name=geom_name.substr(5);

   params[p_zoom]=zoom;
   iparams[p_zoom]=true;
   inverse_zoom();
   compute_points();
   parser.print_long(js, *this);

   parser.loop(js, *this);
}

double SchlegelWindow::inverse_zoom()
{
   const double alpha=schlegel_nearest_neighbor_crossing(NeighborFacets, FacetPoint, ViewRay);
   if ((constrained= alpha>=0 && alpha<1e8)) ViewRay *= alpha;
   return alpha;
}

void SchlegelWindow::compute_points()
{
   const double z = constrained ? zoom : zoom/(1-zoom), VR_d=-ViewRay[d];
   const sequence visible_coord=range(1,d-1);
   Points = Vertices.minor(All,visible_coord);
   auto p_i = rows(Points).begin();
   for (auto P_d = entire(Vertices.col(d)); !P_d.at_end(); ++P_d, ++p_i) {
      (*p_i) = ( VR_d * ((*p_i)-FacetPoint.slice(visible_coord)) + (*P_d) * ViewRay.slice(visible_coord) ) /
         ( (*P_d)/z + VR_d );
   }
}

void SchlegelWindow::set_point(Int i)
{
   if (i != dragged_point) {
      dragged_point=i;
      LastDragged=Points[i];
      return;
   }

   Vector<double> delta=LastDragged-Points[i];
   if (sqr(delta)<1e-5) return;

   if (constrained) ViewRay *= zoom;
   else if (zoom!=1) ViewRay *= zoom/(1-zoom);

   const bool on_proj_facet=VIF(proj_facet,i);
   if (!on_proj_facet)
      delta *= ViewRay[d]/Vertices(i,d)-1.;

   const double beta = schlegel_nearest_neighbor_crossing(NeighborFacets,
                                                          FacetPoint + ViewRay,
                                                          (0 | delta | 0));
   drag_response= beta<=1 ? drag_boundary : drag_OK;

   if (drag_response==drag_boundary) {
      if (beta<=1e-6) {
         Points[i]=LastDragged;
         return;
      }
      delta*=beta;
   }
   if (on_proj_facet)
      FacetPoint.slice(range(1,d-1)) += delta;
   else
      ViewRay.slice(range(1,d-1)) += delta;

   if (drag_response==drag_boundary) {
      constrained=true;
      zoom=1;
   } else {
      const double alpha=inverse_zoom();
      zoom= constrained ? 1/alpha : 0.5;
   }
   params[p_zoom]=zoom;
   compute_points();
   LastDragged=Points[i];
}

BigObject SchlegelWindow::store() const
{
   const Vector<double> FP = FacetPoint * T(inv_Rotation),
                        IP = FP - ViewRay * T(inv_Rotation);
   return BigObject("SchlegelDiagram",
                    "FACET", proj_facet,
                    "ZOOM", zoom,
                    "FACET_POINT", FP,
                    "INNER_POINT", IP);
}

void SchlegelWindow::restart(common::SimpleGeometryParser& parser)
{
   switch (drag_response) {
   case drag_OK:
      parser.print_short(js, *this, p_zoom);
      break;
   case drag_boundary:
      parser.print_error(js, *this, p_zoom, "boundary of projection facet reached");
      break;
   case drag_ignore:
      parser.print_empty(js);
      break;
   }
   drag_response=drag_ignore;
}

std::unique_ptr<SchlegelWindow>
schlegel_interactive(BigObject SD, const Matrix<double>& Points)
{
   BigObject P = SD.parent();
   const Matrix<double> F = P.give("FACETS");
   const IncidenceMatrix<> VIF = P.give("VERTICES_IN_FACETS");
   const Graph<> FG = P.give("DUAL_GRAPH.ADJACENCY");

   const Vector<double> FacetPoint = SD.give("FACET_POINT"),
      InnerPoint=SD.give("INNER_POINT");
   const Int proj_facet = SD.give("FACET");
   const double zoom = SD.give("ZOOM");

   std::unique_ptr<SchlegelWindow> sw=std::make_unique<SchlegelWindow>(Points, F, VIF, FG, FacetPoint, InnerPoint, proj_facet, zoom);
   sw->start_thread();
   return sw;
}

Function4perl(&schlegel_interactive, "schlegel_interactive(SchlegelDiagram, Matrix)");

OpaqueClass4perl("SchlegelWindow", std::unique_ptr<SchlegelWindow>,
                 OpaqueMethod4perl("port()")
                 OpaqueMethod4perl("store()")
                 OpaqueMethod4perl("shutdown()")
                 );
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
