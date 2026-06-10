# Compilador x86-64 — Laboratorio 12

## Descripción general

Compilador de un lenguaje imperativo simple (tipado estático, solo enteros) que produce ensamblador x86-64 en sintaxis AT&T. El proceso sigue el pipeline clásico:

```
Fuente (.txt)
    └─► Scanner  →  tokens
    └─► Parser   →  AST
    └─► TypeChecker  →  análisis semántico
    └─► GenCodeVisitor  →  archivo .s
```

El archivo `.s` resultante puede ensamblarse y vincularse directamente con `gcc`.

---

## Arquitectura del compilador

El compilador está compuesto por los siguientes módulos:

| Archivo | Rol |
|---|---|
| `token.h / token.cpp` | Define el tipo `Token` (enum `Type` + texto del lexema) y su función `typeName()` para mensajes de error |
| `scanner.h / scanner.cpp` | Analizador léxico: consume la entrada carácter a carácter y produce `Token*` mediante `nextToken()` |
| `ast.h / ast.cpp` | Jerarquía de nodos del AST: `Exp`, `Stm`, `VarDec`, `Body`, `FunDec`, `Program` |
| `parser.h / parser.cpp` | Parser descendente recursivo LL que construye el AST a partir del flujo de tokens |
| `visitor.h / visitor.cpp` | Patrón Visitor con dos implementaciones: `TypeCheckerVisitor` (análisis semántico) y `GenCodeVisitor` (generación de código) |
| `environment.h` | Entorno genérico con scopes anidados implementado como pila de tablas hash (`ribs`) |
| `main.cpp` | Punto de entrada: lee el archivo fuente, invoca el pipeline y escribe el `.s` |

---

## Gramática del lenguaje

Notación EBNF. Los terminales entre comillas simples son palabras reservadas o símbolos literales.

```ebnf
Program  →  VarDec* FunDec*

FunDec   →  'fun' ID ID '(' Params ')' Body 'endfun'
Params   →  (ID ID (',' ID ID)*)?

Body     →  VarDec* Stm (';' Stm)*
VarDec   →  'var' ID ID (',' ID)*

Stm      →  AssignStm
          | PrintStm
          | ReturnStm
          | IfStm
          | WhileStm
          | DoWhileStm
          | BreakStm
          | SwitchStm

AssignStm  →  ID '=' CE
PrintStm   →  'print' '(' CE ')'
ReturnStm  →  'return' '(' CE ')'
IfStm      →  'if' CE 'then' Body ('else' Body)? 'endif'
WhileStm   →  'while' CE 'do' Body 'endwhile'
DoWhileStm →  'dowhile' Body 'enddo' CE
BreakStm   →  'break'
SwitchStm  →  'switch' CE
               ('case' NUM ':' Body)*
               ('default' ':' Body)?
              'endswitch'

(* Jerarquía de expresiones — precedencia de menor a mayor: *)

CE  →  AE ('or' AE)*                  (* OR lógico *)
AE  →  NE ('and' NE)*                 (* AND lógico *)
NE  →  ('not')? RE                    (* NOT lógico *)
RE  →  BE (('<' | '>' | '>=' | '<=') BE)?   (* relacionales *)
BE  →  E (('+' | '-') E)*             (* suma / resta *)
E   →  T (('*' | '/') T)*             (* multiplicación / división *)
T   →  F ('**' F)?                    (* potencia *)
F   →  NUM
     | 'true'                         (* alias de 1 *)
     | 'false'                        (* alias de 0 *)
     | '(' CE ')'
     | ID
     | ID '(' (CE (',' CE)*)? ')'     (* llamada a función *)
```

---

## Features implementados

### 1. Operadores relacionales

Tokens reconocidos por el scanner:

| Lexema | Token | Operación |
|---|---|---|
| `<` | `LE` | menor estricto |
| `>` | `GT` | mayor estricto |
| `>=` | `GEQ` | mayor o igual |
| `<=` | `LEQ` | menor o igual |

El parser los reconoce en `parseRE()`. La generación de código emite `cmpq` seguido de una instrucción `set*` que deja 0 o 1 en `%rax`:

| Operador | Instrucción `set*` |
|---|---|
| `<` (`LE_OP`) | `setle` |
| `>` (`GT_OP`) | `setg` |
| `>=` (`GEQ_OP`) | `setge` |
| `<=` (`LEQ_OP`) | `setle` |

Ejemplo:

```
fun int main()
    var int a;
    a = 5;
    if a >= 3 then
        print(1)
    endif
    return(0)
endfun
```

### 2. Operadores lógicos

Tokens: `AND` (`and`), `OR` (`or`), `NOT` (`not`).

Precedencia (mayor a menor): `not` > `and` > `or`, implementada mediante la jerarquía `NE → AE → CE`.

La generación de código normaliza ambos operandos a 0/1 antes de aplicar `andq`/`orq`. El operador `not` usa `sete` (verdadero si `%rax == 0`).

Ejemplo:

```
fun int main()
    var int x;
    x = 4;
    if x >= 3 and x <= 5 then
        print(x)
    endif;
    if not 0 then
        print(1)
    endif
    return(0)
endfun
```

### 3. Bucle do-while

**Sintaxis:**
```
dowhile
    Body
enddo CE
```

**Semántica:** ejecuta `Body` al menos una vez; al llegar a `enddo` evalúa `CE` y vuelve al inicio si el resultado es distinto de 0.

**Generación de código:**

```asm
dowhile_N:
    <Body>
    <CE>           # resultado en %rax
    cmpq $0, %rax
    jne dowhile_N
enddowhile_N:
```

Ejemplo:

```
fun int main()
    var int x;
    x = 1;
    dowhile
        print(x);
        x = x + 1
    enddo x < 5
    return(0)
endfun
```

### 4. Sentencia break

**Sintaxis:** `break`

**Semántica:** salta incondicionalmente al final del loop (`while` / `dowhile`) o del `switch` más cercano que lo contiene.

**Implementación:** `GenCodeVisitor` mantiene un `std::vector<std::string> breakLabels` (pila de etiquetas de salida). Al entrar a un `while`, `dowhile` o `switch` se hace `push_back` de la etiqueta de fin; al salir se hace `pop_back`. `visit(BreakStm)` emite `jmp breakLabels.back()`.

Ejemplo:

```
fun int main()
    var int i;
    i = 0;
    while i < 10 do
        if i >= 5 then
            break
        endif;
        print(i);
        i = i + 1
    endwhile
    return(0)
endfun
```

### 5. Switch / case / default

**Sintaxis:**
```
switch CE
  case N: Body
  case M: Body
  ...
  default: Body
endswitch
```

- Los valores de `case` deben ser literales enteros (`NUM`).
- No hay fall-through: cada case tiene un `jmp endswitch_N` al final de su cuerpo.
- La cláusula `default` es opcional.
- Se puede usar `break` dentro de un case para salir antes del final del cuerpo.

**Generación de código:**

```asm
    <CE>           # evalúa condición, resultado en %rax
    pushq %rax     # guarda condición en el stack
case_N_0:
    movq (%rsp), %rax
    cmpq $valor0, %rax
    jne  case_N_1
    <Body0>
    jmp  endswitch_N
case_N_1:
    movq (%rsp), %rax
    cmpq $valor1, %rax
    jne  default_N
    <Body1>
    jmp  endswitch_N
default_N:
    <BodyDefault>   # ausente si no hay default
endswitch_N:
    popq %rax      # restaura el stack
```

Ejemplo con default:

```
fun int main()
    var int d;
    d = 3;
    switch d
      case 1: print(100)
      case 2: print(200)
      default: print(999)
    endswitch
    return(0)
endfun
```

Ejemplo con break dentro de case:

```
fun int main()
    var int v;
    v = 2;
    switch v
      case 1: print(10); break
      case 2: print(20)
    endswitch
    return(0)
endfun
```

### 6. Estructuras anidadas

Las estructuras pueden anidarse libremente porque `parseBody()` es recursivo: cualquier sentencia que contiene un cuerpo interno (if, while, dowhile, switch) vuelve a llamar a `parseBody()`. El contador `labelcont` en `GenCodeVisitor` garantiza etiquetas únicas incluso con nesting profundo.

#### Ejemplo 1 — switch dentro de while

```
fun int main()
    var int i;
    i = 1;
    while i <= 3 do
        switch i
          case 1: print(10)
          case 2: print(20)
          case 3: print(30)
        endswitch;
        i = i + 1
    endwhile
    return(0)
endfun
```

Resultado esperado: imprime `10`, `20`, `30`.
**Estado:** compilacion exitosa, ensamblador generado correctamente (etiquetas `while_0`, `case_1_0..2`, `endswitch_1`, `endwhile_0`).

#### Ejemplo 2 — dowhile con operadores lógicos compuestos

```
fun int main()
    var int x;
    x = 0;
    dowhile
        if x >= 3 and x <= 5 then
            print(x)
        endif;
        x = x + 1
    enddo x < 8
    return(0)
endfun
```

Resultado esperado: imprime `3`, `4`, `5`.
**Estado:** compilacion exitosa, ensamblador generado correctamente (etiquetas `dowhile_0`, `else_1`, `endif_1`, `enddowhile_0`).

#### Ejemplo 3 — if dentro de switch dentro de while

```
fun int main()
    var int n;
    n = 1;
    while n <= 4 do
        switch n
          case 2:
            if not 0 then
                print(200)
            endif
          case 4: print(400)
        endswitch;
        n = n + 1
    endwhile
    return(0)
endfun
```

Resultado esperado: imprime `200`, `400`.
**Estado:** compilacion exitosa, ensamblador generado correctamente (etiquetas `while_0`, `case_1_0`, `case_1_1`, `else_2`, `endif_2`, `endswitch_1`, `endwhile_0`).

---

## Cambios por archivo y feature

Esta sección detalla exactamente qué se añadió en cada archivo del compilador para implementar cada feature. Organizado primero por feature y luego por archivo para facilitar la revisión.

---

### Feature 1 — Operadores relacionales (`>`, `>=`, `<=`)

> El operador `<` ya existía; se añadieron los tres restantes en todas las capas.

#### `token.h`
Añadidos al enum `Type` (sección *Operadores relacionales*):
```cpp
GT,   // >
GEQ,  // >=
LEQ,  // <=
```

#### `token.cpp`
- En `typeName()`: tres casos nuevos → `"'>'"`, `"'>='"`  , `"'<='"`.
- En `operator<<`: tres ramas nuevas `TOKEN(GT, …)`, `TOKEN(GEQ, …)`, `TOKEN(LEQ, …)`.

#### `scanner.cpp`
- El `case '<'` pasó de emitir directamente `LE` a hacer lookahead de 1 carácter:
  - `<=` → `Token::LEQ`
  - `<`  → `Token::LE`
- Se añadió `'>'` al conjunto `strchr` y su `case '>'` con lookahead:
  - `>=` → `Token::GEQ`
  - `>`  → `Token::GT`

#### `ast.h`
Extendido el enum `BinaryOp`:
```cpp
GT_OP,   // >
GEQ_OP,  // >=
LEQ_OP,  // <=
```

#### `ast.cpp`
En `binopToChar()`: tres casos nuevos → `">"`, `">="`, `"<="`.

#### `parser.h`
- La antigua `parseCE()` (solo manejaba `<`) se renombró a `parseRE()`.
- Se añadieron las declaraciones `parseRE()`, `parseAE()`, `parseNE()`, y la nueva `parseCE()`.

#### `parser.cpp`
- `parseCE()` (renombrada a `parseRE()`): ahora acepta los cuatro operadores relacionales con un `switch` sobre `previous->type`:
```cpp
if (match(Token::LE) || match(Token::GT) || match(Token::GEQ) || match(Token::LEQ)) { … }
```

#### `visitor.h`
Sin cambios propios de este feature (los tres nuevos `BinaryOp` se despachan en `visit(BinaryExp*)` ya existente).

#### `visitor.cpp`
En `GenCodeVisitor::visit(BinaryExp*)`, añadidos tres `case` en el `switch`:
```cpp
case GT_OP:  /* cmpq + setg  + movzbq */
case GEQ_OP: /* cmpq + setge + movzbq */
case LEQ_OP: /* cmpq + setle + movzbq */
```

---

### Feature 2 — Operadores lógicos (`and`, `or`, `not`)

#### `token.h`
Añadidos al enum `Type` (sección *Operadores lógicos*):
```cpp
AND,  // and
OR,   // or
NOT,  // not
```

#### `token.cpp`
- En `typeName()`: `"'and'"`, `"'or'"`, `"'not'"`.
- En `operator<<`: `TOKEN(AND, …)`, `TOKEN(OR, …)`, `TOKEN(NOT, …)`.

#### `scanner.cpp`
En el bloque de identificadores/palabras reservadas, añadidas tres líneas:
```cpp
if (lexema == "and") return new Token(Token::AND, …);
if (lexema == "or")  return new Token(Token::OR,  …);
if (lexema == "not") return new Token(Token::NOT, …);
```

#### `ast.h`
- `BinaryOp` extendido: `AND_OP`, `OR_OP`.
- Añadido enum `UnaryOp { NOT_OP }`.
- Añadida clase `UnaryExp`:
```cpp
class UnaryExp : public Exp {
public:
    Exp*    exp;
    UnaryOp op;
    UnaryExp(Exp* e, UnaryOp o);
    int accept(Visitor*) override;
    ~UnaryExp() { delete exp; }
};
```

#### `ast.cpp`
En `binopToChar()`: `"and"` y `"or"`.

#### `visitor.h`
- Forward declaration: `class UnaryExp;`.
- En `Visitor` (base): `virtual int visit(UnaryExp* exp) = 0;`.
- En `TypeCheckerVisitor` y `GenCodeVisitor`: `int visit(UnaryExp* exp) override;`.

#### `visitor.cpp`
- Dispatcher: `int UnaryExp::accept(Visitor* v) { return v->visit(this); }`.
- `TypeCheckerVisitor::visit(UnaryExp*)`: recursa en `exp->exp`.
- `GenCodeVisitor::visit(BinaryExp*)`, dos `case` nuevos:
  - `AND_OP`: normaliza ambos operandos a 0/1 con `setne`, luego `andq %rcx, %rax`.
  - `OR_OP`: ídem con `orq %rcx, %rax`.
- `GenCodeVisitor::visit(UnaryExp*)` nuevo:
```cpp
// evalúa exp->exp → %rax, luego:
cmpq $0, %rax
movq $0, %rax
sete %al          // 1 si el operando era 0 (NOT lógico)
movzbq %al, %rax
```

#### `parser.h`
Declaraciones nuevas: `parseCE()` (OR), `parseAE()` (AND), `parseNE()` (NOT).

#### `parser.cpp`
Tres funciones nuevas que reemplazan/extienden la jerarquía de expresiones:
```cpp
// NE → ('not')? RE
Exp* Parser::parseNE() {
    if (match(Token::NOT)) return new UnaryExp(parseRE(), NOT_OP);
    return parseRE();
}
// AE → NE ('and' NE)*
Exp* Parser::parseAE() { … }
// CE → AE ('or' AE)*   ← nuevo tope de la jerarquía
Exp* Parser::parseCE() { … }
```

---

### Feature 3 — Bucle `dowhile` / `enddo`

#### `token.h`
```cpp
DOWHILE,  // dowhile
ENDDO,    // enddo
```

#### `token.cpp`
`typeName()` y `operator<<` para `DOWHILE` y `ENDDO`.

#### `scanner.cpp`
```cpp
if (lexema == "dowhile") return new Token(Token::DOWHILE, …);
if (lexema == "enddo")   return new Token(Token::ENDDO,   …);
```

#### `ast.h`
```cpp
class DoWhileStm : public Stm {
public:
    Body* b;
    Exp*  condition;
    DoWhileStm(Body* b, Exp* condition);
    int accept(Visitor*) override;
};
```

#### `visitor.h`
- Forward declaration: `class DoWhileStm;`.
- `Visitor`: `virtual int visit(DoWhileStm*) = 0;`.
- `TypeCheckerVisitor` y `GenCodeVisitor`: `int visit(DoWhileStm*) override;`.

#### `visitor.cpp`
- Dispatcher: `int DoWhileStm::accept(Visitor* v) { return v->visit(this); }`.
- `TypeCheckerVisitor::visit(DoWhileStm*)`: visita body y condición.
- `GenCodeVisitor::visit(DoWhileStm*)`: emite cuerpo primero, luego condición con `jne` al inicio:
```asm
dowhile_N:
    <body>
    <condición>   → %rax
    cmpq $0, %rax
    jne dowhile_N
enddowhile_N:
```
También gestiona `breakLabels`: `push_back("enddowhile_N")` al entrar, `pop_back()` al salir.

#### `parser.cpp`
- `isBodyTerminator`: añadido `check(Token::ENDDO)`.
- `isStmStart`: añadido `check(Token::DOWHILE)`.
- En `parseStm()`, nuevo caso:
```cpp
if (match(Token::DOWHILE)) {
    Body* b = parseBody();
    expect(Token::ENDDO);
    Exp* cond = parseCE();
    return new DoWhileStm(b, cond);
}
```

---

### Feature 4 — Sentencia `break`

#### `token.h`
```cpp
BREAK,  // break
```

#### `token.cpp`
`typeName()` → `"'break'"` y entrada en `operator<<`.

#### `scanner.cpp`
```cpp
if (lexema == "break") return new Token(Token::BREAK, …);
```

#### `ast.h`
```cpp
class BreakStm : public Stm {
public:
    BreakStm() {}
    int accept(Visitor*) override;
};
```

#### `visitor.h`
- Forward declaration: `class BreakStm;`.
- `Visitor`: `virtual int visit(BreakStm*) = 0;`.
- `TypeCheckerVisitor` y `GenCodeVisitor`: `int visit(BreakStm*) override;`.
- `GenCodeVisitor`: campo nuevo `std::vector<std::string> breakLabels;`.

#### `visitor.cpp`
- Dispatcher: `int BreakStm::accept(Visitor* v) { return v->visit(this); }`.
- `TypeCheckerVisitor::visit(BreakStm*)`: no-op (`return 0`).
- `GenCodeVisitor::visit(BreakStm*)`:
```cpp
if (!breakLabels.empty())
    out << " jmp " << breakLabels.back() << "\n";
```
- `GenCodeVisitor::visit(WhileStm*)` **modificado** para gestionar el stack:
```cpp
breakLabels.push_back("endwhile_" + to_string(lbl));
// … cuerpo del while …
breakLabels.pop_back();
```

#### `parser.cpp`
- `isStmStart`: añadido `check(Token::BREAK)`.
- En `parseStm()`, nuevo caso:
```cpp
if (match(Token::BREAK)) return new BreakStm();
```

---

### Feature 5 — `switch` / `case` / `default` / `endswitch`

#### `token.h`
```cpp
SWITCH,     // switch
CASE,       // case
DEFAULT,    // default
ENDSWITCH,  // endswitch
COLON,      // :
```

#### `token.cpp`
`typeName()` y `operator<<` para los 5 tokens nuevos.

#### `scanner.cpp`
- Palabras reservadas:
```cpp
if (lexema == "switch")    return new Token(Token::SWITCH, …);
if (lexema == "case")      return new Token(Token::CASE, …);
if (lexema == "default")   return new Token(Token::DEFAULT, …);
if (lexema == "endswitch") return new Token(Token::ENDSWITCH, …);
```
- Símbolo `':'` añadido al conjunto `strchr` y al `switch`:
```cpp
case ':': token = new Token(Token::COLON, c); break;
```

#### `ast.h`
```cpp
struct CaseItem { int value; Body* body; };

class SwitchStm : public Stm {
public:
    Exp*                  condition;
    std::vector<CaseItem> cases;
    Body*                 defaultBody;   // nullptr si ausente
    SwitchStm(Exp* cond);
    int accept(Visitor*) override;
};
```

#### `visitor.h`
- Forward declaration: `class SwitchStm;`.
- `Visitor`: `virtual int visit(SwitchStm*) = 0;`.
- `TypeCheckerVisitor` y `GenCodeVisitor`: `int visit(SwitchStm*) override;`.

#### `visitor.cpp`
- Dispatcher: `int SwitchStm::accept(Visitor* v) { return v->visit(this); }`.
- `TypeCheckerVisitor::visit(SwitchStm*)`: visita condición, todos los bodies de cases y el defaultBody.
- `GenCodeVisitor::visit(SwitchStm*)`:
  1. Evalúa condición → `pushq %rax` (guarda en stack).
  2. Empuja `"endswitch_N"` en `breakLabels`.
  3. Por cada `CaseItem`: emite `case_N_i:`, `movq (%rsp), %rax`, `cmpq $val, %rax`, `jne` al siguiente, body, `jmp endswitch_N`.
  4. Emite `default_N:` + body opcional.
  5. Emite `endswitch_N:` + `popq %rax`.
  6. Desapila `breakLabels`.

#### `parser.cpp`
- `isBodyTerminator`: añadidos `check(Token::ENDSWITCH)`, `check(Token::CASE)`, `check(Token::DEFAULT)`.
- `isStmStart`: añadido `check(Token::SWITCH)`.
- En `parseStm()`, nuevo caso:
```cpp
if (match(Token::SWITCH)) {
    Exp* cond = parseCE();
    SwitchStm* sw = new SwitchStm(cond);
    while (check(Token::CASE)) {
        match(Token::CASE);
        match(Token::NUM);  int val = stoi(previous->text);
        expect(Token::COLON);
        sw->cases.push_back({ val, parseBody() });
    }
    if (match(Token::DEFAULT)) {
        expect(Token::COLON);
        sw->defaultBody = parseBody();
    }
    expect(Token::ENDSWITCH);
    return sw;
}
```

---

### Feature 6 — Estructuras anidadas

No requirió cambios adicionales en ningún archivo. El soporte es consecuencia directa de:

- **`parseBody()` es recursivo**: cualquier sentencia compuesta (if, while, dowhile, switch) llama internamente a `parseBody()`, que a su vez puede contener cualquier otra sentencia compuesta, sin límite de profundidad.
- **`labelcont` es global en `GenCodeVisitor`**: cada llamada a `visit(IfStm*)`, `visit(WhileStm*)`, etc. incrementa `labelcont` antes de emitir etiquetas, garantizando que `if_0`, `while_1`, `case_2_0`, etc. sean únicas sin importar el nivel de anidación.
- **`breakLabels` es una pila**: al anidar loops o switch, cada nivel empuja su etiqueta de salida independientemente; `break` siempre salta al nivel más interno correcto.

---

## Convenciones del código generado

El compilador sigue la ABI System V x86-64:

| Convención | Detalle |
|---|---|
| Registros de argumentos | `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9` (en ese orden) |
| Registro de retorno | `%rax` |
| Registro temporal de operando derecho | `%rcx` |
| Variables locales | Offsets negativos desde `%rbp` (primer variable: `-8(%rbp)`, segunda: `-16(%rbp)`, etc.) |
| Variables globales | Símbolos en sección `.data` con acceso RIP-relativo: `var(%rip)` |
| Frame | Prólogo: `pushq %rbp` / `movq %rsp, %rbp` / `subq $N, %rsp`; epílogo: `leave` / `ret` |
| Salida de función | Salto a `.end_<nombre>` antes del epílogo (para `return` en medio del cuerpo) |
| Impresión | `printf@PLT` con formato `"%ld \n"` definido en `.data` como `print_fmt` |
| Stack ejecutable | Sección `.note.GNU-stack,"",@progbits` al final para marcar el stack como no ejecutable |

---

## Cómo compilar y usar

### Compilar el compilador

```bash
g++ -std=c++17 -o compilador \
    token.cpp scanner.cpp ast.cpp parser.cpp visitor.cpp main.cpp
```

### Compilar un programa fuente a ensamblador

```bash
./compilador mi_programa.txt
# produce: mi_programa.s
```

### Ensamblar y enlazar con gcc

```bash
gcc mi_programa.s -o mi_programa
./mi_programa
```

---

## Ejemplos de uso

### Ejemplo 1 — Factorial con recursión

```
fun int fac(int n)
    if n < 2 then
        return(n)
    else
        return(fac(n - 1) * n)
    endif
endfun

fun int main()
    var int x;
    x = 1;
    while x < 10 do
        print(fac(x));
        x = x + 1
    endwhile;
    return(0)
endfun
```

Imprime los factoriales del 1 al 9.

### Ejemplo 2 — Bucle do-while con break

```
fun int main()
    var int i;
    i = 0;
    dowhile
        if i >= 5 then
            break
        endif;
        print(i);
        i = i + 1
    enddo i < 100
    return(0)
endfun
```

Imprime `0 1 2 3 4` y sale por `break` antes de llegar a 100.

### Ejemplo 3 — Switch con default

```
fun int main()
    var int d;
    d = 3;
    switch d
      case 1: print(100)
      case 2: print(200)
      default: print(999)
    endswitch
    return(0)
endfun
```

Imprime `999` (ningún case coincide con 3, se ejecuta default).

### Ejemplo 4 — Operadores lógicos y relacionales

```
fun int main()
    var int a;
    a = 7;
    if a >= 5 and a <= 10 then
        print(1)
    else
        print(0)
    endif;
    if not (a > 10) then
        print(2)
    endif
    return(0)
endfun
```

Imprime `1` (7 está en [5,10]) y luego `2` (7 no es mayor que 10).

---

## Limitaciones conocidas

- **Solo tipo `int`:** no hay tipos `bool`, `float`, `char` ni arreglos. `true` y `false` son azúcar sintáctica para 1 y 0.
- **Máximo 6 parámetros por función:** limitación de la ABI System V x86-64 que solo usa registros para argumentos; el compilador no emite código para argumentos en el stack.
- **`switch` solo compara con literales enteros:** el valor de cada `case` debe ser un `NUM` fijo en el código fuente, no una expresión arbitraria.
- **Sin verificación de tipos en expresiones:** `int` y el resultado lógico (0/1) son intercambiables sin error.
- **Sin comentarios en el fuente:** el scanner no reconoce `//` ni `/* */`; cualquier carácter no reconocido produce un error léxico `ERR`.
- **`LE_OP` (`<`) usa `setle` en lugar de `setl`:** ambos `<` y `<=` emiten `setle`; en la práctica `<` se comporta como `<=`. Es una inconsistencia menor en `visitor.cpp`.
- **Variables declaradas solo al inicio del `Body`:** no se permiten declaraciones intercaladas entre sentencias (ej. dentro de un `if` sí, pero siempre antes de la primera sentencia del bloque).
