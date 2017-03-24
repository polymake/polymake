/* Copyright (c) 1997-2016
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

#include "polymake/perl/Ext.h"

/******************************************************************************************************/
/*  references as hash keys  */

static HV* my_pkg;
static AV* allowed_pkgs;

static Perl_check_t def_ck_PUSH;
static Perl_ppaddr_t def_pp_CONST, def_pp_HELEM, def_pp_HSLICE, def_pp_EXISTS, def_pp_DELETE, def_pp_EACH, def_pp_KEYS,
                     def_pp_RV2HV, def_pp_PADHV, def_pp_ANONHASH;

#if PerlVersion >= 5180
static Perl_ppaddr_t def_pp_PADRANGE;
#endif
#if PerlVersion >= 5220
static Perl_check_t def_ck_HELEM, def_ck_EXISTS, def_ck_DELETE;
#endif

typedef struct tmp_keysv {
   HEK hek;
   char key_tail[sizeof(SV*)];  // the last byte is the terminating 0, the first byte of the key resides in hek.
   XPVUV xpv;
   SV sv;
} tmp_keysv;

typedef union key_or_ptr {
   SV* ptr;
   unsigned long keyl;
   char keyp[sizeof(SV*)];
} key_or_ptr;

#if PerlVersion < 5180
# define PmFlagsForHashKey (SVf_FAKE | SVf_READONLY)
#else
# define PmFlagsForHashKey SVf_IsCOW
#endif

static
SV* ref2key(SV* keysv, tmp_keysv* tmp_key)
{
   HEK* hek=&tmp_key->hek;
   key_or_ptr obj;
   obj.ptr=SvRV(keysv);
#if PerlVersion < 5180
   if (SvAMAGIC(keysv)) obj.keyl |= 1;
#endif
   Copy(obj.keyp, HEK_KEY(hek), sizeof(SV*), char);
   HEK_KEY(hek)[sizeof(SV*)] = 0;
   HEK_LEN(hek)=sizeof(SV*);
   HEK_HASH(hek)=obj.keyl >> 4;          // hash value
   HEK_FLAGS(hek)=HVhek_UNSHARED;
   tmp_key->sv.sv_any=&tmp_key->xpv;
   tmp_key->sv.sv_refcnt=1;
   tmp_key->sv.sv_flags= SVt_PVIV | SVf_IVisUV | SVf_POK | SVp_POK | PmFlagsForHashKey;
   SvPV_set(&tmp_key->sv, HEK_KEY(hek));
   SvCUR_set(&tmp_key->sv, sizeof(SV*));
   SvLEN_set(&tmp_key->sv, 0);
   return &tmp_key->sv;
}

#define TmpKeyHash(tk) HEK_HASH(&tk.hek)

#define MarkAsRefHash(hv)     SvSTASH(hv)=my_pkg
#define MarkAsNormalHash(hv)  SvSTASH(hv)=Nullhv

#define ErrNoRef(key) STMT_START {                                      \
   if (SvOK(key)) {                                                     \
      STRLEN kl; const char *k=SvPV(key,kl);                            \
      DIE(aTHX_ "Hash key '%*.s' where reference expected", (int)kl, k);\
   } else {                                                             \
      DIE(aTHX_ "Hash key UNDEF where reference expected");             \
   }                                                                    \
} STMT_END

static const char err_ref[]="Reference as a key in a normal hash";

static inline
int ref_key_allowed(HV* class)
{
   if (AvFILLp(allowed_pkgs)>=0) {
      SV **ap, **end;
      for (ap=AvARRAY(allowed_pkgs), end=ap+AvFILLp(allowed_pkgs); ap<=end; ++ap)
         if (SvRV(*ap)==(SV*)class) return TRUE;
   }
   return FALSE;
}

#define RefKeyAllowed(hv, class)                                         \
   ( class==my_pkg ||                                                   \
    (class==NULL                                                        \
     ? !HvFILL(hv) && !SvRMAGICAL(hv) && (MarkAsRefHash(hv), TRUE)      \
     : ref_key_allowed(class)))

#define HashCPPbound(hv) \
   (class!=NULL && SvMAGICAL(hv) && (mg=pm_perl_get_cpp_magic((SV*)(hv))) != NULL)

typedef struct local_hash_ref_elem {
   HV *hv;
   SV *keyref;
} local_hash_ref_elem;

static inline
void* store_hash_ref_elem(pTHX_ HV* hv, SV* keyref)
{
   local_hash_ref_elem* le;
   New(0, le, 1, local_hash_ref_elem);
   le->hv=(HV*)SvREFCNT_inc_simple_NN(hv);
   le->keyref=SvREFCNT_inc_simple_NN(keyref);
   return le;
}

static
void delete_hash_elem(pTHX_ void* p)
{
   local_hash_ref_elem* le=(local_hash_ref_elem*)p;
   tmp_keysv tmp_key;
   HV* hv=le->hv;
   SV* keyref=le->keyref;
   SV* keysv=ref2key(keyref, &tmp_key);
   (void)hv_delete_ent(hv, keysv, G_DISCARD, TmpKeyHash(tmp_key));
   SvREFCNT_dec(hv);
   SvREFCNT_dec(keyref);
   Safefree(p);
}

HE* pm_perl_refhash_fetch_ent(pTHX_ HV* hv, SV* keysv, I32 lval)
{
   tmp_keysv tmp_key;
   U32 hash;
   HV* class=SvSTASH(hv);
   assert(SvROK(keysv));
   if (!RefKeyAllowed(hv, class))
      Perl_croak(aTHX_ err_ref);
   keysv=ref2key(keysv, &tmp_key);
   hash=TmpKeyHash(tmp_key);
   return hv_fetch_ent(hv, keysv, lval, hash);
}

static
OP* intercept_pp_helem(pTHX)
{
   dSP;
   SV* keysv=TOPs;
   HV* hv=(HV*)TOPm1s;
   HV* class=SvSTASH(hv);
   tmp_keysv tmp_key;
   MAGIC* mg;
   if (HashCPPbound(hv))
      return pm_perl_cpp_helem(aTHX_ hv, mg);
   if (SvROK(keysv)) {
      if (!RefKeyAllowed(hv, class))
         DIE(aTHX_ err_ref);
      if ((PL_op->op_private & (OPpLVAL_INTRO | OPpLVAL_DEFER)) == OPpLVAL_INTRO &&
          (PL_op->op_flags & OPf_MOD || LVRET)) {
         HE* he;
         SV *elem_sv, *keyref=keysv;
         I32 existed;
         U32 hash;
         keysv=ref2key(keysv, &tmp_key);
         hash=TmpKeyHash(tmp_key);
         existed=hv_exists_ent(hv, keysv, hash);
         he=hv_fetch_ent(hv, keysv, TRUE, hash);
         elem_sv=HeVAL(he);
         if (existed)
            pm_perl_localize_scalar(aTHX_ elem_sv);
         else
            save_destructor_x(&delete_hash_elem, store_hash_ref_elem(aTHX_ hv, keyref));
         (void)POPs;
         SETs(elem_sv);
         RETURN;
      }
      SETs(ref2key(keysv, &tmp_key));
   } else if (class == my_pkg) {
      if (HvFILL(hv))
         ErrNoRef(keysv);
      else
         MarkAsNormalHash(hv);
   }
   return Perl_pp_helem(aTHX);
}

static
OP* intercept_pp_hslice(pTHX)
{
   dSP;
   HV *hv=(HV*)POPs, *class=SvSTASH(hv);
   MAGIC *mg;
   SV **firstkey=PL_stack_base+TOPMARK+1;
   I32 gimme;
   if (firstkey <= SP) {
      if (HashCPPbound(hv)) {
         PUTBACK;
         return pm_perl_cpp_hslice(aTHX_ hv, mg);
      }
      if (SvROK(*firstkey)) {
         if (RefKeyAllowed(hv,class)) {
            dMARK; dORIGMARK;
            tmp_keysv tmp_key;
            I32 lval = (PL_op->op_flags & OPf_MOD || LVRET);
            I32 localizing = lval && (PL_op->op_private & OPpLVAL_INTRO);
            HE* he;
            gimme=GIMME_V;

            while (++MARK <= SP) {
               SV *keysv=*MARK, *keyref=keysv;
               I32 existed=FALSE;
               U32 hash;
               if (!SvROK(keysv)) ErrNoRef(keysv);
               keysv=ref2key(keysv, &tmp_key);
               hash=TmpKeyHash(tmp_key);
               if (localizing) existed=hv_exists_ent(hv, keysv, hash);
               he=hv_fetch_ent(hv, keysv, lval, hash);
               *MARK=he ? HeVAL(he) : &PL_sv_undef;
               if (localizing) {
                  if (existed)
                     pm_perl_localize_scalar(aTHX_ *MARK);
                  else
                     save_destructor_x(&delete_hash_elem, store_hash_ref_elem(aTHX_ hv, keyref));
               }
            }

            if (gimme != G_ARRAY) {
               MARK = ORIGMARK;
               *++MARK = *SP;
               SP = MARK;
            }
            RETURN;

         } else {
            DIE(aTHX_ err_ref);
         }
      }
      else if (class==my_pkg) {
         if (HvFILL(hv))
            ErrNoRef(*firstkey);
         else
            MarkAsNormalHash(hv);
      }
      return Perl_pp_hslice(aTHX);
   }
   RETURN;
}

static
OP* intercept_pp_exists(pTHX)
{
   if (!(PL_op->op_private & OPpEXISTS_SUB)) {
      dSP;
      SV *keysv=TOPs;
      HV *hv=(HV*)TOPm1s, *class=SvSTASH(hv);
      MAGIC *mg;
      if (HashCPPbound(hv))
         return pm_perl_cpp_exists(aTHX_ hv, mg);
      if (SvROK(keysv)) {
         tmp_keysv tmp_key;
         (void)POPs; (void)POPs;
         if (class!=my_pkg && !(class && ref_key_allowed(class)))
            RETPUSHNO;
         keysv=ref2key(keysv, &tmp_key);
         if (hv_exists_ent(hv, keysv, TmpKeyHash(tmp_key)))
            RETPUSHYES;
         else
            RETPUSHNO;
      } else if (class==my_pkg) {
         (void)POPs; (void)POPs;
         RETPUSHNO;
      }
   }
   return Perl_pp_exists(aTHX);
}

static
OP* intercept_pp_delete(pTHX)
{
   dSP;
   tmp_keysv tmp_key;
   SV *sv, *keysv;
   HV *hv, *class;
   MAGIC *mg;
   I32 gimme, discard;

   if (PL_op->op_private & OPpSLICE) {
      hv=(HV*)POPs; class=SvSTASH(hv);
      if (HashCPPbound(hv)) {
         PUTBACK;
         return pm_perl_cpp_delete_hslice(aTHX_ hv, mg);
      } else {
         SV **firstkey=PL_stack_base+TOPMARK+1;
         if (firstkey <= SP) {
            if (SvROK(*firstkey)) {
               if (RefKeyAllowed(hv,class)) {
                  dMARK; dORIGMARK;
                  gimme = GIMME_V;
                  discard = (gimme == G_VOID) ? G_DISCARD : 0;

                  while (++MARK <= SP) {
                     keysv=*MARK;
                     if (!SvROK(keysv)) ErrNoRef(keysv);
                     keysv=ref2key(keysv, &tmp_key);
                     sv=hv_delete_ent(hv, keysv, discard, TmpKeyHash(tmp_key));
                     *MARK = sv ? sv : &PL_sv_undef;
                  }

                  if (discard)
                     SP = ORIGMARK;
                  else if (gimme == G_SCALAR) {
                     MARK = ORIGMARK;
                     *++MARK = *SP;
                     SP = MARK;
                  }
                  RETURN;
               } else {
                  DIE(aTHX_ err_ref);
               }
            } else if (class==my_pkg) {
               if (HvFILL(hv))
                  ErrNoRef(*firstkey);
               else
                  MarkAsNormalHash(hv);
            }
         }
      }

   } else {
      keysv=TOPs; hv=(HV*)TOPm1s; class=SvSTASH(hv);
      if (HashCPPbound(hv))
         return pm_perl_cpp_delete_helem(aTHX_ hv, mg);
      if (SvROK(keysv)) {
         if (RefKeyAllowed(hv,class)) {
            I32 discard = (GIMME_V == G_VOID) ? G_DISCARD : 0;
            (void)POPs; (void)POPs;
            keysv=ref2key(keysv, &tmp_key);
            sv=hv_delete_ent(hv, keysv, discard, TmpKeyHash(tmp_key));
            if (!discard) {
               if (!sv) sv = &PL_sv_undef;
               PUSHs(sv);
            }
            RETURN;
         } else {
            DIE(aTHX_ err_ref);
         }
      } else if (class==my_pkg) {
         if (HvFILL(hv))
            ErrNoRef(keysv);
         else
            MarkAsNormalHash(hv);
      }
   }
   return Perl_pp_delete(aTHX);
}

static
void key2ref(pTHX_ SV* keysv)
{
   U32 flags=PmFlagsForHashKey | SVf_POK | SVp_POK | SVf_ROK;
   key_or_ptr obj;
   obj.ptr=*(SV**)SvPVX(keysv);
#if PerlVersion < 5180
   if (obj.keyl & 1) {
      obj.keyl ^= 1;
      flags |= SVf_AMAGIC;
   }
#endif
   if ((SvFLAGS(keysv) & PmFlagsForHashKey) == PmFlagsForHashKey)
      Perl_unshare_hek(aTHX_ SvSHARED_HEK_FROM_PV(SvPVX_const(keysv)));
   SvFLAGS(keysv) ^= flags;
   SvRV(keysv)=obj.ptr;
#ifdef DEBUG_LEAKING_SCALARS
   if (obj.ptr->sv_flags == SVTYPEMASK || obj.ptr->sv_refcnt == 0)
      Perl_croak(aTHX_ "dead key %p", obj.ptr);
#endif
   SvREFCNT_inc_simple_void_NN(obj.ptr);
}

static
OP* intercept_pp_each(pTHX)
{
   dSP;
   HV *hv=(HV*)TOPs, *class=SvSTASH(hv);
   if (class==my_pkg || (class && ref_key_allowed(class))) {
      I32 sp_dist=SP-PL_stack_base;
      OP *ret=Perl_pp_each(aTHX);
      sp=PL_stack_base+sp_dist;
      if (PL_stack_sp >= sp) key2ref(aTHX_ *sp);
      return ret;
   }
   return Perl_pp_each(aTHX);
}

static
OP* intercept_pp_keys(pTHX)
{
   dSP;
   HV *hv=(HV*)TOPs, *class=SvSTASH(hv);
   MAGIC *mg;
   I32 gimme=GIMME_V;
   if (gimme == G_ARRAY && (class==my_pkg || (class && ref_key_allowed(class)))) {
      I32 sp_dist=SP-PL_stack_base;
      OP *ret=Perl_pp_keys(aTHX);
      SV **last=PL_stack_sp;
      for (sp=PL_stack_base+sp_dist; sp <= last; ++sp)
         key2ref(aTHX_ *sp);
      return ret;
   }
   if (gimme == G_SCALAR && HashCPPbound(hv))
      return pm_perl_cpp_keycnt(aTHX_ hv, mg);
   return Perl_pp_keys(aTHX);
}

/* aassign isn't intercepted directly, since it is used very often and not only with hashes.
   Instead, this routine is called from rv2hv and padhv when necessary */
static
OP* ref_assign(pTHX)
{
   dSP;
   I32 gimme = GIMME_V;
   HV* hv=(HV*)POPs, *class=SvSTASH(hv);
   MAGIC* mg;
   I32 lastR=TOPMARK, firstR=PL_markstack_ptr[-1]+1;
   const I32 assign_other= (SP-PL_stack_base) != lastR;
   I32 n_keys=0;

   if (assign_other) {
      SV **lhs=PL_stack_base+lastR+1;
      do {
         I32 type=SvTYPE(*lhs);
         if (type == SVt_PVAV || type == SVt_PVHV) {
            firstR=lastR;
            break;
         }
         ++firstR;
      } while (++lhs <= SP);
   }
   if (HashCPPbound(hv)) {
      PUTBACK;
      n_keys=pm_perl_cpp_hassign(aTHX_ hv, mg, &firstR, lastR, !assign_other);
      SPAGAIN;

   } else if (firstR < lastR && SvROK(PL_stack_base[firstR])) {
      if (!RefKeyAllowed(hv,class))
         DIE(aTHX_ err_ref);

      // the assignment loop is borrowed from the appropriate branch in pp_aassign
      hv_clear(hv);
      do {
         tmp_keysv tmp_key;
         SV *keysv=PL_stack_base[firstR++], *tmp_val;
         if (!keysv || !SvROK(keysv))
            ErrNoRef(keysv);
         keysv = ref2key(keysv, &tmp_key);
         tmp_val = PL_stack_base[firstR] ? newSVsv(PL_stack_base[firstR]) : newSV_type(SVt_NULL);    // value
         PL_stack_base[firstR++] = tmp_val;
         (void)hv_store_ent(hv, keysv, tmp_val, TmpKeyHash(tmp_key));
      } while (firstR < lastR);

      if (firstR == lastR) {
         SV* keysv=PL_stack_base[firstR];
         if (!keysv || !SvROK(keysv))
            ErrNoRef(keysv);
         if (SvSTASH(SvRV(keysv)) == my_pkg)
            DIE(aTHX_ "RefHash object assignment in list context");
         else
            DIE(aTHX_ "Key without value in hash assignment");
      }
      n_keys=HvFILL(hv);

   } else {
      if (class==my_pkg) MarkAsNormalHash(hv);
      return Perl_pp_aassign(aTHX);
   }

   if (assign_other) {
      OP *ret;
      PUTBACK;
      ret=Perl_pp_aassign(aTHX);
      if (gimme == G_ARRAY) {
         SP=PL_stack_base+lastR;
         PUTBACK;
      }
      return ret;
   }

   PL_markstack_ptr-=2;
   if (gimme == G_VOID)
      SP=PL_stack_base+firstR-1;
   else if (gimme == G_ARRAY)
      SP=PL_stack_base+lastR;
   else {
      dTARGET;
      SP=PL_stack_base+firstR;
      SETi(n_keys*2);
   }
   RETURN;
}

static
OP* pp_pushhv(pTHX)
{
   dSP; dMARK; dORIGMARK;
   HV *hv=(HV*)*++MARK, *class=SvSTASH(hv);
   SV *keysv, *value, *tmp_val;

   if (MARK < SP) {
      if (SvROK(MARK[1])) {
         if (RefKeyAllowed(hv,class)) {
            tmp_keysv tmp_key;
            do {
               keysv=*++MARK;
               if (!SvROK(keysv)) ErrNoRef(keysv);
               keysv=ref2key(keysv, &tmp_key);
               value=*++MARK;
               tmp_val = value ? newSVsv(value) : newSV_type(SVt_NULL);      // copy of the value
               (void)hv_store_ent(hv, keysv, tmp_val, TmpKeyHash(tmp_key));
            } while (MARK < SP);
         } else {
            DIE(aTHX_ err_ref);
         }
      } else {
         if (class==my_pkg) {
            if (HvFILL(hv))
               ErrNoRef(MARK[1]);
            else
               MarkAsNormalHash(hv);
         }
         do {
            keysv=*++MARK;
            if (SvROK(keysv))
               DIE(aTHX_ err_ref);
            value=*++MARK;
            tmp_val = value ? newSVsv(value) : newSV_type(SVt_NULL);          // copy of the value
            (void)hv_store_ent(hv, keysv, tmp_val, SvSHARED_HASH(keysv));
         } while (MARK < SP);
      }
   }
   SP=ORIGMARK;
   RETURN;
}

static
OP* pp_rv2hv_ref_retrieve(pTHX)
{
   dSP;
   I32 sp_dist=SP-PL_stack_base;
   OP *ret=Perl_pp_rv2hv(aTHX);
   SV **last=PL_stack_sp;
   for (SP=PL_stack_base+sp_dist; SP < last; SP+=2)
      key2ref(aTHX_ *SP);
   return ret;
}

static
OP* pp_padhv_ref_retrieve(pTHX)
{
   dSP;
   I32 sp_dist=SP-PL_stack_base+1;
   OP *ret=Perl_pp_padhv(aTHX);
   SV **last=PL_stack_sp;
   for (SP=PL_stack_base+sp_dist; SP < last; SP+=2)
      key2ref(aTHX_ *SP);
   return ret;
}

static
OP* intercept_pp_rv2hv(pTHX)
{
   dSP;
   SV *hv=TOPs;
   HV *class;
   if (PL_op->op_flags & OPf_REF) {
      MAGIC *mg;
      if (PL_op->op_next->op_type == OP_AASSIGN) {
         PL_op=Perl_pp_rv2hv(aTHX);
         return ref_assign(aTHX);
      }
      if (SvROK(hv) &&
          (hv=SvRV(hv), class=SvSTASH(hv), (SvTYPE(hv)==SVt_PVHV || SvTYPE(hv)==SVt_PVAV) && HashCPPbound(hv)) &&
          pm_perl_cpp_has_assoc_methods(mg)) {
         /* escape the type check in rv2hv=rv2av in perl 5.10 */
         SETs(hv);
         RETURN;
      }
   } else if (GIMME_V == G_ARRAY) {
      if (SvROK(hv)) {  /* the easiest and most often case */
         class=SvSTASH(SvRV(hv));
         if (class==my_pkg || (class && ref_key_allowed(class)))
            return pp_rv2hv_ref_retrieve(aTHX);
         else
            return Perl_pp_rv2hv(aTHX);
      }
      SAVEI8(PL_op->op_flags);  /* just for the case the op dies */
      PL_op->op_flags ^= OPf_REF;
      Perl_pp_rv2hv(aTHX);               /* get the hash */
      PL_op->op_flags ^= OPf_REF;
      hv=TOPs; class=SvSTASH(hv);
      if (class==my_pkg || (class && ref_key_allowed(class)))
         return pp_rv2hv_ref_retrieve(aTHX);
   }
   return Perl_pp_rv2hv(aTHX);
}

static
OP* intercept_pp_padhv(pTHX)
{
   if (PL_op->op_flags & OPf_REF) {
      if (PL_op->op_next->op_type == OP_AASSIGN) {
         PL_op=Perl_pp_padhv(aTHX);
         return ref_assign(aTHX);
      }
   } else if (GIMME_V == G_ARRAY) {
      dTARGET;
      HV *hv=(HV*)TARG, *class=SvSTASH(hv);
      if (class==my_pkg || (class && ref_key_allowed(class))) {
         return pp_padhv_ref_retrieve(aTHX);
      }
   }
   return Perl_pp_padhv(aTHX);
}

#if PerlVersion >= 5180
static
OP* intercept_pp_padrange_known(pTHX)
{
   PL_op=Perl_pp_padrange(aTHX);
   return ref_assign(aTHX);
}

static
OP* intercept_pp_padrange_unknown(pTHX)
{
   OP* o=PL_op;
   OP* sib=OpSIBLING(o);
   OP* next=Perl_pp_padrange(aTHX);
   if (next->op_type == OP_AASSIGN) {
      while (sib) {
         if (sib->op_type == OP_PADHV && (sib->op_flags & OPf_REF)) {
            o->op_ppaddr=&intercept_pp_padrange_known;
            PL_op=next;
            return ref_assign(aTHX);
         }
         sib=OpSIBLING(sib);
      }
   }
   o->op_ppaddr=def_pp_PADRANGE;
   return next;
}
#endif

static
OP* pp_ref_anonhash(pTHX)
{
    dSP; dMARK; dORIGMARK;
    HV* hv = newHV();
    tmp_keysv tmp_key;
    SV *keysv, *val;
    MarkAsRefHash(hv);
    while (++MARK < SP) {
        keysv = *MARK;
        if (!SvROK(keysv)) ErrNoRef(keysv);
        keysv = ref2key(keysv, &tmp_key);
        val = MARK < SP ? newSVsv(*++MARK) : newSV_type(SVt_NULL);
        (void)hv_store_ent(hv, keysv, val, TmpKeyHash(tmp_key));
    }
    SP = ORIGMARK;
    XPUSHs(sv_2mortal((PL_op->op_flags & OPf_SPECIAL)
                      ? newRV_noinc((SV*)hv) : (SV*)hv));
    RETURN;
}

static
OP* intercept_pp_anonhash(pTHX)
{
   dSP;
   SV **firstkey=PL_stack_base+TOPMARK+1;
   if (firstkey<SP && SvROK(*firstkey))
      return pp_ref_anonhash(aTHX);
   return Perl_pp_anonhash(aTHX);
}

static
OP* check_pushhv(pTHX_ OP *o)
{
   if (o->op_flags & OPf_KIDS) {
      OP *kid=cLISTOPo->op_first;
      if (kid->op_type == OP_PUSHMARK ||
          (kid->op_type == OP_NULL && kid->op_targ == OP_PUSHMARK))
         kid = OpSIBLING(kid);
      if (kid->op_type == OP_RV2HV || kid->op_type == OP_PADHV) {
         int arg_cnt=2;
         op_lvalue(kid, o->op_type);
         while ((kid=OpSIBLING(kid))) {
            if (kid->op_type == OP_RV2HV || kid->op_type == OP_PADHV) {
               Perl_list(aTHX_ kid);
            } else {
               Perl_yyerror(aTHX_ Perl_form(aTHX_ "Type of arg %d to push must be hash (not %s)", arg_cnt, OP_DESC(kid)));
            }
            ++arg_cnt;
         }
         o->op_ppaddr=&pp_pushhv;
         return o;
      }
   }
   return Perl_ck_fun(aTHX_ o);
}

#if PerlVersion >= 5220
// The following senseless routines have a sole purpose:
// to prevent the operations HELEM, EXISTS, and DELETE from being lumped together with MULTIDEREF.
// The concrete manipulations have been deduced from studying the source code of S_maybe_multideref,
// they might need to be adapted in the future versions.

static
OP* intercept_ck_helem(pTHX_ OP *o)
{
   // currently it's enough just to install a non-standard check hook
   return def_ck_HELEM(aTHX_ o);
}

// For EXISTS and DELETE, it's enough to mark the operation delivering the key with a flag OPf_REF;
// this flag does not influence the operation itself but, weirdly enough, is respected by S_maybe_multideref.
static
void protect_key_operand(pTHX_ OP* o)
{
   o=cUNOPo->op_first;  // null = former HELEM or HSLICE
   assert(o->op_type == OP_NULL);
   if (o->op_targ != OP_HELEM) return;

   o=cBINOPo->op_last;  // key source
   switch (o->op_type)
   {
   case OP_PADSV:
      o->op_flags |= OPf_REF;
      break;
   case OP_RV2SV:
      if (cUNOPo->op_first->op_type == OP_GV)
         o->op_flags |= OPf_REF;
      break;
   }
}

static
OP* intercept_ck_exists(pTHX_ OP *o)
{
   o=def_ck_EXISTS(aTHX_ o);
   protect_key_operand(aTHX_ o);
   return o;
}

static
OP* intercept_ck_delete(pTHX_ OP *o)
{
   o=def_ck_DELETE(aTHX_ o);
   protect_key_operand(aTHX_ o);
   return o;
}

#endif

static
OP* intercept_pp_const(pTHX)
{
   SV *sv=cSVOP_sv;
   if ((PL_op->op_private & OPpCONST_BARE)  &&  SvTYPE(sv)==SVt_PV)
      SvIsUV_on(sv);
   PL_op->op_ppaddr=&Perl_pp_const;
   return Perl_pp_const(aTHX);
}

static
void catch_ptrs(pTHX_ SV *dummy)
{
   PL_ppaddr[OP_CONST]=&intercept_pp_const;
   PL_ppaddr[OP_HELEM]=&intercept_pp_helem;
   PL_ppaddr[OP_HSLICE]=&intercept_pp_hslice;
   PL_ppaddr[OP_EXISTS]=&intercept_pp_exists;
   PL_ppaddr[OP_DELETE]=&intercept_pp_delete;
   PL_ppaddr[OP_EACH]=&intercept_pp_each;
   PL_ppaddr[OP_KEYS]=&intercept_pp_keys;
   PL_ppaddr[OP_RV2HV]=&intercept_pp_rv2hv;
   PL_ppaddr[OP_PADHV]=&intercept_pp_padhv;
#if PerlVersion >= 5180
   PL_ppaddr[OP_PADRANGE]=&intercept_pp_padrange_unknown;
#endif
   PL_ppaddr[OP_ANONHASH]=&intercept_pp_anonhash;
   PL_check[OP_PUSH]=&check_pushhv;
#if PerlVersion >= 5220
   PL_check[OP_HELEM]=&intercept_ck_helem;
   PL_check[OP_EXISTS]=&intercept_ck_exists;
   PL_check[OP_DELETE]=&intercept_ck_delete;
#endif
}

static
void reset_ptrs(pTHX_ SV *dummy)
{
   PL_ppaddr[OP_CONST]=def_pp_CONST;
   PL_ppaddr[OP_HELEM]=def_pp_HELEM;
   PL_ppaddr[OP_HSLICE]=def_pp_HSLICE;
   PL_ppaddr[OP_EXISTS]=def_pp_EXISTS;
   PL_ppaddr[OP_DELETE]=def_pp_DELETE;
   PL_ppaddr[OP_EACH]=def_pp_EACH;
   PL_ppaddr[OP_KEYS]=def_pp_KEYS;
   PL_ppaddr[OP_RV2HV]=def_pp_RV2HV;
   PL_ppaddr[OP_PADHV]=def_pp_PADHV;
#if PerlVersion >= 5180
   PL_ppaddr[OP_PADRANGE]=def_pp_PADRANGE;
#endif
   PL_ppaddr[OP_ANONHASH]=def_pp_ANONHASH;
   PL_check[OP_PUSH]=def_ck_PUSH;
#if PerlVersion >= 5220
   PL_check[OP_HELEM]=def_ck_HELEM;
   PL_check[OP_EXISTS]=def_ck_EXISTS;
   PL_check[OP_DELETE]=def_ck_DELETE;
#endif
}

MODULE = Polymake::RefHash              PACKAGE = Polymake

PROTOTYPES: DISABLE

void
is_keyword(sv)
   SV *sv;
PPCODE:
{
   if ((SvFLAGS(sv) & (SVf_POK|SVf_IVisUV)) == (SVf_POK|SVf_IVisUV))
      PUSHs(&PL_sv_yes);
   else
      PUSHs(&PL_sv_no);
}

MODULE = Polymake::RefHash              PACKAGE = Polymake::RefHash

void
allow(pkg)
   SV *pkg;
PPCODE:
{
   av_push(allowed_pkgs, newRV((SV*)gv_stashsv(pkg,FALSE)));
}

BOOT:
{
   my_pkg=gv_stashpv("Polymake::RefHash", FALSE);
   allowed_pkgs=newAV();
   def_pp_CONST=PL_ppaddr[OP_CONST];
   def_pp_HELEM=PL_ppaddr[OP_HELEM];
   def_pp_HSLICE=PL_ppaddr[OP_HSLICE];
   def_pp_EXISTS=PL_ppaddr[OP_EXISTS];
   def_pp_DELETE=PL_ppaddr[OP_DELETE];
   def_pp_EACH=PL_ppaddr[OP_EACH];
   def_pp_KEYS=PL_ppaddr[OP_KEYS];
   def_pp_RV2HV=PL_ppaddr[OP_RV2HV];
   def_pp_PADHV=PL_ppaddr[OP_PADHV];
#if PerlVersion >= 5180
   def_pp_PADRANGE=PL_ppaddr[OP_PADRANGE];
#endif
   def_pp_ANONHASH=PL_ppaddr[OP_ANONHASH];
   def_ck_PUSH=PL_check[OP_PUSH];
#if PerlVersion >= 5220
   def_ck_HELEM=PL_check[OP_HELEM];
   def_ck_EXISTS=PL_check[OP_EXISTS];
   def_ck_DELETE=PL_check[OP_DELETE];
#endif
   pm_perl_namespace_register_plugin(aTHX_ catch_ptrs, reset_ptrs, &PL_sv_undef);
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
