- `program => (declaration | statement)*`
  <br><br>
- `declaration =>`\
  `variable_declaration |` \
  `function_declaration`
  <br><br>
- `variable_declaration =>` \
  `INT IDENT; | INT IDENT = expr;`
  <br><br>
- `function_declaration =>` \
  `FN IDENT((TYPE IDENT) (, TYPE IDENT)?) (-> TYPE)? block_statement`
  <br><br>
- `statement =>` \
  `assignment_statement |` \
  `if_statement         |` \
  `while_statement      |` \
  `block_statement      |` \
  `return_statement     |`
  <br><br>
- `assignment_statement` \
  `IDENT = expression;`
  <br><br>
- `if_statement =>` \
  `IF (CONDITION) block_statement`
  <br><br>
- `while_statement =>` \
  `WHILE (CONDITION) block_statement`
  <br><br>
- `block_statement =>` \
  `{(declatation | statement)*}`
  <br><br>
- `return_statement =>` \
  `RETURN expression;`
  <br><br>
- `expression =>` \
  `binary_expression` \
  `unary_expression` \
  `primary_expression`
  <br><br>
- `primary_expression =>` \
  `paren_expression` \
  `IDENT` \
  `INTEGER_LITERAL`
  <br><br>
- `paren_expression =>` \
  `( expression )`
  <br><br>
- `binary_expression =>` \
  `expression (binary operator) expression`
  <br><br>
- `unary_expression` => \
  `(unary operator) primary_expression`