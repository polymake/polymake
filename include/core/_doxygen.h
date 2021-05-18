/* Copyright (c) 1997-2021
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

#pragma once
// Artificial header file whose sole purpose is to be processed by doxygen

/** @namespace pm
    @brief global namespace for all classes from the %polymake project

    The most common classes/functions/... are exported to the namespace
    polymake.  Use this for your client code.
 */

/** @namespace polymake
    @brief namespace to be used for client code
 */

namespace pm {

/** @mainpage

    @section design_sec Design Principles

    @subsection container_view_sec Container-centric view

    PTL is based on STL and makes exhaustive use of the
    three main concepts of the latter: containers, iterators, and
    functors. There is a subtle difference, however, in the implementation of
    algorithms: in STL, they are designed to work with iterators, while in PTL
    the most of them accept containers as arguments. PTL containers are more
    than pure data structures, they bear some additional semantics, in that
    they belong to one of the @ref generic "generic families".  See
    http://www.cplusplus.com/reference/stl/ for a description of STL's
    container concept.

    Besides real containers, which own their data as prescribed by the
    standard, PTL makes heavy use of @ref manipulations. The latter implement
    though the standard container interface, but do not possess any data at
    all; instead, they refer to other container objects supplying the data
    items. There are three kinds of pseudo-containers in PTL: @ref alias_sec
    alias, @ref masquerade_sec masquerade, and @ref lazy_sec "lazy
    containers".

    @subsection lazy_sec Lazy evaluation

    Many operators defined for PTL container classes, such as vector and
    matrix arithmetic, set-theoretical %operations, etc., don't perform the
    computations immediately, but rather create a temporary object which
    "remembers" the operands and the operation and evaluates it later, on
    demand. This is a well-known technique called @em{lazy evaluation},
    sometimes also referred to as @em{expression templates}. It helps to spare
    unnecessary computations and copying of objects.
    
    The lazy objects fit well in the container concept of PTL, as they always
    belong to the same generic family and implement the same container
    interface as the result of the real operation would do.

    @see 
    - @ref refcounting "Reference Counting"
    - @ref generic "Generic and Persistent Types"

    @section container_sec Container Classes

    @subsection set_sec Set Types

    - Bitset is a container class for dense sets of integers.  Data
      representation is a bit string as implemented in the GNU Multiprecision
      Library (mpz_t).
 
    - Set is a container class for sparse sets of ordered elements.  Data
      representation as a modified kind of AVL tree.

    @subsection vector_sec Vector Types

    The vector class family implement vectors in the usual algebraic notion.
    It contains three @ref generic "persistent" vector types with different
    data storage organization.  These implementations result in @ref
    vector_performance "performance differences" for various functions on
    vectors.

    - Vector holds the elements in a contiguous array.

    - SparseVector is an associative container with element indices
      (coordinates) as keys; elements equal to the default value (@c
      ElementType(), which is 0 for most numerical types) are not stored, but
      implicitly encoded by the gaps in the key set.  It is based on an AVL
      tree.

    @subsection matrix_sec Matrix Types

    The matrix class family implement matrices in the usual algebraic notion.
    It contains three @ref generic "persistent" matrix types with different
    data storage organization.

    - Matrix holds the elements in a contiguous array.

    - SparseMatrix is a two-dimensional associative array with row and column
      indices as keys; elements equal to the default value (@c ElementType(),
      which is 0 for most numerical types) are not stored, but implicitly
      encoded by the gaps in the key set. Each row and column is organized as
      an AVL::tree.

    - ListMatrix is a list of row vectors. Based on @c std::list. Can be
      parameterized with any \ref generic "persistent" \ref vector_sec "vector
      type".

    @subsection other_sec Other Container Types

    - Array class with @ref refcounting "smart pointers".

    - Map - an associative container class based on an AVL tree structure.

    - The AVL::tree class implements balanced binary search trees.

    @section generic_sec Generic Class Families

    - GenericVector - an abstract vector class

    - GenericMatrix - an abstract matrix class 

    - GenericSet - an abstract set class 

    @section gmp_sec Number Types

    These are borrowed from @ref GMP "GMP" and wrapped.

    - Integer
    - Rational
    - QuadraticExtension


    @section polytope_sec Classes Useful for Computions in Geometric Combinatorics

    - EquivalenceRelation
    - FacetList
    - graph::Graph
    - IncidenceMatrix
*/

/** @page vector_performance Performance Comparison of Vector Classes
 
    The following brief analysis might help you when choosing the most efficient representation for a concrete case.
    @a n is the number of (non-zero in sparse case) elements in the vector.

<table class="borderless" border="0" cellspacing="10">
   <caption>Performance comparison</caption>
   <tr>
      <th align="left">Operation</th>
      <th align="left">Vector</th>
      <th align="left">SparseVector</th>
   </tr>
   <tr>
      <td>sequential iteration</td>
      <td>O(n)</td>
      <td>O(n), but with greater overhead constant</td>
   </tr>
   <tr>
      <td>random element access</td>
      <td>O(1)</td>
      <td>O(log(n))</td>
   </tr>
   <tr>
      <td>append an element</td>
      <td>O(n) + reallocation costs</td>
      <td>O(1) if no preceding random access operations were performed on the sparse vector; O(log(n)) if there already were random access operations on the sparse vector</td>
   </tr>
</table>

*/

/** @page refcounting Reference Counting

   Most of the top-level PTL data structures keep their data attached to a smart pointer combined with a reference
   counter.  This technique provides for cheap copy and assignment operations and return of complex objects by value.
   As long as several objects are not changed, they can safely share a single physical collection of data.  A non-const
   access eventually forces the creation of one's own copy of the data (so called @em{copy-on-write strategy}.)
   Obviously, there's also a drawback of reference counting, since the counter checking produces some overhead.  So, one
   should declare one's objects @c const whenever one can - this will surely bring a considerable runtime gain.

   In general, another danger usually entailed by reference counting is the possibility of creating self-referenced
   cyclic structures which can never get destroyed.  However, this is impossible in the PTL, since we do not use
   any recursive list structures.
 */

/** @page generic Generic and Persistent Types

   Many of the PTL container classes have "relative" classes having much the same sematics and interface but different
   data organization as the "primary" class. Especially the wide deployment of the lazy evaluation technique has born a
   plenty of relative classes.  For instance, an object representing a sum of two vectors or a slice of a vector (both
   using lazy evaluation) should be usable in every context where a normal vector object (or at least an immutable one)
   is accepted.

   The classical approach here would be to define an abstract vector base class with all methods declared as pure
   virtual functions, and derive everything from it. Although it would be a clearer design, it causes a serious
   performance penalty due to the fact that virtual functions practically inhibit inlining, and thus impair the
   compiler's optimization. Fortunately, a template library can make use of more tricky concepts.

   A generic class is a template analogon for a classical abstract class. PTL defines a generic class template for each
   family of related classes, for instance, GenericVector for everything looking like a vector.  As in the abstract
   class approach, all concrete classes have to be derived from the generic class. And exactly as an abstract class, a
   generic class should not be instantiated "naked"; to enforce this, its constructor and destructor are always defined
   @c protected.

   To eliminate the need for virtual functions, an instance of the generic class has to know statically (i.e., already
   during the compilation,) what concrete class it belongs to. This is achieved by parameterizing the generic class (you
   remember, it's a template!) with the concrete class derived from it.  Staying by the vectors, an example would look
   like this:

   <pre>
      template &lt;typename _Vector&gt;
      class GenericVector {
         typedef _Vector top_type;
         ...
      };

      template &lt;typename ElementType&gt;
      class Vector : public GenericVector&lt; Vector&lt;ElementType&gt; &gt; { ... };
   </pre>

   It this setting it is very easy for the generic class instance to achieve the full concrete object: it's just
   a @c{ static_cast<top_type*>(this) }.  Such an inverted cast is explicitly allowed by the ANSI/ISO
   Standard C++.  To keep the code more readable, each generic class defines a @c top() method encapsulating
   exactly this cast (in fact, two methods, with and without @c const.)

   The same cast can be applied by every external function working with class families as well:

   <pre>
      template &lt;typename _Vector&gt;
      void f(const GenericVector&lt;_Vector&gt;& v) {
         int s=v.top().size();
         ...
      }
   </pre>

   In many cases, however, the cast is not necessary, since many methods are defined already in the generic class;
   please consult the documentation of the corresponding class family.

   The generic classes can also carry additional attributes, allowing for more fine-granular distinction between
   overloaded template functions. All these attributes are declared with default settings extracted
   from the concrete class, therefore the application functions don't always need to refer to them explicitly.
   The real PTL definition of the @c GenericVector class has the vector element type as the second parameter:

   <pre>
      template &lt;typename _Vector, typename ElementType=typename _Vector::element_type&gt;
      class GenericVector {
         typedef ElementType element_type;
         ...
      };
   </pre>

   Now you can easily distinct between two versions of an algorithm, one requiring two vectors of identical element type,
   and another handling the heterogeneous case:

   <pre>
      // homogeneous case
      template &lt;typename _Vector1, typename _Vector2, typename ElementType&gt;
      void f(const GenericVector&lt;_Vector1, ElementType&gt;& v1, const GenericVector&lt;_Vector2, ElementType&gt;& v2);

      // heterogeneous case
      template &lt;typename _Vector1, typename _Vector2&gt;
      void f(const GenericVector&lt;_Vector1&gt;& v1, const GenericVector&lt;_Vector2&gt;& v2);
   </pre>

   @section generic_caveats_sec Caveats

   However smart this technique might be, it has its not negligible caveats. First of all, it is the problem of so called
   @em{code bloat}: each function designed to work with a generic family has to be a template in its turn.
   Often this "templatization" spreads itself over the entire program module, stopping only at the @c main()
   function. This leads to considerable code size growth and to an immense time and memory consumption by the compiler.

   Another problem arises when you make a mistake in your template code: the compiler diagnostics are very
   heavy to understand, most of them referring to the code you had not written yourself, namely the guts of the
   template library. Mistakes also often stay undiscovered for long periods of time, until the method containing it
   is really used (due to the @em{lazy template instantiation} deployed by the most of the modern C++ compilers.)
   There are some recently invented approaches that can alleviated this, in particular the @em{concept checks}
   originating from the Boost library. They will be probably built in PTL, as soon as their deployment in STL
   will seem stable enough.

 */

/** @page manipulations Container Manipulations

    All constructions described on this page are similar in that they take one or more objects and produce a new object
    which behaves like a container, too.  However, in all these cases it is not a container in the strict sense: it does
    not own any data.  We will call them @em pseudo-container throughout this documentation.

    The pseudo-containers can be divided in three categories according to their functionality:

    - An @em alias pseudo-container forwards all element access operation to the original data container(s).  It can
    hide some elements away or let them appear in other order, in any case its elements are at the same times elements
    of the original container.  This implies that assigning new values to the elements of a mutable alias container in
    fact changes the elements in the original container.  }

    - A @em lazy pseudo-container computes its elements on the fly, just at the moment they are accessed to.  Thus the
    elements are temporary objects returned by container access methods (@c front(), @c back(), or @c operator[]) or
    reside in the iterator.  Traversing a lazy container object multiple times object incurs repeating computations,
    which should be kept in mind when sticking it into a multi-pass algorithm template.

    When you write an algorithm template, you can make it safe from multiple evaluation of lazy objects using the
    following construction:

    <pre>
       typename <Object>::type X=prevent_lazy<Object>::get(x);
    </pre>

    where @a x is an input parameter and @a Object its type. If @a Object is really lazy, it performs the evaluation
    exactly once and stores the results in an appropriate persistent object; if not, it does nothing.

    - A @em masquerade pseudo-container is just another view on the original object. Unlike the first two kinds, masquerade
    objects are never created: the original object address is reinterpreted instead; they even can't be created, since
    their constructors and destructors are purposely declared @c protected and left undefined.
    
    To make the classification complete, let's call the standard containers,
    whose lifetime is the same as of their elements, @em persistent.  This
    notion is traditionally used in the context of data base query languages,
    where it describes objects that can outlive the programs creating them.
    This is also applicable to objects in Polymake Template Library, since
    they can be stored in and retrieved from the a data file via the client
    communication interface without any loss in structure or contents.
    
    @section refcopy_sec Reference or copy?

    For instantiable pseudo-containers (alias and lazy) it is important to
    know how to find the original data containers.  Generally, the latter have
    a lifetime much longer than a pseudo-container object, which in the most
    cases exists during exactly one expression. Thus an internal reference to
    the original object would be sufficient.

    On the other hand, the pseudo-containers are easy to combine and nest (it
    was, after all, the main design aim to made them interchangeable building
    blocks!)  For example, one can first select a subset of elements and then
    apply an operation to it. In this case, the first operand of the outer
    pseudo-container will be the inner pseudo-container, which in its turn is
    a temporary object.  It doesn't create a problem until the entire
    construction is copied outside the scope the components are confined to.

    The best example is a @c return statement: if a function has to return the
    outer pseudo-container object, it @em{may not} contain a reference to the
    inner object, since it was created in the function's scope and will be
    destroyed after the @c return completion.  Therefore the inner object @em
    must be copied into the outer object.

    All pseudo-container objects in the Polymake Template Library can be
    configured for both scenarios. The template parameters describing the data
    sources are @em{optional references}: they can be specified as references
    as well as reference-free data types.  Note that the @em{convenience
    functions} always create pseudo-containers with internal references, as a
    more efficient and more often occurring variant; the referenceless variant
    should be used only when it's really necessary, like in the @c return
    context explained above.
    
*/

/** @page GMP Number Types, Borrowed from GMP

    Most calculations in %polymake are made exactly, using rational numbers.
    The best implementation known to us which provides arbitrary precision and
    high-tuned performance on various computer platforms, is
    http://www.swox.com/gmp/ .  On the other hand, we did like very much the
    convenient interface of the Rational and Integer classes from the (in the
    meanwhile obsolete and no more supported) libg++, which allowed to use the
    rational values in expressions in the most natural way, as if they were
    built-in numeric types.  So we have decided to union the advantages of
    both and have written thin wrapper classes mimicking the old Rational and
    Integer classes.  They perform almost no calculations on their own, but
    delegate the whole hard job to GMP functions.

    In the meanwhile, GMP has got its own C++ wrapper classes. They are
    implemented differently from our classes: all arithmetic %operators are
    "lazy", returning expression templates instead of ready results.  While
    this technique has proven to improve the performance in longer chained
    expressions, and is massively used in %polymake vector and matrix classes
    too, we consider it a bit dangerous for the basic numerical type.

    In an innocent expression like @c (a+b)*M, where @c a and @c b are
    rational scalars and @c M is a rational matrix, the sum @c a+b would be
    repeatedly calculated for each element of the resulting matrix.  Since
    such mixed expressions are prevailing in %polymake code, we have decided to
    stay with our own wrappers, relying on the so called @em{return value
    optimization} of the compiler. Future changes in GMP may let us revise
    this decision.

    GMP numbers can be constructed from the most built-in types.  They can be
    converted to each other and to the most built-in types.
*/

} // namespace pm
