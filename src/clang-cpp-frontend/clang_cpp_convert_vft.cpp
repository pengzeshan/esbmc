#include <util/compiler_defs.h>
// Remove warnings from Clang headers
CC_DIAGNOSTIC_PUSH()
CC_DIAGNOSTIC_IGNORE_LLVM_CHECKS()
#include <clang/AST/Attr.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclFriend.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/Type.h>
#include <clang/Index/USRGeneration.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/AST/ParentMapContext.h>
#include <llvm/Support/raw_os_ostream.h>
CC_DIAGNOSTIC_POP()

#include <clang-cpp-frontend/clang_cpp_convert.h>
#include <util/expr_util.cpp>

bool clang_cpp_convertert::get_struct_class_virtual_methods(
  const clang::CXXRecordDecl *cxxrd,
  struct_typet &type)
{
  for(const auto &md : cxxrd->methods())
  {
    if(!md->isVirtual())
      continue;

    /*
     * 1. convert this virtual method and add them to class symbol type
     */
    struct_typet::componentt comp;
    if(get_decl(*md, comp))
      return true;

    // additional annotations for virtual/overriding methods
    if(annotate_virtual_overriding_methods(md, comp))
      return true;
    type.methods().push_back(comp);

    /*
     * 2. If this is the first time we see a virtual method in this class,
     *  add virtual table type symbol and virtual pointer. Then add a new
     *  entry in the vtable.
     */
    symbolt *vtable_type_symbol = check_vtable_type_symbol_existence(type);
    if(!vtable_type_symbol)
    {
      // first time we create the vtable type for this class
      vtable_type_symbol = add_vtable_type_symbol(md, comp, type);
      if(vtable_type_symbol == nullptr)
        return true;

      add_vptr(md, type);
    }

    /*
     * 3. add an entry in the existing virtual table type symbol
     */
    add_vtable_type_entry(type, comp, vtable_type_symbol);

    /*
     * 4. deal with overriding method
     */
    if(md->begin_overridden_methods() != md->end_overridden_methods())
    {
      /*
       * Assume it *always* points to one overriden method in base class
       * TODO: Use a loop if there are more overriden methods
       *    md->overriden_methods() should do the job
       */
      assert(md->size_overridden_methods() == 1);
      add_thunk_method(*(md->begin_overridden_methods()), comp, type);
    }
  }

  return false;
}

bool clang_cpp_convertert::annotate_virtual_overriding_methods(
  const clang::CXXMethodDecl *md,
  struct_typet::componentt &comp)
{
  // TODO: take care of overriding methods
  //assert(md->begin_overridden_methods() == md->end_overridden_methods());

  comp.type().set("#is_virtual", true);
  comp.type().set("#virtual_name", comp.name().as_string());
  comp.set("is_virtual", true);
  comp.set("virtual_name", comp.name().as_string());

  return false;
}

symbolt *
clang_cpp_convertert::check_vtable_type_symbol_existence(struct_typet &type)
{
  irep_idt vt_name = vtable_type_prefix + tag_prefix + type.tag().as_string();
  return context.find_symbol(vt_name);
}

symbolt *clang_cpp_convertert::add_vtable_type_symbol(
  const clang::CXXMethodDecl *md,
  const struct_typet::componentt &comp,
  struct_typet &type)
{
  /*
   *  We model the type of the virtual table as a struct type, something like:
   *   typedef struct {
   *      void (*do_something)(Base*);
   *    } VftTag_Base;
   * Later, we will instantiate a virtual table as:
   *  VftTag_Base vtable_TagBase@TagBase = { .do_something = &TagBase::do_someting(); }
   *
   *  Vtable type has the id in the form of `virtual_table::tag-BLAH`.
   */

  irep_idt vt_name = vtable_type_prefix + tag_prefix + type.tag().as_string();

  symbolt vt_type_symb;
  vt_type_symb.id = vt_name;
  vt_type_symb.name = vtable_type_prefix + type.tag().as_string();
  vt_type_symb.mode = mode;
  vt_type_symb.type = struct_typet();
  vt_type_symb.is_type = true;
  vt_type_symb.type.set("name", vt_type_symb.id);
  vt_type_symb.location = comp.location();
  vt_type_symb.module =
    get_modulename_from_path(comp.location().file().as_string());

  if(context.move(vt_type_symb))
  {
    log_error(
      "Failed add vtable type symbol {} to symbol table", vt_type_symb.id);
    abort();
  }

  return context.find_symbol(vt_name);
}

void clang_cpp_convertert::add_vptr(
  const clang::CXXMethodDecl *md,
  struct_typet &type)
{
  /*
   * We model the virtual pointer as a `component` to the parent class' type.
   * This will be the vptr pointing to the vtable that contains the overriden functions.
   *
   * Vptr has the name in the form of `tag-BLAH@vtable_pointer`.
   */

  irep_idt vt_name = vtable_type_prefix + tag_prefix + type.tag().as_string();
  // add a virtual-table pointer
  struct_typet::componentt component;
  component.type() = pointer_typet(symbol_typet(vt_name));
  component.set_name(tag_prefix + type.tag().as_string() + "::@vtable_pointer");
  component.base_name("@vtable_pointer");
  component.pretty_name(type.tag().as_string() + "@vtable_pointer");
  component.set("is_vtptr", true);
  component.set("access", "public");
  // add to the class' type
  type.components().push_back(component);
}

void clang_cpp_convertert::add_vtable_type_entry(
  struct_typet &type,
  struct_typet::componentt &comp,
  symbolt *vtable_type_symbol)
{
  /*
   * When we encounter a virtual or overriding method in a class,
   * need to add an entry to the vtable type symbol.
   * Since the vtable type symbol is modelled as a struct,
   * this entry is considered a `component` in this struct.
   * We model this entry as a function pointer, pointing to the
   * virtual or overriding method in this class.
   *
   * Vtable entry's name is of the form ``virtual_table::tag.BLAH::do_something().
   */

  irep_idt vt_name = vtable_type_prefix + tag_prefix + type.tag().as_string();
  std::string virtual_name = comp.name().as_string();
  struct_typet::componentt vt_entry;
  vt_entry.type() = pointer_typet(comp.type());
  vt_entry.set_name(vt_name.as_string() + "::" + virtual_name);
  vt_entry.set("base_name", virtual_name);
  vt_entry.set("pretty_name", virtual_name);
  vt_entry.set("access", "public");
  vt_entry.location() = comp.location();
  // add an entry to the virtual table
  assert(vtable_type_symbol);
  struct_typet &vtable_type = to_struct_type(vtable_type_symbol->type);
  vtable_type.components().push_back(vt_entry);
}

void clang_cpp_convertert::add_thunk_method(
  const clang::CXXMethodDecl *md,
  const struct_typet::componentt &component,
  struct_typet &type)
{
  /*
   * Add a thunk function for a overriding method.
   * This thunk function will be added as a symbol in the symbol table,
   * and considered a `component` to the derived class' type.
   * This thunk function will be used to set up the derived class' vtable
   * to override the base method, e.g.
   *
   * Suppose Penguin derives Bird, we have the following vtables for Penguin:
   *  virtual_table::Bird@Penguin =
   *    {
   *      .do_it() = &thunk::c:@S@Penguin@F@do_something#::tag-Bird; // this is the thunk redirecting call to the overriding function
   *    };
   *
   *  virtual_table::Penguin@Penguin =
   *    {
   *      .do_it() = &c:@S@Penguin@F@do_something#::do_it(); // this is the overriding function
   *    };
   *
   *  The thunk function's symbol id is of the form - "thunk::c:@S@Penguin@F@do_something#::tag-Bird"
   *  meaning "a thunk to Penguin's overriding function `do_something` taking a `this` parameter of the type Bird*"
   */

  /*
   * For this thunk method, we need to add:
   *  1. its symbol in the symbol table
   *  2. its arguments in the symbol table
   *  3. its body
   *
   *  also need to add this thunk method to the list of components of the derived class' type
   */

  std::string base_class_id = tag_prefix + md->getParent()->getNameAsString();

  // Create the thunk method symbol
  symbolt thunk_func_symb;
  thunk_func_symb.id =
    thunk_prefix + component.get_name().as_string() + "::" + base_class_id;
  thunk_func_symb.name = component.base_name();
  thunk_func_symb.mode = mode;
  thunk_func_symb.location = component.location();
  thunk_func_symb.type = component.type();
  thunk_func_symb.module =
    get_modulename_from_path(component.location().file().as_string());

  // make thunk function names intuitive
  set_thunk_name(thunk_func_symb, base_class_id);

  // update the type of `this` argument in thunk
  update_thunk_this_type(thunk_func_symb.type, base_class_id);

  // add symbols for arguments of this thunk function
  add_thunk_method_arguments(thunk_func_symb);

  // add thunk function body
  add_thunk_method_body(thunk_func_symb, component);

  // add thunk function symbol to the symbol table
  symbolt &added_thunk_symbol = *move_symbol_to_context(thunk_func_symb);

  // add thunk function as a `method` in the derived class' type
  add_thunk_component_to_type(added_thunk_symbol, type, component);
}

void clang_cpp_convertert::set_thunk_name(
  symbolt &thunk_func_symb,
  const std::string &base_class_id)
{
  /*
   * set an intuitive name to thunk function in the form of:
   *  thunk_to::<overriding_func_name>::tag-Base
   * where `Base` represents the base function this overriding function
   * applies to.
   */

  irep_idt thunk_bn = thunk_prefix + "to::" + thunk_func_symb.name.as_string() +
                      "::" + base_class_id;
  thunk_func_symb.name = thunk_bn;
}

void clang_cpp_convertert::update_thunk_this_type(
  typet &thunk_symbol_type,
  const std::string &base_class_id)
{
  /*
   * update the type of `this` argument in thunk function:
   * Before:
   *  * type: pointer
        * subtype: symbol
          * identifier: tag-Derived
     After:
   *  * type: pointer
        * subtype: symbol
          * identifier: tag-Base
    */

  code_typet &code_type = to_code_type(thunk_symbol_type);
  code_typet::argumentt &this_arg = code_type.arguments().front();
  this_arg.type().subtype().set("identifier", base_class_id);
}

void clang_cpp_convertert::add_thunk_method_arguments(symbolt &thunk_func_symb)
{
  /*
   * Loop through the arguments of the thunk methods,
   * and add symbol for each argument. We need to
   * update the identifier field in each argument to indicate it "belongs" to the thunk function.
   *
   * Each argument symbol's id is of the form - "<thunk_func_symbol_ID>::<argument_base_name>"
   */

  code_typet &code_type = to_code_type(thunk_func_symb.type);
  code_typet::argumentst &args = code_type.arguments();
  for(unsigned i = 0; i < args.size(); i++)
  {
    code_typet::argumentt &arg = args[i];
    irep_idt base_name = arg.get_base_name();

    assert(base_name != "");

    symbolt arg_symb;
    arg_symb.id = thunk_func_symb.id.as_string() + "::" + base_name.as_string();
    arg_symb.name = base_name;
    arg_symb.mode = mode;
    arg_symb.location = thunk_func_symb.location;
    arg_symb.type = arg.type();

    // Change argument identifier field to thunk function
    arg.set("#identifier", arg_symb.id);

    // add the argument to the symbol table
    if(context.move(arg_symb))
    {
      log_error(
        "Failed to add arg symbol {} for thunk function {}",
        arg_symb.id,
        thunk_func_symb.id);
      abort();
    }
  }
}

void clang_cpp_convertert::add_thunk_method_body(
  symbolt &thunk_func_symb,
  const struct_typet::componentt &component)
{
  code_typet &code_type = to_code_type(thunk_func_symb.type);
  code_typet::argumentst &args = code_type.arguments();

  /*
   * late cast of `this` pointer to (Derived*)this
   */
  typecast_exprt late_cast_this(
    to_code_type(component.type()).arguments()[0].type());
  late_cast_this.op0() =
    symbol_expr(*namespacet(context).lookup(args[0].cmt_identifier()));

  if(
    code_type.return_type().id() != "empty" &&
    code_type.return_type().id() != "destructor")
    add_thunk_method_body_return(thunk_func_symb, component, late_cast_this);
  else
    add_thunk_method_body_no_return(thunk_func_symb, component, late_cast_this);
}

void clang_cpp_convertert::add_thunk_method_body_return(
  symbolt &thunk_func_symb,
  const struct_typet::componentt &component,
  const typecast_exprt &late_cast_this)
{
  /*
   * Add thunk function with return value, something like:
   * thunk_to_do_something(this): // `this` of the type `Base*`
   *  int return_value;
   *  FUNCTION_CALL: return_value = do_something((Derived*)this);
   *  RETURN: return_value;
   * END_FUNCTION
   */
  code_typet::argumentst &args = to_code_type(thunk_func_symb.type).arguments();

  side_effect_expr_function_callt expr_call;
  expr_call.function() = symbol_exprt(component.get_name(), component.type());
  expr_call.type() = to_code_type(component.type()).return_type();
  expr_call.arguments().reserve(args.size());
  // `this` parameter is always the first one to add
  expr_call.arguments().push_back(late_cast_this);

  for(unsigned i = 1; i < args.size(); i++)
  {
    expr_call.arguments().push_back(
      symbol_expr(*namespacet(context).lookup(args[i].cmt_identifier())));
  }

  code_returnt code_return;
  code_return.return_value() = expr_call;

  thunk_func_symb.value = code_return;
}

void clang_cpp_convertert::add_thunk_method_body_no_return(
  symbolt &thunk_func_symb,
  const struct_typet::componentt &component,
  const typecast_exprt &late_cast_this)
{
  /*
   * Add thunk function without return value, something like:
   * thunk_to_do_something(this): // `this` of the type `Base*`
   *  FUNCTION_CALL: do_something((Derived*)this);
   * END_FUNCTION
   */
  code_typet::argumentst &args = to_code_type(thunk_func_symb.type).arguments();

  code_function_callt code_func;
  code_func.function() = symbol_exprt(component.get_name(), component.type());
  code_func.arguments().reserve(args.size());
  // `this` parameter is always the first one to add
  code_func.arguments().push_back(late_cast_this);

  for(unsigned i = 1; i < args.size(); i++)
  {
    code_func.arguments().push_back(
      symbol_expr(*namespacet(context).lookup(args[i].cmt_identifier())));
  }

  thunk_func_symb.value = code_func;
}

void clang_cpp_convertert::add_thunk_component_to_type(
  const symbolt &thunk_func_symb,
  struct_typet &type,
  const struct_typet::componentt &comp)
{
  struct_typet::componentt new_compo = comp;
  new_compo.type() = thunk_func_symb.type;
  new_compo.set_name(thunk_func_symb.id);
  type.methods().push_back(new_compo);
}