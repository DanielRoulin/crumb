#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "generic.h"
#include "scope.h"

// evaluates an ast in a given scope
Generic eval(AstNode *p_head, Scope *p_scope) {
  if (p_head->opcode == OP_INT) {
    // int case
    int *p_val = (int *) malloc(sizeof(int));
    *p_val = atoi(p_head->val);
    return Generic_new(TYPE_INT, p_val);

  } else if (p_head->opcode == OP_FLOAT) {
    // float case
    double *p_val = (double *) malloc(sizeof(double));
    *p_val = atof(p_head->val);
    return Generic_new(TYPE_FLOAT, p_val);

  } else if (p_head->opcode == OP_STRING) {
    // string case
    char **p_val = (char **) malloc(sizeof(char *));
    *p_val = p_head->val;
    return Generic_new(TYPE_STRING, p_val);

  } else if (p_head->opcode == OP_RETURN) {
    // simply return the value
    return eval(p_head->p_headChild, p_scope);

  } else if (p_head->opcode == OP_STATEMENT) {
    // statement case
    // for each child
    AstNode *p_curr = p_head->p_headChild;
    while (p_curr != NULL) {
      // if return found, return value out of statement, else just eval (and free generic)
      if (p_curr->opcode == OP_RETURN) return eval(p_curr, p_scope);
      else eval(p_curr, p_scope);
      p_curr = p_curr->p_next;
    }

    // if no return found, return void generic
    return Generic_new(TYPE_VOID, NULL);
  } else if (p_head->opcode == OP_IDENTIFIER) {
    // identifier case
    // return approriate identifier from scope
    return Scope_get(p_scope, p_head->val, p_head->lineNumber);

  } else if (p_head->opcode == OP_ASSIGNMENT) {
    // assignent case
    Scope_set(p_scope, p_head->p_headChild->val, eval(p_head->p_headChild->p_next, p_scope));
    return Generic_new(TYPE_VOID, NULL);

  } else if (p_head->opcode == OP_FUNCTION) {
    // function case
    // returns a function generic, whose value is a pointer to the functions ast node
    return Generic_new(TYPE_FUNCTION, p_head);

  } else if (p_head->opcode == OP_APPLICATION) {
    // application case
    // get function
    Generic func = eval(p_head->p_headChild, p_scope);

    if (func.type == TYPE_FUNCTION) {
      // if function found, create new scope, with current scope as parent
      Scope *p_local = Scope_new(p_scope);

      // loop over arguments
      AstNode *p_currApplyArg = p_head->p_headChild->p_next;
      AstNode *p_currFuncArg = ((AstNode *) func.p_val)->p_headChild;

      // set vars in local scope
      while (p_currApplyArg != NULL && p_currFuncArg->opcode != OP_STATEMENT) {
        Scope_set(p_local, p_currFuncArg->val, eval(p_currApplyArg, p_scope));
        p_currApplyArg = p_currApplyArg->p_next;
        p_currFuncArg = p_currFuncArg->p_next;
      }

      // error handling
      if (p_currApplyArg != NULL) {
        // supplied too many args, throw error
        printf(
          "Runtime Error @ Line %i: Supplied more arguments than required to function.\n", 
          p_currApplyArg->lineNumber
        );
        exit(0);
      } else if (p_currFuncArg->opcode != OP_STATEMENT) {
        // supplied too little args, throw error
        printf(
          "Runtime Error @ Line %i: Supplied less arguments than required to function.\n", 
          p_head->lineNumber
        );
        exit(0);
      }

      // now p_currFuncArg points to the statement, so we eval it on the local scope, and return the result
      Generic res = eval(p_currFuncArg, p_local);

      // free local scope
      Scope_free(p_local);

      // return 
      return res;
    } else if (func.type == TYPE_NATIVEFUNCTION) {
      // native functions contain pointers to c functions
      // get function
      Generic (*cb)(Scope *, Generic[], int, int) = func.p_val;

      Scope *p_local = Scope_new(p_scope);

      // collect arguments
      // first round collects length
      int count = 0;
      AstNode *p_curr = p_head->p_headChild->p_next;

      while (p_curr != NULL) {
        count++;
        p_curr = p_curr->p_next;
      }

      // second round, append to list
      Generic args[count];

      int i = 0;
      p_curr = p_head->p_headChild->p_next;

      while (i < count) {
        args[i] = eval(p_curr, p_scope);
        
        i++;
        p_curr = p_curr->p_next;
      }
      
      // call and return
      return (*cb)(p_local, args, count, p_head->lineNumber);

      // free scope
      Scope_free(p_local);

    } else {
      // if func is not a function type, throw error
      printf(
        "Runtime Error @ Line %i: Attempted to call %s instead of function.\n", 
        p_head->lineNumber, getTypeString(func.type)
      );
      exit(0);
    }
  }
}