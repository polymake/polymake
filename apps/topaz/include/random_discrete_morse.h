#ifndef POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H
#define POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H

namespace polymake { namespace topaz {

Map< Array<int>,int > random_discrete_morse(const graph::HasseDiagram orig_HD, UniformlyRandom<long> seed, const int strategy, const bool verbose, const int rounds, const Array<int> try_until_reached,  const Array<int> try_until_exception, std::string save_to_filename );

}}

#endif // POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H

