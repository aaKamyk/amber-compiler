- `program ::= (declaration)*` \
  <br>
- `declaration ::=` \
  `variable declaration |` \
  `function declaration` \
  `struct declaration` \
  `enum declaration` \
  <br>
- `variable_declaration ::=` \
  `[modifiers]? TYPE IDENT;` |\
  `[modifiers]? TYPE IDENT = expr;` \
  <br>
- `modifiers ::=` \
  `comptime` | `mut` | `pub` \
  <br>
- `function_declaration ::=` \
  `fn IDENT(TYPE IDENT, ...) (-> TYPE)? block statement` \
  `fn IDENT<TYPE, ...>(TYPE IDENT, ...) (-> TYPE)? where (comptime expression) block statment`
  <br>
- `statement ::=` \
  `block statement      |` \
  `assignment statement |` \
  `if statement         |` \
  `while statement      |` \
  `return statement     |` \
  `expression statement` \
  <br>
- `block statement ::=` \
  `{(declatation | statement)*}` \
  <br>
- `assignment statement` \
  `IDENT = expression;` \
  <br>
- `if statement ::=` \
  `IF (CONDITION) block_statement` \
  <br>
- `while statement ::=` \
  `WHILE (CONDITION) block_statement` \
  <br>
- `return statement ::=` \
  `RETURN expression;` \
  <br>
- `expression statement ::=` \
  `expression;` \
  <br>
- `expression ::=` \
  `primary expression` | \
  `binary expression` \
  <br>
- `primary expression ::=` \
  `atom expression` | \
  `paren expression` | \
  `unary expression` | \
  `call expression` \
  <br>
- `atom expression ::=` \
  `IDENT` \
  `INTEGER_LITERAL` \
  <br>
- `paren expression ::=` \
  `(expression)` \
  <br>
- `unary expression ::=` \
  `(unary operator) primary_expression` \
  <br>
- `call expression ::=` \
  `IDENT(expression, ...)`
  <br>
- `binary expression ::=` \
  `expr ession (binary operator) expression` \
  <br>