// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2011 Thomas Rehn <thomas@carmen76.de>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------

#ifndef PRIME_HELPER_H_
#define PRIME_HELPER_H_

#include <algorithm>
#include <boost/assert.hpp>

namespace permlib {

/// helper class for primality checks
class PrimeHelper {
public:
	/// The number up to which this simple primality check is always correct.
	static const unsigned int largestNumberForPrimalityCheck;

	/// checks if a given number is prime
	/**
	* @param x number to be checked
	* @param checkBounds if true, an assertion failure is triggered if x is too large (larger than permlib::largestNumberForPrimalityCheck)
	* @return false iff number is composite or cannot be checked for primality
	*/
	static bool isPrimeNumber(unsigned int x, bool checkBounds) {
		if (checkBounds && x > largestNumberForPrimalityCheck) {
			// number too big for our simple check
			BOOST_ASSERT( false );
			return false;
		}
		
		if (x > largestNumberForPrimalityCheck) {
			// the number is to big for our simple check
			return false;
		} else if (x > largestPrime) {
			for (unsigned int i = 0; i < numberOfPrimes; ++i) {
				if ((x % primes[i]) == 0)
					return false;
			}
			return true;
		}
		return std::binary_search(primes, primes+numberOfPrimes, x);
	}
	
	/// iterator pointing to the first prime in list
	static const unsigned int* firstPrime() { return primes; }
	/// iterator pointing after the last prime in list
	static const unsigned int* lastPrime() { return primes + numberOfPrimes; }
private:
	static const unsigned int numberOfPrimes;
	// This list is probably incomplete ;)
	static const unsigned int primes[];
	static const unsigned int largestPrime;
};

const unsigned int PrimeHelper::numberOfPrimes = 170;
// Probably this list is incomplete ;)
const unsigned int PrimeHelper::primes[numberOfPrimes] = { 
	2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,
	127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281, 
	283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,
	467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,
	661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,
	877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997,1009,1013
};
const unsigned int PrimeHelper::largestPrime = primes[numberOfPrimes-1];
const unsigned int PrimeHelper::largestNumberForPrimalityCheck = largestPrime * largestPrime;

}

#endif
