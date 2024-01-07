#include "backend_c.h"


void translate_to_c(struct ast_node *ast, string_builder *output, int depth)
{
    char spaces[] = "                                                 ";

    switch (ast->kind)
    {
        case AST__BLOCK:
        {
            string_builder__append_format(output, "{\n");
            translate_to_c(ast->block.statements, output, depth + 1);
            string_builder__append_format(output, "%.*s}\n", depth * 4, spaces);
        }
        break;

        case AST__STATEMENT:
        {
            string_builder__append_format(output, "%.*s", depth * 4, spaces);
            translate_to_c(ast->statement.stmt, output, depth);
            if (ast->statement.stmt->kind != AST__BLOCK)
                string_builder__append_string(output, ";\n");
            if (ast->statement.next != NULL)
            {
                translate_to_c(ast->statement.next, output, depth);
            }
        }
        break;

        case AST__DECLARATION:
        {
            string_builder__append_format(output, "int %.*s", (int) ast->declaration.name.size, ast->declaration.name.data);
            if (ast->declaration.init != NULL)
            {
                string_builder__append_string(output, " = ");
                translate_to_c(ast->declaration.init, output, depth);
            }
        }
        break;

        case AST__BINARY_OPERATOR:
        {
            translate_to_c(ast->binary_operator.lhs, output, depth);
            string_builder__append_format(output, "%c", ast->binary_operator.operator);
            translate_to_c(ast->binary_operator.rhs, output, depth);
        }
        break;

        case AST__LITERAL_INT:
        {
            string_builder__append_format(output, "%lld", ast->literal_int.value);
        }
        break;

        case AST__VARIABLE:
        {
            string_builder__append_format(output, "%.*s", (int) ast->variable.name.size, ast->variable.name.data);
        }
        break;

        case AST__FUNCTION_CALL:
        {
            string_builder__append_format(output, "%.*s(", (int) ast->function_call.name.size, ast->function_call.name.data);
            translate_to_c(ast->function_call.arg1, output, depth);
            string_builder__append_string(output, ")");
        }
        break;

        case AST__TYPE:
            string_builder__append_string(output, "int");
        break;

        default:
        break;
    }
}
