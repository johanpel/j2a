// define a grammar called Hello
grammar Battery;

battery : object+ EOF;

object : HEADER numbers FOOTER '\n';

HEADER : '{"voltage":[';

numbers : INT (',' INT)*;

FOOTER : ']}';

INT
  : '0'
  | [1-9] [0-9]*
  ;

WS  : [ \t\r]+ -> skip ;