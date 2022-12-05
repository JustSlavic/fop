#include "parser.h"


char *token_type_to_cstring(token_type t)
{
    switch (t)
    {
        case TOKEN_INVALID: return "TOKEN_INVALID";
        case TOKEN_PAREN_OPEN: return "TOKEN_PAREN_OPEN";
        case TOKEN_PAREN_CLOSE: return "TOKEN_PAREN_CLOSE";
        case TOKEN_BRACKET_OPEN: return "TOKEN_BRACKET_OPEN";
        case TOKEN_BRACKET_CLOSE: return "TOKEN_BRACKET_CLOSE";
        case TOKEN_BRACE_OPEN: return "TOKEN_BRACE_OPEN";
        case TOKEN_BRACE_CLOSE: return "TOKEN_BRACE_CLOSE";
        case TOKEN_EQUALS: return "TOKEN_EQUALS";
        case TOKEN_COLON: return "TOKEN_COLON";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_LITERAL_INT: return "TOKEN_LITERAL_INT";
        case TOKEN_ARROW_RIGHT: return "TOKEN_ARROW_RIGHT";
        case TOKEN_KW_RETURN: return "TOKEN_KW_RETURN";
        case TOKEN_EOF: return "TOKEN_EOF";
        
        default:
            return "<token_type>";
    }
    return 0;
}

b32 is_ascii_space(char c)
{
    return (c == ' ');
}

b32 is_ascii_whitespace(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

b32 is_ascii_alpha(char c)
{
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

b32 is_ascii_digit(char c)
{
    return (c >= '0') && (c <= '9');
}

b32 is_valid_identifier_head(char c)
{
    return (c == '_') || is_ascii_alpha(c);
}

b32 is_valid_identifier_body(char c)
{
    return (c == '_') || is_ascii_alpha(c) || is_ascii_digit(c);
}


char lexer_get_char(lexer *l)
{
    char c = 0;
    if (l->index < l->buffer_size)
    {
        c = l->buffer[l->index];
    }
    return c;
}

char lexer_eat_char(lexer *l)
{
    char c = lexer_get_char(l);
    l->index += 1;
    if (c == '\n')
    {
        l->line += 1;
        l->column = 0;
    }
    l->column += 1;
    return c;
}

void consume_while(lexer *l, predicate *p)
{
    char c = lexer_get_char(l);
    while (p(c))
    {
        lexer_eat_char(l);
        c = lexer_get_char(l);
    }
}

void consume_until(lexer *l, predicate *p)
{
    char c = lexer_get_char(l);
    while (!p(c))
    {
        lexer_eat_char(l);
        c = lexer_get_char(l);
    }
}

token lexer_get_token(lexer *l)
{
    if (!l->next_token_valid)
    {
        consume_while(l, is_ascii_whitespace);
       
        token t = {0};

        char c = lexer_get_char(l);
        if (c == 0)
        {
            t.type = TOKEN_EOF;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;
            t.span_size = 1;
        }
        else if (is_valid_identifier_head(c))
        {
            t.type = TOKEN_IDENTIFIER;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;

            lexer_eat_char(l);
            c = lexer_get_char(l);
            int span_size = 1;

            while (is_valid_identifier_body(c))
            {
                lexer_eat_char(l);
                c = lexer_get_char(l);
                span_size += 1;
            }

            t.span_size = span_size;
        }
        else if (is_ascii_digit(c))
        {
            t.type = TOKEN_LITERAL_INT;
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;
            t.span_size = 0;

            int integer = 0;

            while (is_ascii_digit(c))
            {
                integer *= 10;
                integer += (c - '0');
                lexer_eat_char(l);
                c = lexer_get_char(l);
                t.span_size += 1;
            }

            t.integer_value = integer;
        }
        else
        {
            t.line = l->line;
            t.column = l->column;
            t.span = l->buffer + l->index;

            if (c == '-')
            {
                lexer_eat_char(l);
                c = lexer_get_char(l);
                if (c == '>')
                {
                    t.type = TOKEN_ARROW_RIGHT;
                    lexer_eat_char(l);
                    t.span_size = 2;
                }
                else
                {
                    t.type = TOKEN_MINUS;
                    t.span_size = 1;
                }
            }
            else
            {
                lexer_eat_char(l);
                t.type = (token_type) c;
                t.span_size = 1;
            }
        }

        l->next_token = t;
        l->next_token_valid = true;
    }

    return l->next_token;
}

token lexer_eat_token(lexer *l)
{
    token result = lexer_get_token(l);
    l->next_token_valid = false;
    return result;
}

int parser_get_operator_precedence(token t)
{
    switch (t.type)
    {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
            return 2;
        default:
            return 0;
    }
    return 0;
}

ast_node *pinapl_parse_expression_operand(allocator *a, lexer *l)
{
    ast_node *result = NULL;

    token t = lexer_get_token(l);
    if (t.type == TOKEN_IDENTIFIER)
    {
        lexer_eat_token(l);

        token paren_open = lexer_get_token(l);
        if (paren_open.type == '(')
        {
            lexer_eat_token(l);

            // @todo: parse argument_list

            token s = lexer_get_token(l);
            if (s.type == ')')
            {
                lexer_eat_token(l);

                result = ALLOCATE(a, ast_node);
                result->type = AST_NODE_FUNCTION_CALL;
                result->function_name = t;
                result->argument_list = NULL;
            }
        }
        else
        {
            result = ALLOCATE(a, ast_node);
            result->type = AST_NODE_VARIABLE;
            result->var_span = t.span;
            result->var_span_size = t.span_size;
        }
    }
    else if (t.type == TOKEN_LITERAL_INT)
    {
        lexer_eat_token(l);

        result = ALLOCATE(a, ast_node);
        result->type = AST_NODE_LITERAL_INT;
        result->literal_span = t.span;
        result->literal_span_size = t.span_size;
        result->integer_value = t.integer_value;
    }

    return result;
}

ast_node *pinapl_parse_expression(allocator *a, lexer *l, int precedence)
{
    ast_node *left_operand = NULL;

    token open_paren = lexer_get_token(l);
    if (open_paren.type == '(')
    {
        lexer_eat_token(l);
        left_operand = pinapl_parse_expression(a, l, precedence);
        if (left_operand == NULL)
        {
            return NULL;
        }

        token close_paren = lexer_get_token(l);
        if (close_paren.type == ')')
        {
            lexer_eat_token(l);
        }
        else
        {
            // @leak left_operand
            return NULL;
        }
    }
    else
    {
        left_operand = pinapl_parse_expression_operand(a, l);
        if (left_operand == NULL)
        {
            return NULL;
        }
    }

    while (true)
    {
        token operator = lexer_get_token(l);
        if ((operator.type != '+') &&
            (operator.type != '-') &&
            (operator.type != '*') &&
            (operator.type != '/') &&
            (operator.type != '='))
        {
            break;
        }

        int operator_precedence = parser_get_operator_precedence(operator);
        if (operator_precedence < precedence) break;

        lexer_eat_token(l);
        
        // +1 for left associativity, +0 for right ass
        ast_node *right_operand = pinapl_parse_expression(a, l, operator_precedence + 1);
        if (right_operand == NULL)
        {
            return NULL;
        }

        ast_node *binary_operator = ALLOCATE(a, ast_node);
        binary_operator->type = AST_NODE_BINARY_OPERATOR;
        binary_operator->op   = operator;
        binary_operator->lhs  = left_operand;
        binary_operator->rhs  = right_operand;

        left_operand = binary_operator;
    }

    return left_operand;
}

ast_node *pinapl_parse_variable_declaration(allocator *a, lexer *l)
{
    //
    // x :: <expr>;
    // x := <expr>;
    // x : int;
    // x : int : <expr>;
    // x : int = <expr>;
    //

    ast_node *result = NULL;

    token var_name = lexer_eat_token(l);
    if (var_name.type == TOKEN_IDENTIFIER)
    {
        token colon = lexer_eat_token(l);
        if (colon.type == ':')
        {
            token type = {0};
            b32 is_constant = false;
            b32 should_init = true;

            token t = lexer_get_token(l);
            if (t.type == '=')
            {
                //
                // <ident> := <expr>
                //
                lexer_eat_token(l);
                is_constant = false;
            }
            else if (t.type == ':')
            {
                //
                // <ident> :: <expr>
                //
                lexer_eat_token(l);
                is_constant = true;
            }
            else if (t.type == TOKEN_IDENTIFIER)
            {
                lexer_eat_token(l);
                if ((t.span_size == 3) &&
                    (t.span[0] == 'i') &&
                    (t.span[1] == 'n') &&
                    (t.span[2] == 't'))
                {
                    token t2 = lexer_get_token(l);
                    if (t2.type == '=')
                    {
                        //
                        // <ident> : int = <expr>
                        //
                        lexer_eat_token(l);
                        type = t;
                        is_constant = false;
                    }
                    else if (t2.type == ':')
                    {
                        //
                        // <ident> : int : <expr>
                        //
                        lexer_eat_token(l);
                        type = t;
                        is_constant = true;
                    }
                    else if (t2.type == ';')
                    {
                        // x : int
                        type = t;
                        is_constant = false;
                        should_init = false;

                        result = ALLOCATE(a, ast_node);
                        result->type = AST_NODE_VARIABLE_DECLARATION;
                        result->var_name = var_name;
                        result->var_type = type;
                        result->is_constant = false;
                        result->init = NULL;
                    }
                    else
                    {
                        // error: expected ':', '=', or ';'
                    }
                }
                else
                {
                    // error: I do not allow other types other than 'int' YET
                }
            }
            else
            {
                should_init = false;
                // error: expected '-', ':', or a type
            }

            if (should_init)
            {
                lexer checkpoint = *l;
                ast_node *initializer = pinapl_parse_expression(a, l, 0);
                if (!initializer)
                {
                    *l = checkpoint;
                    initializer = pinapl_parse_function_definition(a, l);
                }

                if (initializer)
                {
                    result = ALLOCATE(a, ast_node);
                    result->type = is_constant 
                        ? AST_NODE_CONSTANT_DECLARATION
                        : AST_NODE_VARIABLE_DECLARATION;
                    result->var_name = var_name;
                    result->var_type = type;
                    result->is_constant = is_constant;
                    result->init = initializer;
                }
                else
                {
                    // error: expression expected (probably should print error, that comes from that call
                }
            }
        }
        else
        {
            // error: token ':' expected
        }
    }
    else
    {
        // error: variable declaration is not started with identifier
    }

    return result;
}

ast_node *pinapl_parse_block(allocator *a, lexer *l)
{
    ast_node *result = NULL;
    //
    // block ::= { <statement-list> }
    //
    token open_brace = lexer_eat_token(l);
    if (open_brace.type == '{')
    {
        lexer checkpoint = *l;
        ast_node *statement_list = pinapl_parse_statement_list(a, l);
        if (!statement_list)
        {
            *l = checkpoint;
        }
        
        token close_brace = lexer_eat_token(l);
        if (close_brace.type == '}')
        {
            result = ALLOCATE(a, ast_node);
            result->type = AST_NODE_BLOCK;
            result->statement_list = statement_list;
        }
        else
        {
            // error: expected '}'
        }
    }
    else
    {
        // error: expected '{'
    }

    return result;
}

ast_node *pinapl_parse_function_definition(allocator *a, lexer *l)
{
    //
    // function-definition ::= 
    //   (<argument-list>) <block>
    //   (<argument-list>) -> <return-type> <block>
    //

    ast_node *result = NULL;

    token open_paren = lexer_eat_token(l);
    if (open_paren.type == '(')
    {
        // @todo: argument parsing here

        token close_paren = lexer_eat_token(l);
        if (close_paren.type == ')')
        {
            ast_node *block = pinapl_parse_block(a, l);
            if (block)
            {
                result = ALLOCATE(a, ast_node);
                result->type = AST_NODE_FUNCTION_DEFINITION;
                result->parameter_list = NULL; 
                result->return_type = NULL;
                result->block = block; 
            }
            else
            {
                // Do I allow forward-declarations for functions in pinapl ?
            }
        }
        else
        {
            // Error: expected ')'
        }
    }
    else
    {
        // Error: expected '('
    }

    return result;
}

ast_node *pinapl_parse_statement(allocator *a, lexer *l)
{
    ast_node *result = NULL;
    lexer checkpoint = *l;

    ast_node *variable_declaration = pinapl_parse_variable_declaration(a, l);
    if (variable_declaration)
    {
        result = variable_declaration;
    }
    else
    {
        *l = checkpoint;
        ast_node *expression = pinapl_parse_expression(a, l, 0);
        if (expression)
        {
            result = expression;
        }
        else
        {
            // @error!!! I don't know what it is!
        }
    }

    token semicolon = lexer_get_token(l);
    if (semicolon.type == ';')
    {
        lexer_eat_token(l);
    }
    else
    {
        // Error! statement should end with a semicolon!
        // @leak
        result = NULL;
    }

    return result;
}

ast_node *pinapl_parse_statement_list(allocator *a, lexer *l)
{
    ast_node *result = NULL;

    ast_node *first_statement = pinapl_parse_statement(a, l);
    if (first_statement)
    {
        result = ALLOCATE(a, ast_node);
        result->type = AST_NODE_STATEMENT_LIST;
        result->statement = first_statement;
        result->next_statement = NULL;

        ast_node *last_list_node = result;
        
        while (true)
        {
            ast_node *statement = pinapl_parse_statement(a, l);
            if (statement)
            {
                ast_node *new_list_node = ALLOCATE(a, ast_node);
                new_list_node->type = AST_NODE_STATEMENT_LIST;
                new_list_node->statement = statement;
                new_list_node->next_statement = NULL;

                last_list_node->next_statement = new_list_node;
                last_list_node = new_list_node;
            }
            else
            {
                break;
            }
        }
    }

    return result;
}

ast_node *pinapl_parse_global_declaration(allocator *a, lexer *l)
{
    ast_node *result = pinapl_parse_variable_declaration(a, l);
    return result;
}

ast_node *pinapl_parse_global_declaration_list(allocator *a, lexer *l)
{
    ast_node *result = NULL;

    ast_node *first_declaration = pinapl_parse_global_declaration(a, l);
    if (first_declaration)
    {
        result = ALLOCATE(a, ast_node);
        result->type = AST_NODE_GLOBAL_DECLARATION_LIST;
        result->declaration = first_declaration;
        result->next_declaration = NULL;

        ast_node *last_list_node = result;
        while (true)
        {
            ast_node *declaration = pinapl_parse_global_declaration(a, l);
            if (declaration)
            {
                ast_node *next_list_node = ALLOCATE(a, ast_node);
                next_list_node->type = AST_NODE_GLOBAL_DECLARATION_LIST;
                next_list_node->declaration = declaration;
                next_list_node->next_declaration = NULL;

                last_list_node->next_declaration = next_list_node;
                last_list_node = next_list_node;
            }
            else
            {
                break;
            }
        }
    }

    return result;

}

