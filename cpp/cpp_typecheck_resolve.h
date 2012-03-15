/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#ifndef CPROVER_CPP_TYPECHECK_RESOLVE_H
#define CPROVER_CPP_TYPECHECK_RESOLVE_H

#include "cpp_typecheck_fargs.h"
#include "cpp_name.h"

class cpp_typecheck_resolvet
{
public:
  cpp_typecheck_resolvet(
    class cpp_typecheckt &_cpp_typecheck);

  typedef enum { VAR, TYPE, BOTH } wantt;

  exprt resolve(
    const cpp_namet &cpp_name,
    const wantt want,
    const cpp_typecheck_fargst &fargs,
    bool fail_with_exception=true);

  // Returns the scope as a side-effect as 'current_scope'.
  // Should really return explicitly.
  cpp_scopet &resolve_scope(
    const cpp_namet &cpp_name,
    std::string &base_name,
    irept &template_args);

  cpp_scopet &resolve_namespace(const cpp_namet &cpp_name);

protected:
  cpp_typecheckt &cpp_typecheck;
  locationt location;
  cpp_scopet *original_scope;
  
  typedef std::set<exprt> resolve_identifierst;

  void convert_identifiers(
    const cpp_scopest::id_sett &id_set,
    const wantt want,
    const cpp_typecheck_fargst &fargs,
    resolve_identifierst &identifiers);

  exprt convert_template_argument(
    const cpp_idt &id);

  exprt convert_identifier(
    const cpp_idt &id,
    const wantt want,
    const cpp_typecheck_fargst &fargs);

  void disambiguate(
    resolve_identifierst &identifiers,
    const cpp_typecheck_fargst &fargs);

  void filter(
    resolve_identifierst &identifiers,
    const wantt want);

  void make_constructors(
    resolve_identifierst &identifiers);

  void apply_template_args(
    resolve_identifierst &identifiers,
    const cpp_template_args_non_tct &template_args,
    const cpp_typecheck_fargst &fargs);

  void apply_template_args(
    exprt &expr,
    const cpp_template_args_non_tct &template_args_non_tc,
    const cpp_typecheck_fargst &fargs);

  void guess_function_template_args(
    resolve_identifierst &identifiers,
    const cpp_typecheck_fargst &fargs);

  void remove_templates(
    resolve_identifierst &identifiers);

  void remove_duplicates(
    resolve_identifierst &identifiers);

  exprt guess_function_template_args(
    const exprt &expr,
    const cpp_typecheck_fargst &fargs);

  void guess_template_args(
    const typet &template_parameter,
    const typet &desired_type);

  void guess_template_args(
    const exprt &template_parameter,
    const exprt &desired_expr);

  bool disambiguate(
    const exprt &expr,
    unsigned &args_distance,
    const cpp_typecheck_fargst &fargs);

  exprt do_builtin(
    const irep_idt &base_name,
    irept &template_args);

  void show_identifiers(
    const std::string &base_name,
    const resolve_identifierst &identifiers,
    std::ostream &out);

  void resolve_with_arguments(
    cpp_scopest::id_sett &id_set,
    const std::string &base_name,
    const cpp_typecheck_fargst &fargs);

  void filter_for_named_scopes(cpp_scopest::id_sett &id_set);
  void filter_for_namespaces(cpp_scopest::id_sett &id_set);

  #ifdef CPP_SYSTEMC_EXTENSION
  exprt do_builtin_sc_uint_extension(
    const cpp_namet &cpp_name,
    const cpp_template_args_non_tct &template_args);

  exprt do_builtin_sc_int_extension(
    const cpp_namet &cpp_name,
    const cpp_template_args_non_tct &template_args);

  exprt do_builtin_sc_logic_extension(
    const cpp_namet &cpp_name,
    const cpp_template_args_non_tct &template_args);

  exprt do_builtin_sc_lv_extension(
    const cpp_namet &cpp_name,
    const cpp_template_args_non_tct &template_args);
  #endif
};

#endif
