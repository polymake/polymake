/* Copyright (c) 1997-2018
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

#include "polymake/client.h"
#include "polymake/socketstream.h"
#include "polymake/graph/SpringEmbedder.h"
#include "polymake/common/SimpleGeometryParser.h"
#include "polymake/common/SharedMemoryMatrix.h"

#include <pthread.h>
#include <unistd.h>
#include <signal.h>

namespace polymake { namespace graph {

class SpringEmbedderWindow {
protected:  
   pthread_t thr;
   socketstream js;
   SpringEmbedder SE;
   common::SharedMemoryMatrix<double> X;
   RandomSpherePoints<double> random_points;
   int iter_count, max_iter;
   std::string geom_name;
   common::SimpleGeometryParser::param_map params, defaults;
   common::SimpleGeometryParser::iparam_map iparams;
   bool needs_restart;

   static const std::string p_viscosity, p_inertion, p_repulsion, p_orientation, p_delay, p_step, p_continue, p_restart;

   void run();
   static void* run_it(void *me);
public:
   SpringEmbedderWindow(const Graph<>& G, perl::OptionSet options)
      : SE(G,options), X(G.nodes(),3), random_points(3), iter_count(0), needs_restart(false)
   {
      if (!(options["max-iterations"] >> max_iter)) max_iter=10000;
   }

   int port() const { return js.port(); }

   void start_thread();
   void shutdown();

   const std::string& get_name() const { return geom_name; }
   const common::SimpleGeometryParser::param_map& get_params() const { return params; }
   const common::SimpleGeometryParser::iparam_map& get_iparams() const { return iparams; }
   const Matrix<double>& get_points() const { return X; }

   void set_points() { needs_restart=true; } 
   void set_point(int i) { cerr << "SpringEmbedderWindow got an 'p' command!" << endl; }
      
   void set_facet(const Set<int>& f) { needs_restart|=(SE.set_fixed_vertices(f) != f); }
   void set_param(const std::string& key, double value);
   void restart(common::SimpleGeometryParser& parser);

   static const bool has_shared_matrix=true;
   int get_shared_matrix_id() const { return X.get_shmid(); }
};

void SpringEmbedderWindow::start_thread()
{
   if (pthread_create(&thr, 0, &run_it, this))
      throw std::runtime_error("error creating spring embedder thread");
}

void SpringEmbedderWindow::shutdown()
{
   pthread_join(thr, nullptr);
   js.discard_out();
}

void* SpringEmbedderWindow::run_it(void *param)
{
   sigset_t block;
   sigemptyset(&block);
   sigaddset(&block, SIGPIPE);
   pthread_sigmask(SIG_BLOCK, &block, nullptr);

   try {
      SpringEmbedderWindow *me=reinterpret_cast<SpringEmbedderWindow*>(param);
      me->run();
   } catch (const std::exception& ex) {
      cerr << "interactive spring embedder terminated with error: " << ex.what() << endl;
   }
   return nullptr;
}

const std::string SpringEmbedderWindow::p_viscosity("viscosity"), SpringEmbedderWindow::p_inertion("inertion"),
                                   SpringEmbedderWindow::p_repulsion("repulsion"), SpringEmbedderWindow::p_orientation("orientation"),
                                   SpringEmbedderWindow::p_delay("delay"), SpringEmbedderWindow::p_step("step"),
                                   SpringEmbedderWindow::p_continue("continue"), SpringEmbedderWindow::p_restart("restart");

void SpringEmbedderWindow::run()
{
   common::SimpleGeometryParser parser;

   // establish connection to Java GUI
   if (!getline(js, geom_name)) return;
   if (geom_name.substr(0,5) == "read ")
      geom_name=geom_name.substr(5);

   // initialize parameter maps for interchange
   params[p_repulsion]=SE.get_repulsion();
   iparams[p_repulsion]=true;
   params[p_viscosity]=SE.get_viscosity();
   iparams[p_viscosity]=false;
   params[p_inertion]=SE.get_inertion();
   iparams[p_inertion]=false;

   if (SE.has_z_ordering()) {
      params[p_orientation] = SE.get_z_factor();
      iparams[p_orientation]=true;
   }

   params[p_delay] = 50;
   params[p_step] = 0;
   params[p_continue] = 0;
   params[p_restart] = 0;
   defaults=params;

   // compute the first embedding
   SE.start_points(X,random_points.begin());
   SE.calculate(X,random_points,max_iter);
   parser.print_long(js,*this);

   parser.loop(js,*this);
}

void SpringEmbedderWindow::set_param(const std::string& key, double value)
{
   params[key]=value;
   if (key == p_repulsion) needs_restart|=(SE.set_repulsion(value) != value);
   else if (key == p_orientation) needs_restart|=(SE.set_z_factor(value) != value);
   else if (key == p_inertion) SE.set_inertion(value);
   else if (key == p_viscosity) SE.set_viscosity(value);
}

void SpringEmbedderWindow::restart(common::SimpleGeometryParser& parser)
{
   if (params[p_restart] != 0) {
      defaults[p_continue]=params[p_continue];
      params=defaults;
      SE.start_points(X,random_points.begin());
      if (params[p_continue]) SE.calculate(X,random_points,max_iter);
      parser.print_long(js,*this);
   } else {
      if (needs_restart) iter_count=0, needs_restart=false, SE.restart(X);
      if (int step=lround(params[p_step])) {
         while (!SE.calculate(X,random_points,step) && (iter_count+=step)<max_iter) {
            parser.print_short(js,*this,p_continue);
            if (!params[p_continue]) return;
            usleep(lround(params[p_delay]*1000));
            if (js.rdbuf()->in_avail()) return;
         }
      } else {
         SE.calculate(X,random_points,max_iter);
      }
      params[p_continue] = 0;
      parser.print_short(js,*this,p_continue);
   }
}

std::unique_ptr<SpringEmbedderWindow>
interactive_spring_embedder(const Graph<>& G, perl::OptionSet options)
{
   std::unique_ptr<SpringEmbedderWindow> sw=std::make_unique<SpringEmbedderWindow>(G, options);
   sw->start_thread();
   return sw;
}

Function4perl(&interactive_spring_embedder,

              // the next string is *one* long line
              "interactive_spring_embedder(props::Graph<Undirected>, "
              "   { scale => 1, balance => 1, viscosity => 1, inertion => 1, eps => undef,"
              "     'z-ordering' => undef, 'z-factor' => undef, 'edge-weights' => undef,"
              "      seed => undef, 'max-iterations' => 10000 }) ");

OpaqueClass4perl("SpringEmbedderWindow", std::unique_ptr<SpringEmbedderWindow>,
                 OpaqueMethod4perl("port()")
                 OpaqueMethod4perl("shutdown() : void")
                 );
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
