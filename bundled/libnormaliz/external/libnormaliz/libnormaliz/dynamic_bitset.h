/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef LIBNORMALIZ_DYNAMIC_BITSET_H
#define LIBNORMALIZ_DYNAMIC_BITSET_H

// uncomment to disable assert()
// #define NDEBUG
#include <cassert>

#include <climits>  // for CHAR_BIT
#include <iostream>
#include <vector>

namespace libnormaliz {

// An implementation of a subset of boost::dynamic_bitset.
//
// This provides only a subset of boost::dynamic_bitset, based on
// those features Normaliz actually uses. This has the advantage
// that the resulting code is much shorter and simpler. Also, unused
// code often has bugs, while code that was never written is bug free.
//
// Among the features that are left out are:
// - custom allocators -- always use the default allocator
// - custom block/limb types -- we hardcode unsigned long long
// - initialization of bitsets with non-zero data in constructors, resize()
// - iterators
// - shift operators
// - appending, pop_back, push_back
// - conversion from/to strings
// - is_proper_subset_of
// - istream operator>>
//
// Of course any of these could be added in the future, should the need arise
class dynamic_bitset {
   public:
    typedef unsigned long long limb_t;

    static const size_t bits_per_limb = CHAR_BIT * sizeof(limb_t);
    static const size_t npos = static_cast<size_t>(-1);

    // proxy object used for implementing write access to the bitset
    // via operator[].
    class reference {
        friend class dynamic_bitset;
        void operator&();  // left undefined

        reference(dynamic_bitset& x, size_t pos) : _limb(x._limbs[limb_index(pos)]), _mask(bit_mask(pos)) {
            assert(pos < x.size());
        }

       public:
        operator bool() const {
            return (_limb & _mask) != 0;
        }
        bool operator~() const {
            return (_limb & _mask) == 0;
        }

        // b[i] = x
        reference& operator=(bool x) {
            set_to(x);
            return *this;
        }

        // b[i] = b[j]
        reference& operator=(const reference& x) {
            set_to(x);
            return *this;
        }

        reference& flip() {
            _limb ^= _mask;
            return *this;
        }

        reference& operator|=(bool x) {
            if (x)
                _limb |= _mask;
            return *this;
        }
        reference& operator&=(bool x) {
            if (!x)
                _limb &= ~_mask;
            return *this;
        }
        reference& operator^=(bool x) {
            if (x)
                _limb ^= _mask;
            return *this;
        }
        reference& operator-=(bool x) {
            if (x)
                _limb &= ~_mask;
            return *this;
        }

       private:
        limb_t& _limb;
        const limb_t _mask;

        void set_to(bool x) {
            if (x)
                _limb |= _mask;
            else
                _limb &= ~_mask;
        }
    };

    typedef bool const_reference;

    // constructors (copy & move constructors are provided by the compiler)
    dynamic_bitset() : _limbs(), _total_bits(0) {
    }
    explicit dynamic_bitset(size_t nbits) : _limbs(limbs_required(nbits)), _total_bits(nbits) {
    }

    //
    // operations involving the size of the bitset
    //
    void resize(size_t nbits) {
        if (nbits == _total_bits)
            return;
        _limbs.resize(limbs_required(nbits), 0);
        _total_bits = nbits;
        cleanup_last_limb();
    }
    void clear() {
        _limbs.clear();
        _total_bits = 0;
    }
    size_t size() const {
        return _total_bits;
    }
    bool empty() const {
        return size() == 0;
    }

    //
    // operations for accessing or modifying individual bits
    //

    // set the bit at the given position to the given value
    dynamic_bitset& set(size_t pos, bool val) {
        return val ? set(pos) : reset(pos);
    }

    // set the bit at the given position to 1
    dynamic_bitset& set(size_t pos) {
        assert(pos < size());
        _limbs[limb_index(pos)] |= bit_mask(pos);
        return *this;
    }

    // set the bit at the given position to 0
    dynamic_bitset& reset(size_t pos) {
        assert(pos < size());
        _limbs[limb_index(pos)] &= ~bit_mask(pos);
        return *this;
    }

    // flip the bit at the given position
    dynamic_bitset& flip(size_t pos) {
        assert(pos < size());
        _limbs[limb_index(pos)] ^= bit_mask(pos);
        return *this;
    }

    // return true if the bit at the given position is 1, otherwise false
    bool test(size_t pos) const {
        assert(pos < size());
        return (_limbs[limb_index(pos)] & bit_mask(pos)) != 0;
    }

    // subscript operators
    const_reference operator[](size_t pos) const {
        return test(pos);
    }
    reference operator[](size_t pos) {
        assert(pos < size());
        return reference(*this, pos);
    }

    //
    // operations for accessing and modifying the whole bitset
    //

    // set all bits to 1
    dynamic_bitset& set() {
        for (limb_t& limb : _limbs) {
            limb = all_ones_limb;
        }
        cleanup_last_limb();  // cleanup in the last limb
        return *this;
    }

    // set all bits to 0
    dynamic_bitset& reset() {
        for (limb_t& limb : _limbs) {
            limb = 0;
        }
        return *this;
    }

    // flip all bits
    dynamic_bitset& flip() {
        for (limb_t& limb : _limbs) {
            limb ^= all_ones_limb;
        }
        cleanup_last_limb();  // cleanup in the last limb
        return *this;
    }

    // return true if any bit is set to 1, otherwise false
    bool any() const {
        for (const limb_t& limb : _limbs) {
            if (limb != 0)
                return true;
        }
        return false;
    }

    // return true if no bit is set to 1, otherwise false
    bool none() const {
        return !any();
    }

    // return the number of bits which are set to 1
    size_t count() const {
        size_t sum = 0;
        for (const limb_t& limb : _limbs) {
            sum += popcount(limb);
        }
        return sum;
    }

    //
    // in-place operators
    //
    dynamic_bitset& operator&=(const dynamic_bitset& rhs) {
        assert(size() == rhs.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            _limbs[i] &= rhs._limbs[i];
        }
        return *this;
    }
    dynamic_bitset& operator|=(const dynamic_bitset& rhs) {
        assert(size() == rhs.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            _limbs[i] |= rhs._limbs[i];
        }
        return *this;
    }
    dynamic_bitset& operator^=(const dynamic_bitset& rhs) {
        assert(size() == rhs.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            _limbs[i] ^= rhs._limbs[i];
        }
        return *this;
    }
    dynamic_bitset& operator-=(const dynamic_bitset& rhs) {
        assert(size() == rhs.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            _limbs[i] &= ~rhs._limbs[i];
        }
        return *this;
    }

    //
    // non-modifying unary and binary operators
    //

    dynamic_bitset operator~() const {
        dynamic_bitset result(*this);
        result.flip();
        return result;
    }
    dynamic_bitset operator&(const dynamic_bitset& rhs) const {
        dynamic_bitset result(*this);
        return result &= rhs;
    }
    dynamic_bitset operator|(const dynamic_bitset& rhs) const {
        dynamic_bitset result(*this);
        return result |= rhs;
    }
    dynamic_bitset operator^(const dynamic_bitset& rhs) const {
        dynamic_bitset result(*this);
        return result ^= rhs;
    }
    dynamic_bitset operator-(const dynamic_bitset& rhs) const {
        dynamic_bitset result(*this);
        return result -= rhs;
    }

    //
    // comparison of two bitsets
    //

    bool operator==(const dynamic_bitset& x) const {
        return _total_bits == x._total_bits && _limbs == x._limbs;
    }
    bool operator!=(const dynamic_bitset& x) const {
        return !(*this == x);
    }
    bool operator<(const dynamic_bitset& x) const {  // for use in std::set, std::map
        if (_total_bits == x._total_bits) {
            for (size_t i = 1; i <= _limbs.size(); ++i) {
                if (_limbs[_limbs.size() - i] != x._limbs[_limbs.size() - i])
                    return (_limbs[_limbs.size() - i] < x._limbs[_limbs.size() - i]);
            }
            return false;
        }
        return _total_bits < x._total_bits;
    }

    // subsets
    bool is_subset_of(const dynamic_bitset& x) const {
        assert(size() == x.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            if ((_limbs[i] & ~x._limbs[i]) != 0)
                return false;
        }
        return true;
    }
    bool intersects(const dynamic_bitset& x) const {
        assert(size() == x.size());
        for (size_t i = 0; i < _limbs.size(); ++i) {
            if ((_limbs[i] & x._limbs[i]) != 0)
                return true;
        }
        return false;
    }

    //
    // find functions
    //

    // find the index of the first set bit, or npos if no bit is set
    size_t find_first() const {
        for (size_t i = 0; i < _limbs.size(); ++i) {
            if (_limbs[i] != 0)
                return i * bits_per_limb + limb_find_first(_limbs[i]);
        }
        return npos;
    }
    size_t find_next(size_t prev) const {
        if (prev == npos)
            return npos;
        // first bit to consider is prev + 1
        ++prev;
        if (prev >= size())
            return npos;

        // first check whether the limb in which prev is contained
        // contains any set bits after prev
        size_t i = limb_index(prev);
        limb_t limb = _limbs[i] >> (prev % bits_per_limb);
        if (limb != 0)
            return prev + limb_find_first(limb);

        // for the remaining limbs, proceed as in find_first()
        ++i;
        for (; i < _limbs.size(); ++i) {
            if (_limbs[i] != 0)
                return i * bits_per_limb + limb_find_first(_limbs[i]);
        }
        return npos;
    }

   private:
    // the actual bits are stored in a vector<>, which means we don't have
    // to worry about memory management
    std::vector<limb_t> _limbs;

    // the size of this bitset, in bits; note that this may be small than
    // _limbs.size() * bits_per_limb
    size_t _total_bits;

    static const limb_t all_zero_limb = limb_t(0);
    static const limb_t all_ones_limb = limb_t(~all_zero_limb);

    static size_t limbs_required(size_t nbits) {
        return (nbits + bits_per_limb - 1) / bits_per_limb;
    }

    static size_t limb_index(size_t pos) {
        return pos / bits_per_limb;
    }

    static limb_t bit_mask(size_t pos) {
        return limb_t(limb_t(1) << (pos % bits_per_limb));
    }

    // zero out unused bits in last limb
    void cleanup_last_limb() {
        size_t shift = _total_bits % bits_per_limb;
        if (shift > 0) {
            _limbs[_limbs.size() - 1] &= ~(all_ones_limb << shift);
        }
    }

    // Find the first set bit in the given non-zero limb, were we start at the
    // least significant bit (LSB) and move to the most significant (MSB).
    // The behavior for zero limbs is undefined.
    //
    // This is an internal helper function, used by find_first and find_next.
    size_t limb_find_first(limb_t limb) const {
        assert(limb != 0);

// ideally we'd like to use __builtin_ctzll; but this may not be
// supported by the compiler; so we use autoconf to check for it; but
// then client code using this header may for whatever reason not have
// the autoconf macros, so this code is written in such a way that we
// always can safely fall back to the generic code.
// TODO: if we want to support a limb_t other than 'unsigned long long',
// then this code will need to be adjusted.
#if HAVE___BUILTIN_CTZLL
        return __builtin_ctzll(limb);
#endif

        // we rely on the compiler to optimize the following "runtime checks"
        // away in the generated code
        if (bits_per_limb == 64) {
            // source for the following table and code:
            // https://www.chessprogramming.org/BitScan#De_Bruijn_Multiplication
            static const char DeBrujinPositions[64] = {
                0,  1,  48, 2,  57, 49, 28, 3,  61, 58, 50, 42, 38, 29, 17, 4,  62, 55, 59, 36, 53, 51,
                43, 22, 45, 39, 33, 30, 24, 18, 12, 5,  63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21,
                44, 32, 23, 11, 46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9,  13, 8,  7,  6,
            };
            static const limb_t magic = 0x03f79d71b4cb0a89;
            return DeBrujinPositions[((limb & -limb) * magic) >> 58];
        }
        else {
            assert(bits_per_limb == 32);
            // source for the following table and code:
            // http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
            static const char DeBrujinPositions[32] = {
                0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
                31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9,
            };
            static const limb_t magic = 0x077cb531;
            return DeBrujinPositions[((limb & -limb) * magic) >> 27];
        }
    }

    // Count the number of set bits in the given limb.
    //
    // This is an internal helper function, used by count.
    size_t popcount(limb_t limb) const {
// ideally we'd like to use __builtin_popcountll; but this may not be
// supported by the compiler; so we use autoconf to check for it; but
// then client code using this header may for whatever reason not have
// the autoconf macros, so this code is written in such a way that we
// always can safely fall back to the generic code.
// TODO: if we want to support a limb_t other than 'unsigned long long',
// then this code will need to be adjusted.
#if HAVE___BUILTIN_POPCOUNTLL
        return __builtin_popcountll(limb);
#endif

        // we rely on the compiler to optimize the following "runtime checks"
        // away in the generated code
        if (bits_per_limb == 64) {
            limb = (limb & 0x5555555555555555L) + ((limb >> 1) & 0x5555555555555555L);
            limb = (limb & 0x3333333333333333L) + ((limb >> 2) & 0x3333333333333333L);
            limb = (limb + (limb >> 4)) & 0x0f0f0f0f0f0f0f0fL;
            limb = (limb + (limb >> 8));
            limb = (limb + (limb >> 16));
            limb = (limb + (limb >> 32)) & 0x00000000000000ffL;
            return limb;
        }
        else {
            assert(bits_per_limb == 32);
            limb = (limb & 0x55555555) + ((limb >> 1) & 0x55555555);
            limb = (limb & 0x33333333) + ((limb >> 2) & 0x33333333);
            limb = (limb + (limb >> 4)) & 0x0f0f0f0f;
            limb = (limb + (limb >> 8));
            limb = (limb + (limb >> 16)) & 0x000000ff;
            return limb;
        }
    }
};

// implement output to an ostream; this is mostly for debugging,
// hence no effort was made to optimize this
static inline std::ostream& operator<<(std::ostream& os, const dynamic_bitset& bitset) {
    size_t pos = bitset.size();
    while (pos--) {
        os << bitset[pos];
    }
    return os;
}

}  // namespace libnormaliz

#endif /* LIBNORMALIZ_DYNAMIC_BITSET_H */
