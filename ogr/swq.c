/******************************************************************************
 *
 * Component: OGDI Driver Support Library
 * Purpose: Generic SQL WHERE Expression Implementation.
 * Author: Frank Warmerdam <warmerdam@pobox.com>
 * 
 ******************************************************************************
 * Copyright (C) 2001 Information Interoperability Institute (3i)
 * Permission to use, copy, modify and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies, that
 * both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of 3i not be used 
 * in advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  3i makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.8  2002/04/25 16:06:57  warmerda
 * added more general distinct support
 *
 * Revision 1.7  2002/04/25 02:23:43  warmerda
 * fixed support for ORDER BY <field> ASC
 *
 * Revision 1.6  2002/04/23 20:05:23  warmerda
 * added SELECT statement parsing
 *
 * Revision 1.5  2002/04/19 20:46:06  warmerda
 * added [NOT] IN, [NOT] LIKE and IS [NOT] NULL support
 *
 * Revision 1.4  2002/03/01 04:13:40  warmerda
 * Made swq_error static.
 *
 * Revision 1.3  2001/11/07 12:45:42  danmo
 * Use #ifdef _WIN32 instead of WIN32 for strcasecmp check
 *
 * Revision 1.2  2001/06/26 00:59:39  warmerda
 * fixed strcasecmp on WIN32
 *
 * Revision 1.1  2001/06/19 15:46:30  warmerda
 * New
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "swq.h"

#ifndef SWQ_MALLOC
#define SWQ_MALLOC(x) malloc(x)
#define SWQ_FREE(x) free(x)
#endif

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#ifdef _WIN32
#  define strcasecmp stricmp
#endif

static char	swq_error[1024];

#define SWQ_OP_IS_LOGICAL(op) ((op) == SWQ_OR || (op) == SWQ_AND || (op) == SWQ_NOT)
#define SWQ_OP_IS_POSTUNARY(op) ((op) == SWQ_ISNULL || (op) == SWQ_ISNOTNULL)

/************************************************************************/
/*                           swq_isalphanum()                           */
/*                                                                      */
/*      Is the passed character in the set of things that could         */
/*      occur in an alphanumeric token, or a number?                    */
/************************************************************************/

static int swq_isalphanum( char c )

{

    if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-'
        || c == '_' )
        return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                             swq_token()                              */
/************************************************************************/

static char *swq_token( const char *expression, char **next, int *is_literal )

{
    char	*token;
    int		i_token;

    if( is_literal != NULL )
        *is_literal = 0;

    while( *expression == ' ' || *expression == '\t' )
        expression++;

    if( *expression == '\0' )
    {
        *next = (char *) expression;
        return NULL; 
    }

/* -------------------------------------------------------------------- */
/*      Handle string constants.                                        */
/* -------------------------------------------------------------------- */
    if( *expression == '"' )
    {
        expression++;

        token = (char *) SWQ_MALLOC(strlen(expression)+1);
        i_token = 0;

        while( *expression != '\0' )
        {
            if( *expression == '\\' && expression[1] == '"' )
                expression++;
            else if( *expression == '"' )
            {
                expression++;
                break;
            }
            
            token[i_token++] = *(expression++);
        }
        token[i_token] = '\0';

        if( is_literal != NULL )
            *is_literal = 1;
    }

/* -------------------------------------------------------------------- */
/*      Handle alpha-numerics.                                          */
/* -------------------------------------------------------------------- */
    else if( swq_isalphanum( *expression ) )
    {
        token = (char *) SWQ_MALLOC(strlen(expression)+1);
        i_token = 0;

        while( swq_isalphanum( *expression ) )
        {
            token[i_token++] = *(expression++);
        }

        token[i_token] = '\0';
    }

/* -------------------------------------------------------------------- */
/*      Handle special tokens.                                          */
/* -------------------------------------------------------------------- */
    else
    {
        token = (char *) SWQ_MALLOC(3);
        token[0] = *expression;
        token[1] = '\0';
        expression++;

        /* special logic to group stuff like '>=' into one token. */

        if( (*token == '<' || *token == '>' || *token == '=' || *token == '!')
           && (*expression == '<' || *expression == '>' || *expression == '='))
        {
            token[1] = *expression;
            token[2] = '\0';
            expression++;
        }
    }

    *next = (char *) expression;

    return token;
}

/************************************************************************/
/*                             swq_strdup()                             */
/************************************************************************/

static char *swq_strdup( const char *input )

{
    char *result;

    result = (char *) SWQ_MALLOC(strlen(input)+1);
    strcpy( result, input );

    return result;
}

/************************************************************************/
/* ==================================================================== */
/*		WHERE clause parsing                                    */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           swq_test_like()                            */
/*                                                                      */
/*      Does input match pattern?                                       */
/************************************************************************/

int swq_test_like( const char *input, const char *pattern )

{
    if( input == NULL || pattern == NULL )
        return 0;

    while( *input != '\0' )
    {
        if( *pattern == '\0' )
            return 0;

        else if( *pattern == '_' )
        {
            input++;
            pattern++;
        }
        else if( *pattern == '%' )
        {
            int   eat;

            if( pattern[1] == '\0' )
                return 1;

            /* try eating varying amounts of the input till we get a positive*/
            for( eat = 0; input[eat] != '\0'; eat++ )
            {
                if( swq_test_like(input+eat,pattern+1) )
                    return 1;
            }

            return 0;
        }
        else
        {
            if( tolower(*pattern) != tolower(*input) )
                return 0;
            else
            {
                input++;
                pattern++;
            }
        }
    }

    if( *pattern != '\0' && strcmp(pattern,"%") != 0 )
        return 0;
    else
        return 1;
}

/************************************************************************/
/*                          swq_identify_op()                           */
/************************************************************************/

static swq_op swq_identify_op( char **tokens, int *tokens_consumed )

{
    const char *token = tokens[*tokens_consumed];

    if( strcasecmp(token,"OR") == 0 )
        return SWQ_OR;
    
    if( strcasecmp(token,"AND") == 0 )
        return SWQ_AND;
    
    if( strcasecmp(token,"NOT") == 0 )
    {
        if( tokens[*tokens_consumed+1] != NULL
            && strcasecmp(tokens[*tokens_consumed+1],"LIKE") == 0 )
        {
            *tokens_consumed += 1;
            return SWQ_NOTLIKE;
        }
        else if( tokens[*tokens_consumed+1] != NULL
            && strcasecmp(tokens[*tokens_consumed+1],"IN") == 0 )
        {
            *tokens_consumed += 1;
            return SWQ_NOTIN;
        }
        else
            return SWQ_NOT;
    }
    
    if( strcasecmp(token,"<=") == 0 )
        return SWQ_LE;

    if( strcasecmp(token,">=") == 0 )
        return SWQ_GE;
    
    if( strcasecmp(token,"=") == 0 )
        return SWQ_EQ;
    
    if( strcasecmp(token,"!=") == 0 )
        return SWQ_NE;
    
    if( strcasecmp(token,"<>") == 0 )
        return SWQ_NE;
    
    if( strcasecmp(token,"<") == 0 )
        return SWQ_LT;
    
    if( strcasecmp(token,">") == 0 )
        return SWQ_GT;

    if( strcasecmp(token,"LIKE") == 0 )
        return SWQ_LIKE;

    if( strcasecmp(token,"IN") == 0 )
        return SWQ_IN;

    if( strcasecmp(token,"IS") == 0 )
    {
        if( tokens[*tokens_consumed+1] == NULL )
            return SWQ_UNKNOWN;
        else if( strcasecmp(tokens[*tokens_consumed+1],"NULL") == 0 )
        {
            *tokens_consumed += 1;
            return SWQ_ISNULL;
        }
        else if( strcasecmp(tokens[*tokens_consumed+1],"NOT") == 0
                 && tokens[*tokens_consumed+2] != NULL
                 && strcasecmp(tokens[*tokens_consumed+2],"NULL") == 0 )
        {
            *tokens_consumed += 2;
            return SWQ_ISNOTNULL;
        }
        else 
            return SWQ_UNKNOWN;
    }

    return SWQ_UNKNOWN;
}

/************************************************************************/
/*                         swq_identify_field()                         */
/************************************************************************/

static int swq_identify_field( const char *token,
                               int field_count,
                               char **field_list, 
                               swq_field_type *field_types, 
                               swq_field_type *this_type )

{
    int	i;

    for( i = 0; i < field_count; i++ )
    {
        if( strcasecmp( field_list[i], token ) == 0 )
        {
            if( this_type != NULL )
            {
                if( field_types != NULL )
                    *this_type = field_types[i];
                else
                    *this_type = SWQ_OTHER;
            }
            
            return i;
        }
    }

    if( this_type != NULL )
        *this_type = SWQ_OTHER;

    return -1;
}

/************************************************************************/
/*                         swq_parse_in_list()                          */
/*                                                                      */
/*      Parse the argument list to the IN predicate. Might be used      */
/*      something like:                                                 */
/*                                                                      */
/*        WHERE color IN ('Red', 'Green', 'Blue')                       */
/************************************************************************/

static char *swq_parse_in_list( char **tokens, int *tokens_consumed )

{
    int   i, text_off = 2;
    char *result;
    
    if( tokens[*tokens_consumed] == NULL
        || strcasecmp(tokens[*tokens_consumed],"(") != 0 )
    {
        sprintf( swq_error, "IN argument doesn't start with '('." );
        return NULL;
    }

    *tokens_consumed += 1;

    /* Establish length of all tokens plus separators. */

    for( i = *tokens_consumed; 
         tokens[i] != NULL && strcasecmp(tokens[i],")") != 0; 
         i++ )
    {
        text_off += strlen(tokens[i]) + 1;
    }
    
    result = (char *) SWQ_MALLOC(text_off);

    /* Actually capture all the arguments. */

    text_off = 0;
    while( tokens[*tokens_consumed] != NULL 
           && strcasecmp(tokens[*tokens_consumed],")") != 0 )
    {
        strcpy( result + text_off, tokens[*tokens_consumed] );
        text_off += strlen(tokens[*tokens_consumed]) + 1;

        *tokens_consumed += 1;

        if( strcasecmp(tokens[*tokens_consumed],",") != 0
            && strcasecmp(tokens[*tokens_consumed],")") != 0 )
        {
            sprintf( swq_error, 
               "Contents of IN predicate missing comma or closing bracket." );
            SWQ_FREE( result );
            return NULL;
        }
        else if( strcasecmp(tokens[*tokens_consumed],",") == 0 )
            *tokens_consumed += 1;
    }

    /* add final extra terminating zero char */
    result[text_off] = '\0';

    if( tokens[*tokens_consumed] == NULL )
    {
        sprintf( swq_error, 
                 "Contents of IN predicate missing closing bracket." );
        SWQ_FREE( result );
        return NULL;
    }

    *tokens_consumed += 1;

    return result;
}

/************************************************************************/
/*                        swq_subexpr_compile()                         */
/************************************************************************/

static const char *
swq_subexpr_compile( char **tokens,
                     int field_count,
                     char **field_list, 
                     swq_field_type *field_types, 
                     swq_expr **expr_out,
                     int *tokens_consumed )

{
    swq_expr	*op;
    const char  *error;
    int         op_code;

    *tokens_consumed = 0;
    *expr_out = NULL;

    if( tokens[0] == NULL || tokens[1] == NULL )
    {
        sprintf( swq_error, "Not enough tokens to complete expression." );
        return swq_error;
    }
    
    op = (swq_field_op *) SWQ_MALLOC(sizeof(swq_field_op));
    memset( op, 0, sizeof(swq_field_op) );
    op->field_index = -1;

    if( strcmp(tokens[0],"(") == 0 )
    {
        int	sub_consumed = 0;

        error = swq_subexpr_compile( tokens + 1, field_count, field_list, 
                                     field_types, 
                                     (swq_expr **) &(op->first_sub_expr), 
                                     &sub_consumed );
        if( error != NULL )
        {
            swq_expr_free( op );
            return error;
        }

        if( strcmp(tokens[sub_consumed+1],")") != 0 )
        {
            swq_expr_free( op );
            sprintf(swq_error,"Unclosed brackets, or incomplete expression.");
            return swq_error;
        }

        *tokens_consumed += sub_consumed + 2;

        /* If we are at the end of the tokens, we should return our subnode */
        if( tokens[*tokens_consumed] == NULL
            || strcmp(tokens[*tokens_consumed],")") == 0 )
        {
            *expr_out = (swq_expr *) op->first_sub_expr;
            op->first_sub_expr = NULL;
            swq_expr_free( op );
            return NULL;
        }
    }
    else if( strcasecmp(tokens[0],"NOT") == 0 )
    {
        /* do nothing, the NOT will be collected as the operation */
    }
    else
    {
        op->field_index = 
            swq_identify_field( tokens[*tokens_consumed], 
                                field_count, field_list, field_types, 
                                &(op->field_type) );

        if( op->field_index < 0 )
        {
            swq_expr_free( op );
            sprintf( swq_error, "Failed to identify field:" );
            strncat( swq_error, tokens[*tokens_consumed], 
                     sizeof(swq_error) - strlen(swq_error) - 1 );
            return swq_error;
        }

        (*tokens_consumed)++;
    }

    /*
    ** Identify the operation.
    */
    if( tokens[*tokens_consumed] == NULL || tokens[*tokens_consumed+1] == NULL)
    {
        sprintf( swq_error, "Not enough tokens to complete expression." );
        return swq_error;
    }
    
    op->operation = swq_identify_op( tokens, tokens_consumed );
    if( op->operation == SWQ_UNKNOWN )
    {
        swq_expr_free( op );
        sprintf( swq_error, "Failed to identify operation:" );
        strncat( swq_error, tokens[*tokens_consumed], 
                 sizeof(swq_error) - strlen(swq_error) - 1 );
        return swq_error;
    }

    if( SWQ_OP_IS_LOGICAL( op->operation ) 
        && op->first_sub_expr == NULL 
        && op->operation != SWQ_NOT )
    {
        swq_expr_free( op );
        strcpy( swq_error, "Used logical operation with non-logical operand.");
        return swq_error;
    }

    if( op->field_index != -1 && op->field_type == SWQ_STRING
        && (op->operation != SWQ_EQ && op->operation != SWQ_NE
            && op->operation != SWQ_LIKE && op->operation != SWQ_NOTLIKE
            && op->operation != SWQ_IN && op->operation != SWQ_NOTIN
            && op->operation != SWQ_ISNULL && op->operation != SWQ_ISNOTNULL ))
    {
        sprintf( swq_error, 
            "Attempt to use STRING field `%s' with numeric comparison `%s'.",
            field_list[op->field_index], tokens[*tokens_consumed] );
        swq_expr_free( op );
        return swq_error;
    }

    (*tokens_consumed)++;

    /*
    ** Collect the second operand as a subexpression.
    */
    
    if( SWQ_OP_IS_POSTUNARY(op->operation) )
    {
        /* we don't need another argument. */
    }

    else if( tokens[*tokens_consumed] == NULL )
    {
        sprintf( swq_error, "Not enough tokens to complete expression." );
        return swq_error;
    }
    
    else if( SWQ_OP_IS_LOGICAL( op->operation ) )
    {
        int	sub_consumed = 0;

        error = swq_subexpr_compile( tokens + *tokens_consumed, 
                                     field_count, field_list, field_types, 
                                     (swq_expr **) &(op->second_sub_expr), 
                                     &sub_consumed );
        if( error != NULL )
        {
            swq_expr_free( op );
            return error;
        }

        *tokens_consumed += sub_consumed;
    }

    /* The IN predicate has a complex argument syntax. */
    else if( op->operation == SWQ_IN || op->operation == SWQ_NOTIN )
    {
        op->string_value = swq_parse_in_list( tokens, tokens_consumed );
        if( op->string_value == NULL )
        {
            swq_expr_free( op );
            return swq_error;
        }
    }

    /*
    ** Otherwise collect it as a literal value.
    */
    else
    {
        op->string_value = swq_strdup(tokens[*tokens_consumed]);
        op->int_value = atoi(op->string_value);
        op->float_value = atof(op->string_value);
        
        if( op->field_index != -1 
            && (op->field_type == SWQ_INTEGER || op->field_type == SWQ_FLOAT) 
            && op->string_value[0] != '-'
            && op->string_value[0] != '+'
            && op->string_value[0] != '.'
            && (op->string_value[0] < '0' || op->string_value[0] > '9') )
        {
            sprintf( swq_error, 
                     "Attempt to compare numeric field `%s' to non-numeric"
                     " value `%s' is illegal.", 
                     field_list[op->field_index], op->string_value );
            swq_expr_free( op );
            return swq_error;
        }

        (*tokens_consumed)++;
    }

    *expr_out = op;

    /* Transform stuff like A NOT LIKE X into NOT (A LIKE X) */
    if( op->operation == SWQ_NOTLIKE
        || op->operation == SWQ_ISNOTNULL 
        || op->operation == SWQ_NOTIN )
    {
        if( op->operation == SWQ_NOTLIKE )
            op->operation = SWQ_LIKE;
        else if( op->operation == SWQ_NOTIN )
            op->operation = SWQ_IN;
        else if( op->operation == SWQ_ISNOTNULL )
            op->operation = SWQ_ISNULL;

        op = (swq_field_op *) SWQ_MALLOC(sizeof(swq_field_op));
        memset( op, 0, sizeof(swq_field_op) );
        op->field_index = -1;
        op->second_sub_expr = (struct swq_node_s *) *expr_out;
        op->operation = SWQ_NOT;

        *expr_out = op;
    }

    op = NULL;
    
    /*
    ** Are we part of an unparantized logical expression chain?  If so, 
    ** grab the remainder of the expression at "this level" and add to the
    ** local tree. 
    */
    if( tokens[*tokens_consumed] != NULL
        && (op_code == swq_identify_op( tokens, tokens_consumed ))
        && SWQ_OP_IS_LOGICAL(op_code) )
    {
        swq_expr *remainder = NULL;
        swq_expr *parent;
        int	 sub_consumed;

        error = swq_subexpr_compile( tokens + *tokens_consumed + 1, 
                                     field_count, field_list, field_types, 
                                     &remainder, &sub_consumed );
        if( error != NULL )
        {
            swq_expr_free( *expr_out );
            *expr_out = NULL;
            return error;
        }

        parent = (swq_field_op *) SWQ_MALLOC(sizeof(swq_field_op));
        memset( parent, 0, sizeof(swq_field_op) );
        parent->field_index = -1;

        parent->first_sub_expr = (struct swq_node_s *) *expr_out;
        parent->second_sub_expr = (struct swq_node_s *) remainder;
        parent->operation = op_code;

        *expr_out = parent;

        *tokens_consumed += sub_consumed + 1;
    }

    return NULL;
}

/************************************************************************/
/*                          swq_expr_compile()                          */
/************************************************************************/

const char *swq_expr_compile( const char *where_clause, 
                              int field_count,
                              char **field_list, 
                              swq_field_type *field_types, 
                              swq_expr **expr_out )

{
#define MAX_TOKEN 1024
    char	*token_list[MAX_TOKEN], *rest_of_expr;
    int		token_count = 0;
    int		tokens_consumed, i;
    const char *error;
    
    /*
    ** Collect token array.
    */
    rest_of_expr = (char *) where_clause;
    while( token_count < MAX_TOKEN )
    {
        token_list[token_count] = swq_token(rest_of_expr,&rest_of_expr,NULL);
        if( token_list[token_count] == NULL )
            break;

        token_count++;
    }
    token_list[token_count] = NULL;
    
    /*
    ** Parse the expression.
    */
    *expr_out = NULL;
    error = 
        swq_subexpr_compile( token_list, field_count, field_list, field_types, 
                             expr_out, &tokens_consumed );

    for( i = 0; i < token_count; i++ )
        SWQ_FREE( token_list[i] );

    if( error != NULL )
        return error;

    if( tokens_consumed < token_count )
    {
        swq_expr_free( *expr_out );
        *expr_out = NULL;
        sprintf( swq_error, "Syntax error, %d extra tokens", 
                 token_count - tokens_consumed );
        return swq_error;
    }

    return NULL;
}

/************************************************************************/
/*                           swq_expr_free()                            */
/************************************************************************/

void swq_expr_free( swq_expr *expr )

{
    if( expr == NULL )
        return;

    if( expr->first_sub_expr != NULL )
        swq_expr_free( (swq_expr *) expr->first_sub_expr );
    if( expr->second_sub_expr != NULL )
        swq_expr_free( (swq_expr *) expr->second_sub_expr );

    if( expr->string_value != NULL )
        SWQ_FREE( expr->string_value );

    SWQ_FREE( expr );
}

/************************************************************************/
/*                         swq_expr_evaluate()                          */
/************************************************************************/

int swq_expr_evaluate( swq_expr *expr, swq_op_evaluator fn_evaluator, 
                       void *record_handle )

{
    if( expr->operation == SWQ_OR )
    {
        return swq_expr_evaluate( (swq_expr *) expr->first_sub_expr, 
                                  fn_evaluator, 
                                  record_handle) 
            || swq_expr_evaluate( (swq_expr *) expr->second_sub_expr, 
                                  fn_evaluator, 
                                  record_handle);
    }
    else if( expr->operation == SWQ_AND )
    {
        return swq_expr_evaluate( (swq_expr *) expr->first_sub_expr, 
                                  fn_evaluator, 
                                  record_handle) 
            && swq_expr_evaluate( (swq_expr *) expr->second_sub_expr, 
                                  fn_evaluator, 
                                  record_handle);
    }
    else if( expr->operation == SWQ_NOT )
    {
        return !swq_expr_evaluate( (swq_expr *) expr->second_sub_expr, 
                                  fn_evaluator, 
                                   record_handle);
    }
    else
    {
        return fn_evaluator( expr, record_handle );
    }

    return FALSE;
}

/************************************************************************/
/*                           swq_expr_dump()                            */
/************************************************************************/

void swq_expr_dump( swq_expr *expr, FILE * fp, int depth )

{
    char	spaces[60];
    int		i;
    const char  *op_name = "unknown";

    for( i = 0; i < depth*2 && i < sizeof(spaces); i++ )
        spaces[i] = ' ';
    spaces[i] = '\0';

    /*
    ** first term.
    */
    if( expr->first_sub_expr != NULL )
        swq_expr_dump( (swq_expr *) expr->first_sub_expr, fp, depth + 1 );
    else
        fprintf( fp, "%s  Field %d\n", spaces, expr->field_index );

    /*
    ** Operation.
    */
    if( expr->operation == SWQ_OR )
        op_name = "OR";
    if( expr->operation == SWQ_AND )
        op_name = "AND";
    if( expr->operation == SWQ_NOT)
        op_name = "NOT";
    if( expr->operation == SWQ_GT )
        op_name = ">";
    if( expr->operation == SWQ_LT )
        op_name = "<";
    if( expr->operation == SWQ_EQ )
        op_name = "=";
    if( expr->operation == SWQ_NE )
        op_name = "!=";
    if( expr->operation == SWQ_GE )
        op_name = ">=";
    if( expr->operation == SWQ_LE )
        op_name = "<=";
    if( expr->operation == SWQ_LIKE )
        op_name = "LIKE";
    if( expr->operation == SWQ_ISNULL )
        op_name = "IS NULL";
    if( expr->operation == SWQ_IN )
        op_name = "IN";

    fprintf( fp, "%s%s\n", spaces, op_name );

    /*
    ** Second term.
    */
    if( expr->second_sub_expr != NULL )
        swq_expr_dump( (swq_expr *) expr->second_sub_expr, fp, depth + 1 );
    else if( expr->operation == SWQ_IN || expr->operation == SWQ_NOTIN )
    {
        const char *src;

        fprintf( fp, "%s  (\"%s\"", spaces, expr->string_value );
        src = expr->string_value + strlen(expr->string_value) + 1;
        while( *src != '\0' )
        {
            fprintf( fp, ",\"%s\"", src );
            src += strlen(src) + 1;
        }

        fprintf( fp, ")\n" );
    }
    else if( expr->string_value != NULL )
        fprintf( fp, "%s  %s\n", spaces, expr->string_value );
}

/************************************************************************/
/* ==================================================================== */
/*		SELECT statement parsing                                */
/* ==================================================================== */
/************************************************************************/

/*
Supported SQL Syntax:

SELECT <field-list> FROM table-name [WHERE <where-expr>] 
     [ORDER BY <sort specification list>]


<field-list> ::= DISTINCT fieldname | <field-spec> 
                 | <field-spec> , <field-list>

<field-spec> ::= fieldname 
                 | <field_func> ( [DISTINCT] <field-func> )
                 | Count(*)

<field-func> ::= AVG | MAX | MIN | SUM | COUNT


<sort specification list> ::=
              <sort specification> [ { <comma> <sort specification> }... ]

<sort specification> ::= <sort key> [ <ordering specification> ]

<sort key> ::=  <column name>

<ordering specification> ::= ASC | DESC
 */

/************************************************************************/
/*                        swq_select_preparse()                         */
/************************************************************************/

const char *swq_select_preparse( const char *select_statement, 
                                 swq_select **select_info_ret )

{
    swq_select *select_info;
    char *token;
    char *input;
    int  is_literal;
    swq_col_def  *swq_cols;

#define MAX_COLUMNS 250

    *select_info_ret = NULL;

/* -------------------------------------------------------------------- */
/*      Get first token. Ensure it is SELECT.                           */
/* -------------------------------------------------------------------- */
    token = swq_token( select_statement, &input, NULL );
    if( strcasecmp(token,"select") != 0 )
    {
        SWQ_FREE( token );
        strcpy( swq_error, "Missing keyword SELECT" );
        return swq_error;
    }
    SWQ_FREE( token );

/* -------------------------------------------------------------------- */
/*      allocate selection structure.                                   */
/* -------------------------------------------------------------------- */
    select_info = (swq_select *) SWQ_MALLOC(sizeof(swq_select));
    memset( select_info, 0, sizeof(swq_select) );

    select_info->raw_select = swq_strdup( select_statement );

/* -------------------------------------------------------------------- */
/*	Allocate a big field list. 					*/
/* -------------------------------------------------------------------- */
    swq_cols = (swq_col_def *) SWQ_MALLOC(sizeof(swq_col_def) * MAX_COLUMNS);
    memset( swq_cols, 0, sizeof(swq_col_def) * MAX_COLUMNS );

    select_info->column_defs = swq_cols;

/* -------------------------------------------------------------------- */
/*      Collect the field list, terminated by FROM keyword.             */
/* -------------------------------------------------------------------- */
    token = swq_token( input, &input, &is_literal );
    while( token != NULL 
           && (is_literal || strcasecmp(token,"FROM") != 0) )
    {
        char *next_token;
        int   next_is_literal;
        
        if( select_info->result_columns == MAX_COLUMNS )
        {
            SWQ_FREE( token );
            swq_select_free( select_info );
            sprintf( swq_error, 
                "More than MAX_COLUMNS (%d) columns in SELECT statement.", 
                     MAX_COLUMNS );
            return swq_error;
        }

        /* Ensure that we have a comma before fields other than the first. */

        if( select_info->result_columns > 0 )
        {
            if( strcasecmp(token,",") != 0 )
            {
                sprintf( swq_error, 
                         "Missing comma after column %s in SELECT statement.", 
                         swq_cols[select_info->result_columns-1].field_name );
                SWQ_FREE( token );
                swq_select_free( select_info );
                return swq_error;
            }

            SWQ_FREE( token );
            token = swq_token( input, &input, &is_literal );
        }

        /* read an extra token to check for brackets. */
        
        select_info->result_columns++;

        next_token = swq_token( input, &input, &next_is_literal );

        /*
        ** Handle function operators.
        */
        if( !is_literal && !next_is_literal && next_token != NULL 
            && strcasecmp(next_token,"(") == 0 )
        {
            SWQ_FREE( next_token );

            swq_cols[select_info->result_columns-1].col_func_name = token;

            token = swq_token( input, &input, &is_literal );

            if( token != NULL && !is_literal 
                && strcasecmp(token,"DISTINCT") == 0 )
            {
                swq_cols[select_info->result_columns-1].distinct_flag = 1;

                SWQ_FREE( token );
                token = swq_token( input, &input, &is_literal );
            }

            swq_cols[select_info->result_columns-1].field_name = token;

            token = swq_token( input, &input, &is_literal );

            if( token == NULL || strcasecmp(token,")") != 0 )
            {
                if( token != NULL )
                    SWQ_FREE( token );
                swq_select_free( select_info );
                return "Missing closing bracket in field function.";
            }

            SWQ_FREE( token );
            token = swq_token( input, &input, &is_literal );
        }

        /*
        ** Handle simple field.
        */
        else
        {
            if( token != NULL && !is_literal 
                && strcasecmp(token,"DISTINCT") == 0 )
            {
                swq_cols[select_info->result_columns-1].distinct_flag = 1;

                SWQ_FREE( token );
                token = next_token;
                is_literal = next_is_literal;

                next_token = swq_token( input, &input, &next_is_literal );
            }
            
            swq_cols[select_info->result_columns-1].field_name = token;
            token = next_token;
            is_literal = next_is_literal;
        }
    }

    /* make a columns_def list that is just the right size. */
    select_info->column_defs = (swq_col_def *) 
        SWQ_MALLOC(sizeof(swq_col_def) * select_info->result_columns);
    memcpy( select_info->column_defs, swq_cols,
            sizeof(swq_col_def) * select_info->result_columns );
    SWQ_FREE( swq_cols );

/* -------------------------------------------------------------------- */
/*      Collect the table name from the FROM clause.                    */
/* -------------------------------------------------------------------- */
    if( token == NULL || strcasecmp(token,"FROM") != 0 )
    {
        strcpy( swq_error, "Missing FROM clause in SELECT statement." );
        swq_select_free( select_info );
        return swq_error;
    }

    SWQ_FREE( token );
    token = swq_token( input, &input, &is_literal );

    if( token == NULL )
    {
        strcpy( swq_error, "Missing table name in FROM clause." );
        swq_select_free( select_info );
        return swq_error;
    }

    select_info->from_table = token;
    
    token = swq_token( input, &input, &is_literal );

/* -------------------------------------------------------------------- */
/*      Do we have a WHERE clause?                                      */
/* -------------------------------------------------------------------- */
    if( token != NULL && strcasecmp(token,"WHERE") == 0 )
    {
        const char *where_base = input;
        
        SWQ_FREE( token );
        
        token = swq_token( input, &input, &is_literal );
        while( token != NULL )
        {
            if( strcasecmp(token,"ORDER") == 0 && !is_literal )
            {
                break;
            }

            if( token != NULL )
            {
                SWQ_FREE( token );
            
                token = swq_token( input, &input, &is_literal );
            }
        }

        if( token != NULL )
        {
            select_info->whole_where_clause = swq_strdup(where_base);
            if( input != NULL )
                select_info->whole_where_clause[input - where_base - 5] = '\0';
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse ORDER BY clause.                                          */
/* -------------------------------------------------------------------- */
    if( token != NULL && strcasecmp(token,"ORDER") == 0 )
    {
        SWQ_FREE( token );
        
        token = swq_token( input, &input, &is_literal );

        if( token == NULL || strcasecmp(token,"BY") != 0 )
        {
            if( token != NULL )
                SWQ_FREE( token );

            strcpy( swq_error, "ORDER BY clause missing BY keyword." );
            swq_select_free( select_info );
            return swq_error;
        }

        SWQ_FREE( token );
        token = swq_token( input, &input, &is_literal );
        while( token != NULL 
               && (select_info->order_specs == 0 
                   || strcasecmp(token,",") == 0) )
        {
            swq_order_def  *old_defs = select_info->order_defs;
            swq_order_def  *def;

            if( select_info->order_specs != 0 )
            {
                SWQ_FREE( token );
                token = swq_token( input, &input, &is_literal );
            }

            select_info->order_defs = (swq_order_def *) 
                SWQ_MALLOC(sizeof(swq_order_def)*(select_info->order_specs+1));

            if( old_defs != NULL )
            {
                memcpy( select_info->order_defs, old_defs, 
                        sizeof(swq_order_def)*select_info->order_specs );
                SWQ_FREE( old_defs );
            }

            def = select_info->order_defs + select_info->order_specs;
            def->field_name = token;
            def->field_index = 0;
            def->ascending_flag = 1;

            token = swq_token( input, &input, &is_literal );
            if( token != NULL && strcasecmp(token,"DESC") == 0 )
            {
                SWQ_FREE( token );
                token = swq_token( input, &input, &is_literal );

                def->ascending_flag = 0;
            } 
            else if( token != NULL && strcasecmp(token,"ASC") == 0 )
            {
                SWQ_FREE( token );
                token = swq_token( input, &input, &is_literal );
            }

            select_info->order_specs++;
        }
    }

/* -------------------------------------------------------------------- */
/*      If we have anything left it indicates an error!                 */
/* -------------------------------------------------------------------- */
    if( token != NULL )
    {

        sprintf( swq_error, 
                 "Failed to parse SELECT statement, extra input at %s token.", 
                 token );

        SWQ_FREE( token );
        swq_select_free( select_info );
        return swq_error;
    }

    *select_info_ret = select_info;

    return NULL;
}

/************************************************************************/
/*                          swq_select_parse()                          */
/************************************************************************/

const char *swq_select_parse( swq_select *select_info, 
                              int field_count, 
                              char **field_list,
                              swq_field_type *field_types,
                              int parse_flags )

{
    int  i;

/* -------------------------------------------------------------------- */
/*      Expand "*" field for selects.                                   */
/* -------------------------------------------------------------------- */
    if( select_info->result_columns == 1
        && strcmp(select_info->column_defs[0].field_name,"*") == 0 
        && select_info->column_defs[0].col_func_name == NULL )
    {
        SWQ_FREE( select_info->column_defs[0].field_name );
        SWQ_FREE( select_info->column_defs );
        
        select_info->result_columns = field_count;
        select_info->column_defs = (swq_col_def *) 
            SWQ_MALLOC(field_count * sizeof(swq_col_def));
        memset( select_info->column_defs, 0, 
                sizeof(swq_col_def) * field_count );

        for( i = 0; i < select_info->result_columns; i++ )
        {
            swq_col_def *def = select_info->column_defs + i;

            def->field_name = swq_strdup( field_list[i] );
        }
    }

/* -------------------------------------------------------------------- */
/*      Identify field information.                                     */
/* -------------------------------------------------------------------- */
    for( i = 0; i < select_info->result_columns; i++ )
    {
        swq_col_def *def = select_info->column_defs + i;
        swq_field_type  this_type;

        /* identify field */
        def->field_index = swq_identify_field( def->field_name, field_count,
                                               field_list, field_types, 
                                               &this_type );

        /* record field type */
        def->field_type = field_types[def->field_index];

        /* identify column function if present */
        if( def->col_func_name != NULL )
        {
            if( strcasecmp(def->col_func_name,"AVG") == 0 )
                def->col_func = SWQCF_AVG;
            else if( strcasecmp(def->col_func_name,"MIN") == 0 )
                def->col_func = SWQCF_MIN;
            else if( strcasecmp(def->col_func_name,"MAX") == 0 )
                def->col_func = SWQCF_MAX;
            else if( strcasecmp(def->col_func_name,"SUM") == 0 )
                def->col_func = SWQCF_SUM;
            else if( strcasecmp(def->col_func_name,"COUNT") == 0 )
                def->col_func = SWQCF_COUNT;
            else
            {
                def->col_func = SWQCF_CUSTOM;
                if( !(parse_flags & SWQP_ALLOW_UNDEFINED_COL_FUNCS) )
                {
                    sprintf( swq_error, "Unrecognised field function %s.",
                             def->col_func_name );
                    return swq_error;
                }
            }

            if( (def->col_func == SWQCF_MIN 
                 || def->col_func == SWQCF_MAX
                 || def->col_func == SWQCF_AVG
                 || def->col_func == SWQCF_SUM)
                && this_type == SWQ_STRING )
            {
                sprintf( swq_error, 
                     "Use of field function %s() on string field %s illegal.", 
                         def->col_func_name, def->field_name );
                return swq_error;
            }
        }
        else
            def->col_func = SWQCF_NONE;

        if( def->field_index == -1 && def->col_func != SWQCF_COUNT )
        {
            sprintf( swq_error, "Unrecognised field name %s.", 
                     def->field_name );
            return swq_error;
        }
    }

/* -------------------------------------------------------------------- */
/*      Check if we are producing a one row summary result or a set     */
/*      of records.  Generate an error if we get conflicting            */
/*      indications.                                                    */
/* -------------------------------------------------------------------- */
    select_info->query_mode = -1;
    for( i = 0; i < select_info->result_columns; i++ )
    {
        swq_col_def *def = select_info->column_defs + i;
        int this_indicator = -1;

        if( def->col_func == SWQCF_MIN 
            || def->col_func == SWQCF_MAX
            || def->col_func == SWQCF_AVG
            || def->col_func == SWQCF_SUM
            || def->col_func == SWQCF_COUNT )
            this_indicator = SWQM_SUMMARY_RECORD;
        else if( def->col_func == SWQCF_NONE )
        {
            if( def->distinct_flag )
                this_indicator = SWQM_DISTINCT_LIST;
            else
                this_indicator = SWQM_RECORDSET;
        }

        if( this_indicator != select_info->query_mode
             && this_indicator != -1
            && select_info->query_mode != -1 )
        {
            return "Field list implies mixture of regular recordset mode, summary mode or distinct field list mode.";
        }

        if( this_indicator != -1 )
            select_info->query_mode = this_indicator;
    }

    if( select_info->result_columns > 1 
        && select_info->query_mode == SWQM_DISTINCT_LIST )
    {
        return "SELECTing more than one DISTINCT field is a query not supported.";
    }

/* -------------------------------------------------------------------- */
/*      Process column names in order specs.                            */
/* -------------------------------------------------------------------- */
    for( i = 0; i < select_info->order_specs; i++ )
    {
        swq_order_def *def = select_info->order_defs + i;

        /* identify field */
        def->field_index = swq_identify_field( def->field_name, field_count,
                                               field_list, field_types, NULL );
        if( def->field_index == -1 )
        {
            sprintf( swq_error, "Unrecognised field name %s in ORDER BY.", 
                     def->field_name );
            return swq_error;
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse the WHERE clause.                                         */
/* -------------------------------------------------------------------- */
    if( select_info->whole_where_clause != NULL )
    {
        const char *error;

        error = swq_expr_compile( select_info->whole_where_clause, 
                                  field_count, field_list, field_types, 
                                  &(select_info->where_expr) );

        if( error != NULL )
            return error;
    }

    return NULL;
}

/************************************************************************/
/*                        swq_select_summarize()                        */
/************************************************************************/

const char *
swq_select_summarize( swq_select *select_info, 
                      int dest_column, const char *value )

{
    swq_col_def *def = select_info->column_defs + dest_column;
    swq_summary *summary;

/* -------------------------------------------------------------------- */
/*      Do various checking.                                            */
/* -------------------------------------------------------------------- */
    if( !select_info->query_mode == SWQM_RECORDSET )
        return "swq_select_summarize() called on non-summary query.";

    if( dest_column < 0 || dest_column >= select_info->result_columns )
        return "dest_column out of range in swq_select_summarize().";

    if( def->col_func == SWQCF_NONE && !def->distinct_flag )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create the summary information if this is the first row         */
/*      being processed.                                                */
/* -------------------------------------------------------------------- */
    if( select_info->column_summary == NULL )
    {
        int i;

        select_info->column_summary = (swq_summary *) 
            SWQ_MALLOC(sizeof(swq_summary) * select_info->result_columns);
        memset( select_info->column_summary, 0, 
                sizeof(swq_summary) * select_info->result_columns );

        for( i = 0; i < select_info->result_columns; i++ )
        {
            select_info->column_summary[i].min = 1e20;
            select_info->column_summary[i].max = -1e20;
        }
    }

/* -------------------------------------------------------------------- */
/*      If distinct processing is on, process that now.                 */
/* -------------------------------------------------------------------- */
    summary = select_info->column_summary + dest_column;
    
    if( def->distinct_flag )
    {
        int  i;

        /* This should be implemented with a much more complicated
           data structure to achieve any sort of efficiency. */
        for( i = 0; i < summary->count; i++ )
        {
            if( strcmp(value,summary->distinct_list[i]) == 0 )
                break;
        }
        
        if( i == summary->count )
        {
            char  **old_list = summary->distinct_list;
            
            summary->distinct_list = (char **) 
                SWQ_MALLOC(sizeof(char *) * (summary->count+1));
            memcpy( summary->distinct_list, old_list, 
                    sizeof(char *) * summary->count );
            summary->distinct_list[(summary->count)++] = 
                swq_strdup( value );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process various options.                                        */
/* -------------------------------------------------------------------- */

    switch( def->col_func )
    {
      case SWQCF_MIN:
        if( value != NULL && value[0] != '\0' )
        {
            double df_val = atof(value);
            if( df_val < summary->min )
                summary->min = df_val;
        }
        break;
      case SWQCF_MAX:
        if( value != NULL && value[0] != '\0' )
        {
            double df_val = atof(value);
            if( df_val > summary->max )
                summary->max = df_val;
        }
        break;
      case SWQCF_AVG:
      case SWQCF_SUM:
        if( value != NULL && value[0] != '\0' )
        {
            summary->count++;
            summary->sum += atof(value);
        }
        break;

      case SWQCF_COUNT:
        if( value != NULL && !def->distinct_flag )
            summary->count++;
        break;

      case SWQCF_NONE:
        break;

      case SWQCF_CUSTOM:
        return "swq_select_summarize() called on custom field function.";

      default:
        return "swq_select_summarize() - unexpected col_func";
    }

    return NULL;
}
/************************************************************************/
/*                      sort comparison functions.                      */
/************************************************************************/

static int swq_compare_int( const void *item1, const void *item2 )
{
    int  v1, v2;

    v1 = atoi(*((const char **) item1));
    v2 = atoi(*((const char **) item2));

    return v2 - v1;
}

static int swq_compare_real( const void *item1, const void *item2 )
{
    double  v1, v2;

    v1 = atof(*((const char **) item1));
    v2 = atof(*((const char **) item2));

    if( v1 < v2 )
        return -1;
    else if( v1 == v2 )
        return 0;
    else
        return 1;
}

static int swq_compare_string( const void *item1, const void *item2 )
{
    return strcmp( *((const char **) item1), *((const char **) item2) );
}

/************************************************************************/
/*                    swq_select_finish_summarize()                     */
/*                                                                      */
/*      Call to complete summarize work.  Does stuff like ordering      */
/*      the distinct list for instance.                                 */
/************************************************************************/

const char *swq_select_finish_summarize( swq_select *select_info )

{
    int (*compare_func)(const void *, const void*);
    int count;
    char **distinct_list;

    if( select_info->query_mode != SWQM_DISTINCT_LIST 
        || select_info->order_specs == 0 )
        return NULL;

    if( select_info->order_specs > 1 )
        return "Can't ORDER BY a DISTINCT list by more than one key.";

    if( select_info->order_defs[0].field_index != 
        select_info->column_defs[0].field_index )
        return "Only selected DISTINCT field can be used for ORDER BY.";

    if( select_info->column_defs[0].field_type == SWQ_INTEGER )
        compare_func = swq_compare_int;
    else if( select_info->column_defs[0].field_type == SWQ_FLOAT )
        compare_func = swq_compare_real;
    else
        compare_func = swq_compare_string;

    distinct_list = select_info->column_summary[0].distinct_list;
    count = select_info->column_summary[0].count;

    qsort( distinct_list, count, sizeof(char *), compare_func );

/* -------------------------------------------------------------------- */
/*      Do we want the list ascending in stead of descending?           */
/* -------------------------------------------------------------------- */
    if( select_info->order_defs[0].ascending_flag )
    {
        char *saved;
        int i;

        for( i = 0; i < count/2; i++ )
        {
            saved = distinct_list[i];
            distinct_list[i] = distinct_list[count-i-1];
            distinct_list[count-i-1] = saved;
        }
    }

    return NULL;
}

/************************************************************************/
/*                          swq_select_free()                           */
/************************************************************************/

void swq_select_free( swq_select *select_info )

{
    int	i;

    if( select_info == NULL )
        return;

    if( select_info->raw_select != NULL )
        SWQ_FREE( select_info->raw_select );

    if( select_info->whole_where_clause != NULL )
        SWQ_FREE( select_info->whole_where_clause );

    if( select_info->from_table != NULL )
        SWQ_FREE( select_info->from_table );

    for( i = 0; i < select_info->result_columns; i++ )
    {
        if( select_info->column_defs[i].field_name != NULL )
            SWQ_FREE( select_info->column_defs[i].field_name );
        if( select_info->column_defs[i].col_func_name != NULL )
            SWQ_FREE( select_info->column_defs[i].col_func_name );

        if( select_info->column_summary != NULL 
            && select_info->column_summary[i].distinct_list != NULL )
        {
            int j;
            
            for( j = 0; j < select_info->column_summary[i].count; j++ )
                SWQ_FREE( select_info->column_summary[i].distinct_list[j] );

            SWQ_FREE( select_info->column_summary[i].distinct_list );
        }
    }

    if( select_info->column_defs != NULL )
        SWQ_FREE( select_info->column_defs );

    if( select_info->column_summary != NULL )
        SWQ_FREE( select_info->column_summary );

    for( i = 0; i < select_info->order_specs; i++ )
    {
        if( select_info->order_defs[i].field_name != NULL )
            SWQ_FREE( select_info->order_defs[i].field_name );
    }
    
    if( select_info->order_defs != NULL )
        SWQ_FREE( select_info->order_defs );

    SWQ_FREE( select_info );
}


