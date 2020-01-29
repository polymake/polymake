/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_PERL_MACROS_H
#define POLYMAKE_PERL_MACROS_H

/* Most of the following macros are designed solely for the automatically generated wrapper files.
   If you use them in a manually written code, you are doing it at your own risk!

   The changes in syntax and contents must be concerted with the corresponding routines
   in the package Polymake::Core::CPlusPlus
*/

#define MacroTokenAsString(arg) #arg

#define RegistratorInstance4perl(...) \
   template<> __VA_ARGS__ __VA_ARGS__::r

#define ClassInstance4perl(pkg, file, inst_num, ...) \
   RegistratorInstance4perl( QueueingRegistrator4perl<polymake::perl_bindings::Class<__VA_ARGS__>, inst_num> ) \
   (pkg, file, inst_num)

#define Class4perl(inst_num, pkg, ...) ClassInstance4perl(#pkg, POLYMAKE_CPPERL_FILE, inst_num, __VA_ARGS__)

#define Builtin4perl(inst_num, pkg, ...) \
   RegistratorInstance4perl( QueueingRegistrator4perl<pm::perl::Builtin<__VA_ARGS__>, inst_num> ) \
   (#pkg, POLYMAKE_CPPERL_FILE, inst_num)

#define ClassTemplate4perl(inst_num, pkg) \
   RegistratorInstance4perl( StaticRegistrator4perl<pm::perl::ClassTemplate, inst_num> ) \
   (#pkg)

#define FunctionCallerTagsClass4perl Function__caller_tags_4perl
#define FunctionCallerBodyClass4perl Function__caller_body_4perl

#define FunctionCallerStart4perl \
template <typename CallerTag, pm::perl::FunctionCaller::FuncKind Kind> \
struct FunctionCallerBodyClass4perl; \
struct FunctionCallerTagsClass4perl

#define FunctionCallerName4perl(name, kind) \
FunctionCallerBodyClass4perl<FunctionCallerTagsClass4perl::name, pm::perl::FunctionCaller::FuncKind::kind>

#define FunctionCallerBody4perl(name, kind) \
template <>                                 \
struct FunctionCallerName4perl(name, kind)

// Certain amount of code duplication in the following call adapters is caused by two circumstances:
// - a void function can't be placed within another function call, even if that does not take any arguments,
//   therefore every adapter has a variant without consumer
// - consumer must be called before any temporary object created during argument retrieval is destroyed,
//   therefore the variants with consumers can't reuse variants without

// free function
#define FunctionCallerBody_free_4perl(name)                             \
FunctionCallerBody4perl(name, free)                                     \
   : public pm::perl::FunctionCaller {                                  \
   template <size_t... I_, typename... T_>                              \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<>, mlist<T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      return name(args_.template get<I_, T_>()...);                     \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename... T_>          \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<>, mlist<T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      return consumer_(name(args_.template get<I_, T_>()...), args_);   \
   }                                                                    \
}

// free function template requiring explicit type parameters
#define FunctionCallerBody_free_t_4perl(name)                           \
FunctionCallerBody4perl(name, free_t)                                   \
   : public pm::perl::FunctionCaller {                                  \
   template <size_t... I_, typename... E_, typename... T_>              \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<E_...>, mlist<T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      return name<E_...>(args_.template get<I_, T_>()...);              \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename... E_, typename... T_> \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<E_...>, mlist<T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      return consumer_(name<E_...>(args_.template get<I_, T_>()...), args_); \
   }                                                                    \
}

// class instance method
#define FunctionCallerBody_meth_4perl(name)                             \
FunctionCallerBody4perl(name, meth)                                     \
   : public pm::perl::FunctionCaller {                                  \
   template <size_t... I_, typename T0_, typename... T_>                \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)+1>& args_, \
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const \
   {                                                                    \
      return args_.template get<0, T0_>().name(args_.template get<I_, T_>()...); \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename T0_, typename... T_> \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)+1>& args_, \
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const \
   {                                                                    \
      return consumer_(args_.template get<0, T0_>().name(args_.template get<I_, T_>()...), args_); \
   }                                                                    \
}

// class instance method template requiring explicit type parameters
#define FunctionCallerBody_meth_t_4perl(name)                           \
FunctionCallerBody4perl(name, meth_t)                                   \
   : public pm::perl::FunctionCaller {                                  \
   template <size_t... I_, typename... E_, typename T0_, typename... T_> \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)+1>& args_, \
                             mlist<E_...>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const \
   {                                                                    \
      return args_.template get<0, T0_>().template name<E_...>(args_.template get<I_, T_>()...); \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename... E_, typename T0_, typename... T_> \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)+1>& args_, \
                             mlist<E_...>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const \
   {                                                                    \
      return consumer_(args_.template get<0, T0_>().template name<E_...>(args_.template get<I_, T_>()...), args_); \
   }                                                                    \
}

// class static method
#define FunctionCallerBody_stat_4perl(name)                             \
FunctionCallerBody4perl(name, stat)                                     \
   : public pm::perl::StaticFunctionCaller {                            \
   template <size_t... I_, typename T0_, typename... T_>                \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      using class_t_ = pm::perl::static_class_t<T0_>;                   \
      return class_t_::name(args_.template get<I_, T_>()...);           \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename T0_, typename... T_> \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      using class_t_ = pm::perl::static_class_t<T0_>;                   \
      return consumer_(class_t_::name(args_.template get<I_, T_>()...), args_); \
   }                                                                    \
}

// class static method template requiring explicit type parameters
#define FunctionCallerBody_stat_t_4perl(name)                           \
FunctionCallerBody4perl(name, stat_t)                                   \
   : public pm::perl::StaticFunctionCaller {                            \
   template <size_t... I_, typename... E_, typename T0_, typename... T_> \
   decltype(auto) operator()(const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<E_...>, mlist<T0_, T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      using class_t_ = pm::perl::static_class_t<T0_>;                   \
      return class_t_::template name<E_...>(args_.template get<I_, T_>()...); \
   }                                                                    \
   template <typename Consumer_, size_t... I_, typename... E_, typename T0_, typename... T_> \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<sizeof...(T_)>& args_, \
                             mlist<E_...>, mlist<T0_, T_...>, std::index_sequence<I_...>) const \
   {                                                                    \
      using class_t_ = pm::perl::static_class_t<T0_>;                   \
      return consumer_(class_t_::template name<E_...>(args_.template get<I_, T_>()...), args_); \
   }                                                                    \
}

#define FunctionCallerBodyImpl4perl(kind) FunctionCallerBody_##kind##_4perl
#define FunctionCaller4perl(name, kind) FunctionCallerBodyImpl4perl(kind)(name)

#define OperatorCallerName4perl(name) Operator_##name##__caller_4perl

#define UnaryOperatorCallerBody4perl(sign, name)                        \
struct OperatorCallerName4perl(name)                                    \
   : public pm::perl::FunctionCaller {                                  \
   template <typename T_>                                               \
   decltype(auto) operator()(const pm::perl::ArgValues<1>& args_,       \
                             mlist<>, mlist<T_>, std::index_sequence<0>) const \
   {                                                                    \
      return sign(args_.template get<0, T_>());                         \
   }                                                                    \
   template <typename Consumer_, typename T_>                           \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<1>& args_,       \
                             mlist<>, mlist<T_>, std::index_sequence<0>) const \
   {                                                                    \
      return consumer_(sign(args_.template get<0, T_>()), args_);       \
   }                                                                    \
}

#define BinaryOperatorCallerBody4perl(sign, name)                       \
struct OperatorCallerName4perl(name) : pm::perl::FunctionCaller {       \
   template <typename T0_, typename T1_>                                \
   decltype(auto) operator()(const pm::perl::ArgValues<2>& args_,       \
                             mlist<>, mlist<T0_, T1_>, std::index_sequence<0, 1>) const \
   {                                                                    \
      return args_.template get<0, T0_>() sign args_.template get<1, T1_>(); \
   }                                                                    \
   template <typename Consumer_, typename T0_, typename T1_>            \
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const pm::perl::ArgValues<2>& args_,       \
                             mlist<>, mlist<T0_, T1_>, std::index_sequence<0, 1>) const \
   {                                                                    \
      return consumer_(args_.template get<0, T0_>() sign args_.template get<1, T1_>(), args_); \
   }                                                                    \
}

#define SubstTypeList4perl TypeList4perl
#define TypeList4perl(...) mlist<__VA_ARGS__>

#define FunctionTemplateInstance4perl(inst_num, name, kind, uniq_name, flags, n_explicit, arg_types, ...) \
   RegistratorInstance4perl( QueueingRegistrator4perl<pm::perl::FunctionWrapper<FunctionCallerName4perl(name, kind), \
                             flags, n_explicit, SubstTypeList4perl arg_types>, inst_num> ) \
   (#uniq_name, POLYMAKE_CPPERL_FILE, inst_num, ##__VA_ARGS__)

#define OperatorTemplateInstance4perl(inst_num, name, uniq_name, flags, arg_types, ...) \
   RegistratorInstance4perl( QueueingRegistrator4perl<pm::perl::FunctionWrapper<pm::perl::OperatorCallerName4perl(name), \
                             flags, 0, SubstTypeList4perl arg_types>, inst_num> ) \
   (#uniq_name, POLYMAKE_CPPERL_FILE, inst_num, ##__VA_ARGS__)


#define FunctionInstance4perl(f_class, ...) ERROR obsolete wrapper - please delete and regenerate
#define OperatorInstance4perl(op_class, ...) ERROR obsolete wrapper - please delete and regenerate
#define FunctionCrossAppInstance4perl(f_class, app_list, ...) ERROR obsolete wrapper - please delete and regenerate
#define OperatorCrossAppInstance4perl(op_class, app_list, ...) ERROR obsolete wrapper - please delete and regenerate

#define FunctionInterface4perl(name, ...) ERROR obsolete wrapper - please delete and regenerate

#define FunctionWrapper4perl(...) ERROR obsolete wrapper - please delete and regenerate

#define FunctionWrapperInstance4perl(...) ERROR obsolete wrapper - please delete and regenerate

// ---

#define FindDefinitionSource4perl2(dir,name) MacroTokenAsString(dir/name)
#define FindDefinitionSource4perl(name) FindDefinitionSource4perl2(POLYMAKE_DEFINITION_SOURCE_DIR, name)

#ifdef POLYMAKE_NO_EMBEDDED_RULES
// module with wrapper code only; proper definitions are located in core or another extension

#define DeclareRegularFunction(...) namespace { }
#define InsertEmbeddedRule(...) namespace { }
#define OpaqueClass4perl(...) namespace { }
#define OpaqueMethod4perl(...)

#else

#define SourceLine4perl(line, file) "#line " MacroTokenAsString(line) " \"" MacroTokenAsString(file) "\"\n"

#define DeclareRegularFunction(pre, fptr, decl)  \
namespace {                                       \
   RegistratorInstance4perl( QueueingRegistrator4perl<pm::perl::RegularFunctionWrapper<decltype(fptr),fptr>,__LINE__> ) \
   (pre " " decl " : c++ (regular=>%d);\n", SourceLine4perl(__LINE__, POLYMAKE_DEFINITION_SOURCE_FILE), 0); \
}

#define InsertEmbeddedRule(text)                                                      \
namespace {                                                                            \
   RegistratorInstance4perl( QueueingRegistrator4perl<pm::perl::EmbeddedRule,__LINE__> ) \
      (text, SourceLine4perl(__LINE__, POLYMAKE_DEFINITION_SOURCE_FILE));                               \
}

#define FullPackage4perl(pkg, app) "Polymake::" MacroTokenAsString(app) "::" pkg

#define OpaqueClass4perl(pkg, name, methods)                                        \
namespace {                                                                         \
   ClassInstance4perl(FullPackage4perl(pkg, POLYMAKE_APPNAME), nullptr, 0, name);   \
}                                                                                   \
InsertEmbeddedRule("# @hide\n"                                                      \
                   "declare property_type " pkg " : c++ (special=>'" #name "') {\n" \
                   methods                                                          \
                   "}\n")

#define OpaqueMethod4perl(decl) "\nmethod " decl " : c++;\n"

#endif

#define Function4perl(fptr, decl)           DeclareRegularFunction(            "function",fptr,decl)
#define UserFunction4perl(help, fptr, decl) DeclareRegularFunction(help "\nuser_function",fptr,decl)

#define FunctionTemplate4perl(decl)           InsertEmbeddedRule("function " decl " : c++;\n")
#define UserFunctionTemplate4perl(help, decl) InsertEmbeddedRule(help "\nuser_function " decl " : c++;\n")

#define RecognizeType4perl(name, typelist, ...)                         \
decltype(auto) recognize(pm::perl::type_infos& ti, bait, T*, __VA_ARGS__*) \
{                                                                       \
   const std::is_same<T, __VA_ARGS__ > exact_match{};                   \
   if (SV* proto = pm::perl::PropertyTypeBuilder::build(name, SubstTypeList4perl typelist (), exact_match)) \
      ti.set_proto(proto);                                              \
   return exact_match;                                                  \
}

#endif // POLYMAKE_MACROS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
