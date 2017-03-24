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

#ifndef GROUP_TYPE_H_
#define GROUP_TYPE_H_

#include <cstring>

namespace permlib {

/// abstract base class for permutation group types
class GroupType {
public:
	/// types for which an implementation of GroupType exists
	enum Type {
		None,
		Trivial,
		Named,
		Anonymous,
		WreathSymmetric,
		DirectProduct
	};
	
	/// writes a human readable identifier to the given output stream
	void writeToStream(std::ostream& o) const {
		if (!m_naturalAction)
			o << "ISO( ";
		this->writeTypeToStream(o);
		if (!m_naturalAction)
			o << " , " << m_realDegree << " )";
	}
	
	/// the degree of the group as permutation group
	unsigned int realDegree() const { return m_realDegree; }
	/// returns true iff action is natural
	/**
	 * Natural action means, for instance, that a S_k acts on k elements.
	 */
	bool isNaturalAction() const { return m_naturalAction; }
	/// the type of this the group
	Type type() const { return m_type; }
	
	/// checks if two group types represent the same permutation group
	bool equals(const GroupType* type_) const {
		if (this->m_type != type_->m_type)
			return false;
		if (this->m_realDegree != type_->m_realDegree)
			return false;
		if (this->m_naturalAction != type_->m_naturalAction)
			return false;
		return this->equalsType(type_);
	}
	
	/// stores the information that this group acts non-naturally on realDegree many elements
	void setNonNaturalAction(unsigned int realDegree_) {
		m_realDegree = realDegree_;
		m_naturalAction = false;
	}
	
	/// destructor
	virtual ~GroupType() { }
protected:
	/// group type
	Type m_type;
	/// degree of the permutation group
	unsigned int m_realDegree;
	/// stores whether action is natural
	bool m_naturalAction;
	
	/// protected constructor
	GroupType(Type type_, unsigned int realDegree_, bool naturalAction) 
		: m_type(type_), m_realDegree(realDegree_), m_naturalAction(naturalAction) { }
	
	/// checks if two group types represent the same permutation group
	/**
	 * This method may expect that the given group type is of the same type as itself.
	 * Thus it may cast type to its own type.
	 */
	virtual bool equalsType(const GroupType* type_) const { return false; }
	/// writes type specific string to output stream
	virtual void writeTypeToStream(std::ostream& o) const = 0;
};


/// Group type for a trivial permutation group
class TrivialGroupType : public GroupType {
public:
	/**
	 * @param realDegree the number of elements the permutation group acts on
	 */
	TrivialGroupType(unsigned int realDegree_) : GroupType(Trivial, realDegree_, true) { }
	
	virtual void writeTypeToStream(std::ostream& o) const {
		o << "Trivial(" << m_realDegree << ")";
	}
protected:
	virtual bool equalsType(const GroupType* type_) const { return true; }
};


/// Group type for a permutation group whose type could not be determined
template<class IntegerType = boost::uint64_t>
class AnonymousGroupType : public GroupType {
public:
	/**
	 * @param realDegree the number of elements the permutation group acts on
	 * @param order the order of the permutation group, if known
	 */
	AnonymousGroupType(unsigned int realDegree_, IntegerType order_ = 0)
		: GroupType(Anonymous, realDegree_, true), m_order(order_) { }
	
	virtual void writeTypeToStream(std::ostream& o) const {
		if (m_order)
			o << "anonymous(" << m_realDegree << ", " << m_order << ")";
		else
			o << "anonymous(" << m_realDegree << ")";
	}
protected:
	IntegerType m_order;
	
	virtual bool equalsType(const GroupType* type_) const { 
		// we can never know if two anonymous groups are equal without digging deeper
		return false;
	}
};


/// abstract base class for named groups (such as cyclic and symmetric groups)
class NamedGroupType : public GroupType {
public:
	virtual void writeTypeToStream(std::ostream& o) const {
		o << m_name << "_" << m_typeDegree;
	}
	
	/// the name of the group
	const char* name() const { return m_name; }
	/// the degree of the named group to which the real action is isomorphic to
	unsigned int typeDegree() const { return m_typeDegree; }
protected:
	const char* m_name;
	unsigned int m_typeDegree;
	
	/**
	 * @param name short name for the group
	 * @param typeDegree degree of named group to which the real action is isomorphic to
	 * @param realDegree the number of elements the permutation group acts on
	 */
	NamedGroupType(const char* name_, unsigned int typeDegree_, unsigned int realDegree_)
		: GroupType(Named, realDegree_, typeDegree_ == realDegree_), m_name(name_), m_typeDegree(typeDegree_) { }
	
	virtual bool equalsType(const GroupType* type_) const { 
		const NamedGroupType* namedType = dynamic_cast<const NamedGroupType*>(type_);
		return namedType->m_typeDegree == this->m_typeDegree && std::strcmp(namedType->m_name, this->m_name) == 0;
	}
};


/// Group type for symmetric groups
class SymmetricGroupType : public NamedGroupType {
public:
	/**
	 * @param typeDegree degree of named group to which the real action is isomorphic to
	 * @param realDegree the number of elements the permutation group acts on
	 */
	SymmetricGroupType(unsigned int typeDegree_, unsigned int realDegree_)
		: NamedGroupType("S", typeDegree_, realDegree_) { }
};


/// Group type for alternating groups
class AlternatingGroupType : public NamedGroupType {
public:
	/**
	 * @param typeDegree degree of named group to which the real action is isomorphic to
	 * @param realDegree the number of elements the permutation group acts on
	 */
	AlternatingGroupType(unsigned int typeDegree_, unsigned int realDegree_)
		: NamedGroupType("A", typeDegree_, realDegree_) { }
};


/// Group type for cyclic groups
class CyclicGroupType : public NamedGroupType {
public:
	/**
	 * @param typeDegree degree of named group to which the real action is isomorphic to
	 * @param realDegree the number of elements the permutation group acts on
	 */
	CyclicGroupType(unsigned int typeDegree_, unsigned int realDegree_)
		: NamedGroupType("C", typeDegree_, realDegree_) { }
};


/// Group type for a wreath product of symmetric groups
/**
 * S_k wr S_l    where k = deg G and l = deg H
 */
class WreathSymmetricGroupType : public GroupType {
public:
	WreathSymmetricGroupType(unsigned int degreeG_, unsigned int degreeH_, unsigned int realDegree_)
		: GroupType(WreathSymmetric, realDegree_, realDegree_ == degreeG_ * degreeH_), m_degreeG(degreeG_), m_degreeH(degreeH_) { }
	
	virtual void writeTypeToStream(std::ostream& o) const {
		o << "S_" << m_degreeG << " wr S_" << m_degreeH;
	}
	
	unsigned int degreeG() const { return m_degreeG; }
	unsigned int degreeH() const { return m_degreeH; }
protected:
	unsigned int m_degreeG;
	unsigned int m_degreeH;
	
	virtual bool equalsType(const GroupType* type_) const { 
		const WreathSymmetricGroupType* wreathType = dynamic_cast<const WreathSymmetricGroupType*>(type_);
		return wreathType->m_degreeG == m_degreeG && wreathType->m_degreeH == m_degreeH;
	}
};

/// Group type for a direct product of two groups
class DirectProductGroupType : public GroupType {
public:
	/**
	 * @param type1 type of first factor
	 * @param type2 type of second factor
	 * @param realDegree the number of elements the permutation group acts on
	 */
	DirectProductGroupType(const GroupType* type1, const GroupType* type2, unsigned int realDegree_)
		: GroupType(DirectProduct, realDegree_, realDegree_ == type1->realDegree() + type2->realDegree()), m_components(2) 
	{ 
		m_components[0] = GroupTypePtr(type1);
		m_components[1] = GroupTypePtr(type2);
	}
	
	virtual void writeTypeToStream(std::ostream& o) const {
		for (unsigned int i = 0; i < m_components.size(); ++i) {
			if (i > 0)
				o << " x ";
			m_components[i]->writeToStream(o);
		}
	}
	
protected:
	typedef boost::shared_ptr<const GroupType> GroupTypePtr;
	std::vector<GroupTypePtr> m_components;
	
	virtual bool equalsType(const GroupType* type_) const { 
		const DirectProductGroupType* directType = dynamic_cast<const DirectProductGroupType*>(type_);
		if (m_components.size() != directType->m_components.size())
			return false;
		std::vector<GroupTypePtr>::const_iterator itMe = m_components.begin();
		std::vector<GroupTypePtr>::const_iterator itOther = directType->m_components.begin();
		while (itMe != m_components.end()) {
			if ( ! (*itMe)->equals((*itOther).get()) )
				return false;
			++itMe;
			++itOther;
		}
		return true;
	}
};

}

#endif
