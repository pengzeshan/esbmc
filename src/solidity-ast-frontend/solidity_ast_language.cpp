/*******************************************************************\

Module: Solidity AST module

\*******************************************************************/

#include <solidity-ast-frontend/solidity_ast_language.h>
#include <libUtils/Utils.h>

languaget *new_solidity_ast_language()
{
  return new solidity_ast_languaget;
}

solidity_ast_languaget::solidity_ast_languaget()
{
}

bool solidity_ast_languaget::parse(
  const std::string &path,
  message_handlert &message_handler)
{
    /* store AST text and json contents in stream objects to facilitate parsing and node visiting */
    printf("plaintext ast path: %s\n", plaintext_ast_path.c_str());
    assert(plaintext_ast_path != "");

    std::ifstream ast_text_file_stream(plaintext_ast_path), ast_json_file_stream(path);
    std::stringstream ast_text_stream;
    std::string new_line, sol_name, ast_json_content;

    printf("\n### ast_text_file stream processing:... \n");
    while (getline(ast_text_file_stream, new_line)) {
        printf("new_line: %s\n", new_line.c_str());
        ast_text_stream << new_line << "\n"; // store AST text in stream object
    }

    while (getline(ast_json_file_stream, new_line)) {
        if (new_line.find(".sol =======") != std::string::npos) {
            printf("found .sol ====== , breaking ...\n");
            sol_name = Sif::Utils::substr_by_edge(new_line, "======= ", " =======");
            break;
        }
    }

    printf("\n### ast_json_file_stream processing:... \n");
    while (getline(ast_json_file_stream, new_line)) { // file pointer continues from "=== *.sol ==="
        //printf("new_line: %s\n", new_line.c_str());
        if (new_line.find(".sol =======") == std::string::npos) {
            ast_json_content = ast_json_content + new_line + "\n";
        }
        else {
            assert(!"Unsupported feature: found multiple contracts defined in a single .sol file");
        }
    }
    printf("\n ### ast_json_content: ###");
    printf("%s", ast_json_content.c_str());
    printf("\n");

    assert("cool");

    return false;
}

bool solidity_ast_languaget::typecheck(
  contextt &context,
  const std::string &module,
  message_handlert &message_handler)
{
  assert(!"come back and continue - solidity_ast_languaget::typecheck");
  return false;
}

void solidity_ast_languaget::show_parse(std::ostream &)
{
  assert(!"come back and continue - solidity_ast_languaget::show_parse");
}

bool solidity_ast_languaget::final(
  contextt &context,
  message_handlert &message_handler)
{
  assert(!"come back and continue - solidity_ast_languaget::final");
  return false;
}

bool solidity_ast_languaget::from_expr(
  const exprt &expr,
  std::string &code,
  const namespacet &ns)
{
  assert(!"come back and continue - solidity_ast_languaget::from_expr");
  return false;
}

bool solidity_ast_languaget::from_type(
  const typet &type,
  std::string &code,
  const namespacet &ns)
{
  assert(!"come back and continue - solidity_ast_languaget::from_type");
  return false;
}