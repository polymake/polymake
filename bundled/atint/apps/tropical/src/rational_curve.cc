/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	Functions to compute the combinatorics of rational n-marked curves, e.g.
	the metric-to-tree algorithm by Buneman
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/permutations.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical { 

	using namespace atintlog::donotlog;
	//using namespace atintlog::dolog;
	//   using namespace atintlog::dotrace;



	///////////////////////////////////////////////////////////////////////////////////////

	int moduliDimensionFromLength(int length) {
		int s = sqrt(1 + 8*length);
		int r = ((1+s) / 2);
		//Test for validity
		if((r*(r-1)/2) != length) {
			throw std::runtime_error("Length is not of the form (n over 2)");
		}
		return r;
	}

	/**
	  @brief Takes three rational values and checks whether two of them are equal and >= than the third
	  */
	inline bool fpcCheck(Rational a, Rational b, Rational c) {
		if(a == b && a >= c) return true;
		if(a == c && a >= b) return true;
		if(b == c && b >= a) return true;
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper of wrapTestFourPointCondition (does the same except that it returns
	// a vector of int)
	Vector<int> testFourPointCondition(Vector<Rational> v) {
		//Convert metric into map
		int n = moduliDimensionFromLength(v.dim());
		Matrix<Rational> d(n+1,n+1);

		int mindex = 0;
		for(int i = 1; i < n; i++) {
			for(int j = i+1; j <= n; j++) {
				d(i,j) = d(j,i) = v[mindex];
				mindex++;
			}
		}

		//First we test all 4-element subsets
		Set<int> complete = sequence(1,n);
		if(n >= 4) {
			Vector<Set<int> > fours(all_subsets_of_k(complete,4));
			//pm::Subsets_of_k<Set<int> > ( complete,4 );
			for(int f = 0; f < fours.dim(); f++) {
				Vector<int> l(fours[f]);
				Rational a = d(l[0],l[1]) + d(l[2],l[3]);
				Rational b = d(l[0],l[2]) + d(l[1],l[3]);
				Rational c = d(l[0],l[3]) + d(l[1],l[2]);
				//Check that two of a,b,c are equal and not less than the third
				if(!fpcCheck(a,b,c)) {
					Vector<int> fault;
					fault |= l[0];
					fault |= l[1];
					fault |= l[2];
					fault |= l[3];
					return fault;
				}
			}
		}
		//Now we check all 3-element subsets
		Vector<Set<int> > threes(all_subsets_of_k(complete,3));
		//pm::Subsets_of_k<Set<int> >(complete, 3);
		for(int f = 0; f < threes.dim(); f++) {
			Vector<int> l(threes[f]);
			//Now check the three possibilities, where the fourth element is equal to any of the three
			for(int t = 0; t < l.size(); t++) {
				Rational a = d(l[0],l[1]) + d(l[2],l[t]);
				Rational b = d(l[0],l[2]) + d(l[1],l[t]);
				Rational c = d(l[0],l[t]) + d(l[1],l[2]);
				//Check that two of a,b,c are equal and not less than the third
				if(!fpcCheck(a,b,c)) {
					Vector<int> fault;
					fault |= l[0];
					fault |= l[1];
					fault |= l[2];
					fault |= l[t];
					return fault;
				}
			}
		}
		//Now we check all 2-element subsets
		Vector<Set<int> > twos(all_subsets_of_k(complete,2));
		//pm::Subsets_of_k<Set<int> >(complete, 2);
		for(int f = 0; f < twos.dim(); f++) {
			Vector<int> l(twos[f]);
			//We have three possibilites for the other two z,t: t=x,z=y or t=z=x or t=z=y
			for(int p = 1; p <= 3; p++) {
				int t = p < 3? l[0] : l[1];
				int z = p != 2? l[1] : l[0];
				Rational a = d(l[0],l[1]) + d(z,t);
				Rational b = d(l[0],z) + d(l[1],t);
				Rational c = d(l[0],t) + d(l[1],z);
				//Check that two of a,b,c are equal and not less than the third
				if(!fpcCheck(a,b,c)) {
					Vector<int> fault;
					fault |= l[0];
					fault |= l[1];
					fault |= z;
					fault |= t;
					return fault;
				}
			}
		}
		Vector<int> result;
		return result;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	perl::ListReturn wrapTestFourPointCondition(Vector<Rational> v) {
		Vector<int> fault = testFourPointCondition(v);
		perl::ListReturn result;
		for(int i = 0; i < fault.dim(); i++) {
			result << fault[i];
		}
		return result;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Computes a rational curve (in the v_I-representation) and its graph from a given metric (or more precisely a vector equivalent to a metric). It is wrapped by curveFromMetric and graphFromMetric, which should be called instead and whose documentation can be found in the corr. perl wrappers rational_curve_from_metric and curve_graph_from_metric. Note that the order of the [[EDGES]] of the graph object that do not contain leaf vertices is the same as the order of the corresponding [[SETS]] of the curve. Furthermore, the first n vertices of the graph are the end vertices of the leave edges (in order 1 .. n)
	  */
	perl::Object curveAndGraphFromMetric(Vector<Rational> metric) {
		// We prepare the metric by making sure, all entries are > 0
		int n = moduliDimensionFromLength(metric.dim());

		//We now add Phi(sum of unit vectors) to the metric until it fulfills the four-point-condition
		//and is positive
		//Then we add it once more to make sure every leaf has positive distance to its vertex
		//Note that adding Phi(..) is the same as adding 2 to every entry in the matrix
		bool hasBeenfpced = false;
		bool hasBeenStretched = false;
		int tryCount = 0;
		while(!(hasBeenfpced && hasBeenStretched)) {
			//Since we cannot predict, how many tries we need to ensure 4-point-condition,
			//we insert a maximum bound to avoid endless looping on metrics that don't lie in M_n
			if(tryCount >= 1000000) {
				throw std::runtime_error("Cannot make metric four-point-condition-compatible: Maybe it's not in the moduli space?");
			}
			metric += 2*ones_vector<Rational>(metric.dim());
			//Check if everything is positive	
			if (accumulate(Set<Rational>(metric), operations::min()) <= 0) {
				continue;
			}
			//If it already had been 4-point-cond-compatible and positive, this was the stretching
			if(hasBeenfpced) hasBeenStretched = true;
			//Check if it fulfills the 4-pc
			if(testFourPointCondition(metric).dim() == 0) hasBeenfpced = true;
			else tryCount++;
		}

		//For simplicity we ignore the first row and column and start counting at 1
		Matrix<Rational> d(n+1,n+1); 
		int mindex = 0;
		for(int i = 1; i < n; i++) {
			for(int j = i+1; j <= n; j++) {
				d(i,j) = metric[mindex];
				d(j,i) = metric[mindex];
				mindex++;
			}
		}

		//     //Now check for nonpositive entries
		//dbgtrace << "Distance metric before making positive" << d <<  endl;
		//     for(int i = 1; i < n; i++) {
		//       for(int j = i+1; j <= n; j++) {
		// 	if(d(i,j) <= 0) {
		// 	  Vector<Rational> add = (-d(i,j) +1) * ones_vector<Rational>(d.cols());
		// 	  d.row(i) += add;
		// 	  d.col(i) += add;
		// 	  d(i,i) = 0;
		// 	}
		//       }
		//     }
		//dbgtrace << "Positive distance metric: " << d << endl;




		//dbgtrace << "Starting with metric matrix\n" << d << endl;

		//For the graph case, we prepare a graph object
		Graph<> G(n);

		//To make the order of SETS/COEFFS agree with the order of the edges in graph, 
		//we keep track of the original graph edge order.
		Vector<std::pair<int,int> > edge_order;

		//The algorithm possibly produces "double" vertices when creating a new vertex t
		// that has distance 0 to p or q. In this case we have to remember the original index p (or q)
		// to be able to create the graph correctly
		// So, at position i, it gives the number of the vertex (starting at 1), the virtual vertex i
		// actually represents. When creating a new vertex, this should be the maximum over the list + 1
		Vector<int> orig(n+1);
		for(int i = 1; i <= n; i++) { orig[i] = i;}

		//Result variable
		Vector<Rational> coeffs;
		Vector<Set<int> > sets;
		//Prepare vertex set, leaf map 
		//dbgtrace << "Moduli dimension is " << n << endl;
		Set<int> V = sequence(1,n);
		Map<int,Set<int> > leaves;
		for(int i = 1; i <=n; i++) {
			Set<int> singleset; singleset += i;
			leaves[i] = singleset;
		}
		//These variables will contain the node data
		Vector<Set<int> > nodes_by_leaves(n), nodes_by_sets(n);

		//dbgtrace << "Starting with leaf map " << leaves << endl;

		//Now inductively remove pairs of vertices until only 3 are left
		while(V.size() > 3) {
			//dbgtrace << "Have " << V.size() << " vertices. Reducing..." << endl;
			//Find the triple (p,q,r) that maximizes the Buneman term
			int p,q,r;
			p = q = r = 0;
			bool init = false;
			Rational max = 0;
			for(Entire<Set<int> >::iterator a = entire(V); !a.at_end(); a++) {
				for(Entire<Set<int> >::iterator b = entire(V); !b.at_end(); b++) {
					if(*b != *a) { 
						for(Entire<Set<int> >::iterator c = entire(V); !c.at_end(); c++) {
							if(*c != *b && *c != *a) {
								Rational newd = d(*a,*c) + d(*b,*c) - d(*a,*b);
								//dbgtrace << *a <<","<<*b<<","<<*c<<": " << newd << endl;
								if(newd > max || !init) {
									max = newd;
									p = *a; q = *b; r = *c;
									init = true;
								}
							}
						}
					}
				}
			} //End find maximal triple
			//dbgtrace << "Maximal triple is (" << p << "," << q << "," << r << ")" << endl;

			//Compute distances to the new virtual element t
			Rational dtp = (d(p,q) + d(p,r) - d(q,r));
			dtp /= Rational(2);
			//dbgtrace << "dtp: " << dtp << endl;
			Vector<Rational> dtx(d.cols()); //Again, start counting from 1
			int x = 0;
			for(Entire<Set<int> >::iterator i = entire(V); !i.at_end(); i++) {
				if(*i != p) {
					//dbgtrace << "Setting distance d(t," << *i << ")" << endl;
					dtx[*i] = d(*i,p) - dtp;
					if(*i != q && dtx[*i] == 0) {
						x = *i;
					}
				}
			}
			V = V - p; 
			V = V - q;
			//dbgtrace << "Computed new distances" << endl;

			//Now 'add' the new vertex
			if(x> 0) {
				//dbgtrace << "Attaching to vertex " << x << endl;
				leaves[x] = leaves[x] + leaves[p] + leaves[q];
				if(leaves[p].size() > 1 && leaves[p].size() < n-1) {
					coeffs |= d(p,x);
					sets |= leaves[p];
					nodes_by_sets[orig[p]-1] += (sets.dim()-1);
					nodes_by_sets[orig[x]-1] += (sets.dim()-1);
				}
				if(leaves[q].size() > 1 && leaves[q].size() < n-1) {
					coeffs |= d(q,x);
					sets |= leaves[q];
					nodes_by_sets[orig[q]-1] += (sets.dim()-1);
					nodes_by_sets[orig[x]-1] += (sets.dim()-1);
				}
				//If p or q are leaves, add them to the node leaves of x
				if(leaves[p].size() == 1) nodes_by_leaves[orig[x]-1] += leaves[p];
				if(leaves[q].size() == 1) nodes_by_leaves[orig[x]-1] += leaves[q];
				//Graph case
				G.edge(orig[p]-1,orig[x]-1);
				G.edge(orig[q]-1,orig[x]-1);
				edge_order |= std::pair<int,int>(orig[p]-1,orig[x]-1);
				edge_order |= std::pair<int,int>(orig[q]-1,orig[x]-1);
			}
			else {
				//Note: It can not be possible that t=q and q is a leaf, since the removal
				//of p would then yield a disconnected graph
				//dbgtrace << "Creating new vertex" << endl;
				// Graph case
				// If d(t,p) or d(t,q) = 0, identify t with p (or q)
				// Otherwise give t the next available node index
				if(dtp != 0 && dtx[q] != 0) {
					int node_number = G.add_node();
					orig |= (node_number+1);
					nodes_by_leaves |= Set<int>();
					nodes_by_sets |= Set<int>();
				}
				if(dtp == 0) {
					orig |= orig[p];
				}
				if(dtx[q] == 0) {
					orig |= orig[q];
				}
				//We update the distance matrix, since we add a new element
				d = d | zero_vector<Rational>();
				d = d / zero_vector<Rational>();
				int t = d.cols() -1;
				for(Entire<Set<int> >::iterator i = entire(V); !i.at_end(); i++) {
					d(*i,t) = d(t,*i) = dtx[*i];
				}

				//Now add the new vertex
				V += t;
				leaves[t] = leaves[p] + leaves[q];

				if(leaves[p].size() > 1 && leaves[p].size() < n-1) {
					if(dtp != 0) {
						coeffs |= dtp;
						sets |= leaves[p];
						nodes_by_sets[orig[p]-1] += (sets.dim()-1);
						nodes_by_sets[orig[t]-1] += (sets.dim()-1);
					}
				}
				if(leaves[q].size() > 1 && leaves[q].size() < n-1) {
					if(dtx[q] != 0) {
						coeffs |= dtx[q];
						sets |= leaves[q];
						nodes_by_sets[orig[q]-1] += (sets.dim()-1);
						nodes_by_sets[orig[t]-1] += (sets.dim()-1);
					}
				}

				//Now add the leaves
				if(leaves[p].size() == 1) nodes_by_leaves[orig[t]-1] += leaves[p];
				if(leaves[q].size() == 1) nodes_by_leaves[orig[t]-1] += leaves[q];


				//dbgtrace << "New orig vertex " << orig << endl;
				if(dtp != 0) {
					G.edge(orig[t]-1,orig[p]-1);
					edge_order |= std::pair<int,int>(orig[t]-1,orig[p]-1);
				}
				if(dtx[q] != 0) {
					G.edge(orig[t]-1,orig[q]-1);
					edge_order |= std::pair<int,int>(orig[t]-1,orig[q]-1);
				}
			}
			//dbgtrace << "Distance matrix\n" << d << endl;
			//dbgtrace << "Leaf map\n" << leaves << endl;
		} //End while(>3)

		//Now treat the basic cases of size 2 and 3
		Vector<int> vAsList(V);
		//dbgtrace << "G before rest cases: " << G << endl;
		if(V.size() == 3) {
			//dbgtrace << "Remaining: " << vAsList << endl;
			//Solve the linear system given by the pairwise distances
			Matrix<Rational> A(3,3);
			//Create the inverse matrix of the distance relation and multiply it with the distance vectors
			A(0,0) = A(0,1) = A(1,0) = A(1,2) = A(2,1) = A(2,2) = 0.5;
			A(0,2)  = A (1,1) = A(2,0) = -0.5;
			Vector<Rational> B(3);
			B[0] = d(vAsList[0],vAsList[1]); 
			B[1] = d(vAsList[0],vAsList[2]);
			B[2] = d(vAsList[1],vAsList[2]);
			//dbgtrace << "Solving " << A << "," << B << endl;
			Vector<Rational> a = A * B;
			//dbgtrace << "Result: " << a << endl;
			int zeroa = -1;
			Array<int> setsindices(3); //Indices of partitions in variable sets
			for(int i = 0; i < 3; i++) {
				setsindices[i] = -1;
				if(a[i] != 0) {
					if(leaves[vAsList[i]].size() > 1 && leaves[vAsList[i]].size() < n-1) {
						coeffs |= a[i];
						sets |= leaves[vAsList[i]];
						setsindices[i] = sets.dim()-1;
					}
				}
				else {
					zeroa = i;
				}
			}
			//Graph case
			//If all distances are nonzero, we add a vertex
			if(zeroa == -1) {
				int t = G.add_node();
				nodes_by_leaves |= Set<int>();
				nodes_by_sets |= Set<int>();
				for(int j=0;j<3;j++) { 
					G.edge(t,orig[vAsList[j]]-1);
					edge_order |= std::pair<int,int>(t,orig[vAsList[j]]-1);
					if(leaves[vAsList[j]].size() == 1) {
						nodes_by_leaves[t] += leaves[vAsList[j]];
					}
					else {
						nodes_by_sets[t] += setsindices[j];
						nodes_by_sets[orig[vAsList[j]]-1] += setsindices[j];
					}
				}	
			}
			//Otherwise we add the adjacencies of the two egdes
			else {
				for(int j=0; j<3; j++) {
					if(j != zeroa) {
						G.edge(orig[vAsList[j]]-1,orig[vAsList[zeroa]]-1);
						edge_order |= std::pair<int,int>(orig[vAsList[j]]-1,orig[vAsList[zeroa]]-1);
						if(leaves[vAsList[j]].size() ==1) {
							nodes_by_leaves[orig[vAsList[zeroa]]-1] += leaves[vAsList[j]];
						}
						else {
							nodes_by_sets[orig[vAsList[zeroa]]-1] += setsindices[j];
							nodes_by_sets[orig[vAsList[j]]-1] += setsindices[j];
						}
					}
				}
			}
		}//End case size == 3
		//dbgtrace << "G after case 3: " << G << endl;
		if(V.size() == 2) {    
			//Two remaining nodes forming a tree cannot both be leaves of the original tree
			//If necessary, we swap the list, so that the first element is the non-leaf
			if(leaves[vAsList[0]].size() == 1) {
				std::swap(vAsList[0],vAsList[1]);
			}
			//We only have a bounded edge, if the second element is also not a leaf
			if(leaves[vAsList[1]].size() > 1 && leaves[vAsList[1]].size() < n-1) {
				coeffs |= d(vAsList[0],vAsList[1]);
				sets |= leaves[vAsList[0]];
				nodes_by_sets[orig[vAsList[0]]-1] += (sets.dim()-1);
				nodes_by_sets[orig[vAsList[1]]-1] += (sets.dim()-1);
			}
			//Otherwise we add a leaf at the first element
			else {
				nodes_by_leaves[orig[vAsList[0]]-1] += leaves[vAsList[1]];
			}
			//Graph case
			G.edge(orig[vAsList[0]]-1,orig[vAsList[1]]-1);
			edge_order |= std::pair<int,int>(orig[vAsList[0]]-1,orig[vAsList[1]]-1);
		}


		//Now we're done, so we create the result

		//Create node labels
		Vector<std::string> labels(G.nodes());
		for(int i = 0; i < labels.dim(); i++) {
			if(i < n) {
				std::stringstream num;
				num << (i+1);
				labels[i] = num.str(); 
			}
			else {
				labels[i] = "";
			}
		}

		//Compute node degrees
		nodes_by_leaves = nodes_by_leaves.slice(~sequence(0,n));
		nodes_by_sets = nodes_by_sets.slice(~sequence(0,n));
		Vector<int> node_degrees(nodes_by_leaves.dim());
		for(int n = 0; n < node_degrees.dim(); n++) {
			node_degrees[n] = nodes_by_leaves[n].size() + nodes_by_sets[n].size();
		}

		//dbgtrace << "Graph: " << endl;
		//dbgtrace << "Nodes. " << G.nodes() << endl;
		//dbgtrace << "Edges: " << G.edges() << endl;
		//dbgtrace << "Adjacency: " << G << endl;
		//dbgtrace << "Labels: " << labels << endl;

		//     pm::cout << "Edge Order: " << edge_order << endl;
		//     pm::cout << "Sets and coeffs: " << sets << ", " << coeffs << endl;

		perl::Object graph("graph::Graph");
		graph.take("N_NODES") << G.nodes();
		graph.take("ADJACENCY") << G;
		graph.take("NODE_LABELS") << labels;

		//The edges in G might now have a different order than the one in which we put it in
		//Hence we have to make sure, the order of SETS and COEFFS is compatible with the EDGES order
		//For this we have kept edge_order as a record of the original order of edges.
		Array<Set<int> > edge_list_array = graph.CallPolymakeMethod("EDGES");
		Vector<Set<int> > edge_list(edge_list_array);
		//First we throw out all the edges in the original ordering that are actually leaves
		Set<int> leaf_edges;
		for(int oe = 0; oe < edge_order.dim(); oe++) {
			if(edge_order[oe].first < n || edge_order[oe].second < n) leaf_edges += oe;
		}
		edge_order = edge_order.slice(~leaf_edges);

		Vector<Set<int> > ordered_sets;
		Vector<Rational> ordered_coeffs;
		for(int e = 0; e < edge_list.dim(); e++) {
			//First, see if it has an entry < n. Then its actually a leaf, not a bounded edge
			Vector<int> elist(edge_list[e]);
			if(elist[0] >= n && elist[1] >= n) {
				//Check which edge (in the original ordering) agrees with this one
				int oeindex = -1;
				for(int oe = 0; oe < edge_order.dim(); oe++) {
					Set<int> oeset; oeset += edge_order[oe].first; oeset += edge_order[oe].second;
					if( (oeset * edge_list[e]).size() == 2) {
						oeindex = oe; break;
					}
				}
				ordered_sets |= sets[oeindex];
				ordered_coeffs |= coeffs[oeindex];
			}
		}//END re-order sets and coeffs



		if(sets.dim() == 0) {
			sets |= Set<int>();
			coeffs |= 0;
		}

		perl::Object curve("RationalCurve");
		curve.take("SETS") << ordered_sets;
		curve.take("COEFFS") << ordered_coeffs;
		curve.take("N_LEAVES") << n;
		curve.take("GRAPH") << graph;
		curve.take("GRAPH_EDGE_LENGTHS") << ordered_coeffs;
		curve.take("NODES_BY_LEAVES") << nodes_by_leaves;
		curve.take("NODES_BY_SETS") << nodes_by_sets;
		curve.take("NODE_DEGREES") << node_degrees;

		return curve;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	perl::Object curveFromMetric(Vector<Rational> metric) {
		return curveAndGraphFromMetric(metric);
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/*
	 * @brief Takes a vector from Q^(n over 2) that describes an n-marked rational abstract
	 * curve as a distance vector between its leaves. It then computes the
	 * graph of the curve corresponding to this vector.
	 * @param Vector<Rational> v A vector of length (n over 2). Its entries are
	 * interpreted as the distances d(i,j) ordered lexicographically according to i,j. However, they need not be positive, as long as v is equivalent to a proper
	 * metric modulo leaf lengths.
	 * @return An array containing first the graph::Graph and then a Vector<Rational>, containing
	 * the lengths of the bounded edges (in the order they appear in EDGES)
	 */
	perl::ListReturn graphFromMetric(Vector<Rational> metric) {
		//dbgtrace << "Recomputing curve with graph" << endl;
		perl::Object curve = curveAndGraphFromMetric(metric);
		perl::Object graph = curve.give("GRAPH"); 
		Vector<Rational> lengths = curve.give("COEFFS");
		perl::ListReturn result;
		result << graph.copy();
		result << lengths;
		return result;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief Takes a linear combination of abstract n-marked curves with 1 bounded edge, described by their partitions and the corresponding edge length and computes the resulting metric
	  @param IncidenceMatrix<> sets A list of partitions of {1,..,n}. May be redundant.
	  @param Vector<Set<int> > coeffs A list of arbitrary rational coefficients. Superfluous coefficients are ignored, missing ones replaced by 0.
	  @param int n The size of the leaf set
	  @return Vector<Rational> A curve metric of length (n over 2)
	  */
	Vector<Rational> metricFromCurve(IncidenceMatrix<> sets, Vector<Rational> coeffs, int n) {
		//Create distance matrix (we count from 1 for simplicity)
		Matrix<Rational> d(n+1,n+1);
		Set<int> completeSet = sequence(1,n);
		//Go through all sets
		for(int s = 0; s < sets.rows(); s++) {
			//If we have no more coefficients, stop calculating
			if(s >= coeffs.dim()) break;
			//Otherwise add the coefficients to the appropriate distances
			Rational c = coeffs[s];
			Set<int> sset = sets.row(s);
			Set<int> complement = completeSet - sset;
			for(Entire<Set<int> >::iterator selt = entire(sset); !selt.at_end(); selt++) {
				for(Entire<Set<int> >::iterator celt = entire(complement); !celt.at_end(); celt++) {
					//dbgtrace << "Adding " << c << " to distance of " << *selt << ", " << *celt << endl;
					d(*selt,*celt) += c;
					d(*celt, *selt) += c;
				}
			}
		}

		//Now convert to a vector
		Vector<Rational> result;
		for(int i = 1; i < n; i++) {
			for(int j = i+1; j <= n; j++) {
				result |= d(i,j);
			}
		}

		return result;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object rational_curve_from_matroid_coordinates(Vector<Rational> matroidVector) {

			matroidVector = matroidVector.slice(~scalar2set(0));

			//Convert vector to a map
			int n = moduliDimensionFromLength(matroidVector.dim())+1;
			Matrix<Rational> d(n,n);
			int index = 0;
			for(int i = 1; i < n-1; i++) {
				for(int j = i+1; j <= n-1; j++) {
					//The isomorphism is rigged for max, so we need to insert a sign here
					d(i,j) = (-Addition::orientation())*matroidVector[index];
					index++;
				}
			}

			//Now apply mapping
			Vector<Rational> metric;
			for(int i = 1; i < n; i++) {
				for(int j = i+1; j <= n; j++) {
					if(j == n) {
						metric |= 0;
					}
					else {
						metric |= (2* d(i,j));
					}
				}
			}
			//dbgtrace << metric << endl;

			return curveFromMetric(metric); 
		}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	template <typename Addition>
		perl::ListReturn rational_curve_list_from_matroid_coordinates(Matrix<Rational> m) {
			perl::ListReturn result;

			for(int i = 0; i < m.rows(); i++) {
				result << rational_curve_from_matroid_coordinates<Addition>(m.row(i));
			}

			return result;
		}

	///////////////////////////////////////////////////////////////////////////////////////
	/**
	  @brief Takes a rational n-marked abstract curve and computes its representation in the matroid coordinates of the moduli space
	  @param perl::Object The rational curve
	  @return Vector<Rational>
	  */
	template <typename Addition>
		Vector<Rational> matroid_coordinates_from_curve(perl::Object curve) {
			//Extract values
			IncidenceMatrix<> sets = curve.give("SETS");
			Vector<Rational> coeffs = curve.give("COEFFS");
			int n = curve.give("N_LEAVES");

			//Create edge index map (i,j) -> vector index
			Matrix<int> E(n,n); int index = 0;
			for(int i = 1; i < n-1; i++) {
				for(int j = i+1; j <= n-1; j++) {
					E(i,j) = E(j,i) = index;
					index++;
				}
			}

			//Compute ambient dimension of moduli space
			int raydim = (n*(n-3))/2 +1;
			Set<int> completeSet = sequence(1,n);

			Vector<Rational> result(raydim);

			//Map each set to matroid coordinates with appropriate coefficient
			for(int s = 0; s < sets.rows(); s++) {
				Set<int> sset = sets.row(s);
				//Make sure the set does not contain n
				if(sset.contains(n)) sset = completeSet - sset;
				//Now create the flat vector for the complete graph on vertices in sset
				Vector<int> slist(sset);
				for(int i = 0; i < slist.dim(); i++) {
					for(int j = i+1; j < slist.dim(); j++) {
						result[E(slist[i],slist[j])] += Addition::orientation() * coeffs[s];
					}
				}
			}

			result = (Rational(0) | result);
			return result;
		}

	// A wrapper with an additional unused templated argument, so we can call the function
	// with CallPolymakeFunction. (We can't move this to the header).
	template <typename Addition>
		Vector<Rational> matroid_vector(perl::Object curve,Addition flag) {
			return matroid_coordinates_from_curve<Addition>(curve);	
		}


	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see perl wrapper
	perl::ListReturn curveFromMetricMatrix(Matrix<Rational> m) {
		perl::ListReturn result;

		for(int i = 0; i < m.rows(); i++) {
			result << curveFromMetric(m.row(i));
		}

		return result;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	/**
	  @brief This computes the properties [[NODES_BY_SETS]] and [[NODES_BY_LEAVES]] of a RationalCurve object
	  */
	void computeNodeData(perl::Object curve) {
		Vector<Rational> metric = curve.CallPolymakeMethod("metric_vector");
		//dbgtrace << "Recomputing curve" << endl;
		//We recompute the curve to make sure the graph edges have the same order
		//as the curve sets
		perl::Object newcurve = curveAndGraphFromMetric(metric);

		//dbgtrace << "Computing sets permutation" << endl;

		//We might have to permute the column indices in the node matrices, since the sets might 
		// be in a different order in the actual curve
		//For this we have to normalize both set descriptions to contain the element 1

		int n = newcurve.give("N_LEAVES");
		Set<int> all_leaves = sequence(1,n);
		IncidenceMatrix<> newsetsInc = newcurve.give("SETS");
		Vector<Set<int> > newsets = incMatrixToVector(newsetsInc);
		for(int ns = 0; ns < newsets.dim(); ns++) {
			if(*(newsets[ns].begin()) != 1) newsets[ns] = all_leaves - newsets[ns];
		}
		IncidenceMatrix<>	oldsetsInc = curve.give("SETS");
		Vector<Set<int> > oldsets = incMatrixToVector(oldsetsInc); 
		for(int os = 0; os < oldsets.dim(); os++) {
			if(*(oldsets[os].begin()) != 1) oldsets[os] = all_leaves - oldsets[os];
		}
		//dbgtrace << "newsets: " << newsets << endl;
		//dbgtrace << "oldsets: " << oldsets << endl;
		Array<int> perm(newsets.dim());
		for(int i = 0; i < perm.size(); i++) {
			//Find equal set
			for(int j = 0; j < perm.size(); j++) {
				if(newsets[i] == oldsets[j]) {
					perm[i] = j; break;
				}
			}
		}
		//dbgtrace << "Permutation: " << perm << endl;

		IncidenceMatrix<> new_node_sets = newcurve.give("NODES_BY_SETS");
		//dbgtrace << "node sets: " << new_node_sets << endl;
		IncidenceMatrix<> node_leaves = newcurve.give("NODES_BY_LEAVES");
		Vector<int> node_degrees = newcurve.give("NODE_DEGREES");

		//Convert the node set matrix
		Vector<Set<int> > old_node_sets;
		for(int nns = 0; nns < new_node_sets.rows(); nns++) {
			Set<int> new_edge = new_node_sets.row(nns);
			Set<int> old_edge;
			for(Entire<Set<int> >::iterator ne = entire(new_edge); !ne.at_end(); ne++) {
				old_edge += perm[*ne];
			}
			old_node_sets |= old_edge;
		}

		curve.take("NODES_BY_LEAVES") << node_leaves;
		curve.take("NODES_BY_SETS") << old_node_sets;
		curve.take("NODE_DEGREES") << node_degrees;
	}

	// ------------------------- PERL WRAPPERS ---------------------------------------------------

	UserFunction4perl("# @category Abstract rational curves"
			"# Takes a vector from Q^(n over 2) that describes an n-marked rational abstract"
			"# curve as a distance vector between its leaves. It then computes the "
			"# curve corresponding to this vector."
			"# @param Vector<Rational> v A vector of length (n over 2). Its entries are "
			"# interpreted as the distances d(i,j) ordered lexicographically according to i,j. However, they need not be positive, as long as v is equivalent to a proper "
			"# metric modulo leaf lengths."
			"# @return RationalCurve",
			&curveFromMetric,"rational_curve_from_metric(Vector<Rational>)");

	UserFunctionTemplate4perl("# @category Abstract rational curves"
			"# Takes a vector from Q^((n-1) over 2) that lies in M_0,n (in its matroid coordinates) "
			"# and computes the corresponding rational curve."
			"# In the isomorphism of the metric curve space and the moduli coordinates"
			"# the last leaf is considered as the special leaf"
			"# @param Vector<Rational> v A vector in the moduli space (WITH leading coordinate)."
			"# @tparam Addition Min or Max (i.e. what are the matroid coordinates using)"
			"# @return RationalCurve",
			"rational_curve_from_matroid_coordinates<Addition>(Vector<Rational>)");

	UserFunctionTemplate4perl("# @category Abstract rational curves"
			"# Takes a matrix whose rows are elements in the moduli space M_0,n in matroid "
			"# coordinates. Returns a list, where the i-th element is the curve corr. to "
			"# the i-th row in the matrix"
			"# @param Matrix<Rational> m A list of vectors in the moduli space (with leading coordinate)."
			"# @tparam Addition Mir or Max (i.e. what are the matroid coordinates using"
			"# @return RationalCurve : An array of RationalCurves",
			"rational_curve_list_from_matroid_coordinates<Addition>(Matrix<Rational>)");

	UserFunction4perl("# @category Abstract rational curves"
			"# Takes a matrix whose rows are metrics of rational n-marked curves."
			"# Returns a list, where the i-th element is the curve corr. to "
			"# the i-th row in the matrix"
			"# @param Matrix<Rational> m"
			"# @return RationalCurve : An array of RationalCurves",
			&curveFromMetricMatrix, "rational_curve_list_from_metric(Matrix<Rational>)");

	UserFunction4perl("# @category Abstract rational curves" 
			"# Takes a metric vector in Q^{(n over 2)} and checks whether it fulfills "
			"# the four-point condition, i.e. whether it lies in M_0,n. More precisely "
			"# it only needs to be equivalent to such a vector"
			"# @param Vector<Rational> v The vector to be checked"
			"# @return Int A quadruple (array) of indices, where the four-point condition "
			"# is violated or an empty list, if the vector is indeed in M_0,n",
			&wrapTestFourPointCondition, "testFourPointCondition(Vector<Rational>)");

	UserFunctionTemplate4perl("# @category Abstract rational curves"
			"# Takes a rational curve and converts it into the corresponding matroid coordinates"
			"# in the moduli space of rational curves (including the leading 0 for a ray!)"
			"# @param RationalCurve r A rational n-marked curve"
			"# @tparam Addition Min or Max, i.e. which coordinates to use."
			"# @return Vector<Rational> The matroid coordinates, tropically homogeneous and"
			"# with leading coordinate",
			"matroid_coordinates_from_curve<Addition>(RationalCurve)");

	Function4perl(&graphFromMetric, "curve_graph_from_metric(Vector)");
	Function4perl(&metricFromCurve, "metric_from_curve(IncidenceMatrix, Vector<Rational>, $)");
	Function4perl(&computeNodeData, "compute_node_data(RationalCurve)");
	FunctionTemplate4perl("matroid_vector<Addition>(RationalCurve,Addition)");

}}
