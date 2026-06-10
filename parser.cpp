// =============================================================================
// parser.cpp — Implementación del Parser (analizador sintáctico descendente)
// =============================================================================

#include <iostream>
#include <stdexcept>
#include <string>
#include "ast.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"

// =============================================================================
// Constructor
// =============================================================================

Parser::Parser(Scanner* sc) : scanner(sc), previous(nullptr) {
    current = scanner->nextToken();
    if (current->type == Token::ERR) {
        throw std::runtime_error(
            "Error léxico: carácter no reconocido '" + current->text + "'"
        );
    }
}

// =============================================================================
// Primitivas de consumo de tokens
// =============================================================================

bool Parser::isAtEnd() {
    return current->type == Token::END;
}

bool Parser::check(Token::Type ttype) {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp = current;
        if (previous) delete previous;
        current = scanner->nextToken();
        previous = temp;
        if (current->type == Token::ERR) {
            throw std::runtime_error(
                "Error léxico: carácter no reconocido '" + current->text + "'"
            );
        }
        return true;
    }
    return false;
}

bool Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
        return true;
    }
    return false;
}

// =============================================================================
// Reporte de errores
// =============================================================================

// error — construye un mensaje indicando qué se esperaba vs. qué se encontró.
void Parser::error(const std::string& expected) {
    std::string found;
    if (isAtEnd()) {
        found = "fin de entrada";
    } else {
        found = Token::typeName(current->type);
        if (!current->text.empty())
            found += " '" + current->text + "'";
    }
    throw std::runtime_error(
        "Error sintáctico: se esperaba " + expected + ", pero se encontró " + found
    );
}

// expect — consume el token si coincide; si no, lanza error descriptivo.
void Parser::expect(Token::Type ttype) {
    if (!match(ttype))
        error(Token::typeName(ttype));
}

// =============================================================================
// Reglas gramaticales
// =============================================================================

// -----------------------------------------------------------------------------
// Program → VarDec* FunDec*
// -----------------------------------------------------------------------------
Program* Parser::parseProgram() {
    Program* p = new Program();

    // Declaraciones globales de variables
    while (check(Token::VAR)) {
        p->vdlist.push_back(parseVarDec());
        // Las VarDec globales se separan con ';'
        if (check(Token::FUN) || isAtEnd()) break;
        expect(Token::SEMICOL);
    }

    // Declaraciones de funciones
    while (check(Token::FUN)) {
        p->fdlist.push_back(parseFunDec());
    }

    // Al terminar debe haberse consumido toda la entrada
    if (!isAtEnd()) {
        error("declaración de función ('fun') o fin de entrada");
    }

    std::cout << "Parser exitoso" << std::endl;
    return p;
}

// -----------------------------------------------------------------------------
// VarDec → 'var' ID ID (',' ID)*
//   Primer ID = tipo, siguientes IDs = nombres de variables
// -----------------------------------------------------------------------------
VarDec* Parser::parseVarDec() {
    VarDec* vd = new VarDec();

    expect(Token::VAR);

    // Tipo
    if (!match(Token::ID))
        error("nombre de tipo después de 'var'");
    vd->type = previous->text;

    // Al menos un nombre de variable
    if (!match(Token::ID))
        error("nombre de variable después del tipo '" + vd->type + "'");
    vd->vars.push_back(previous->text);

    // Más variables separadas por coma
    while (match(Token::COMA)) {
        if (!match(Token::ID))
            error("nombre de variable después de ','");
        vd->vars.push_back(previous->text);
    }

    return vd;
}

// -----------------------------------------------------------------------------
// FunDec → 'fun' ID ID '(' Params ')' Body 'endfun'
//   Primer ID = tipo de retorno, segundo ID = nombre de función
// -----------------------------------------------------------------------------
FunDec* Parser::parseFunDec() {
    FunDec* fd = new FunDec();

    expect(Token::FUN);

    // Tipo de retorno
    if (!match(Token::ID))
        error("tipo de retorno después de 'fun'");
    fd->tipo = previous->text;

    // Nombre de la función
    if (!match(Token::ID))
        error("nombre de función después del tipo '" + fd->tipo + "'");
    fd->nombre = previous->text;

    // Parámetros formales
    expect(Token::LPAREN);
    while (check(Token::ID)) {
        // Tipo del parámetro
        match(Token::ID);
        fd->Ptipos.push_back(previous->text);

        // Nombre del parámetro
        if (!match(Token::ID))
            error("nombre de parámetro después del tipo '" + previous->text + "'");
        fd->Pnombres.push_back(previous->text);

        // Coma opcional entre parámetros
        if (check(Token::RPAREN)) break;
        if (!match(Token::COMA))
            error("',' o ')' en la lista de parámetros de '" + fd->nombre + "'");
    }
    expect(Token::RPAREN);

    // Cuerpo
    fd->cuerpo = parseBody();

    // Aceptar 'endfun' o fin de archivo (tolerancia a endfun faltante)
    if (!match(Token::ENDFUN) && !isAtEnd()) {
        error("'endfun' para cerrar la función '" + fd->nombre + "'");
    }

    return fd;
}

// -----------------------------------------------------------------------------
// Body → VarDec* Stm (';' Stm)*
// -----------------------------------------------------------------------------
Body* Parser::parseBody() {
    Body* b = new Body();

    // Declaraciones locales (separadas por ';'; el ';' final es opcional)
    while (check(Token::VAR)) {
        b->declarations.push_back(parseVarDec());
        if (!match(Token::SEMICOL)) break;
    }

    // Función auxiliar local: ¿el token actual puede iniciar un statement?
    // Un body termina cuando llega endif/endwhile/endfun/else/END.
    auto isBodyTerminator = [&]() {
        return check(Token::ENDIF)     || check(Token::ENDWHILE) ||
               check(Token::ENDFUN)    || check(Token::ELSE)     ||
               check(Token::ENDDO)     || check(Token::ENDSWITCH)||
               check(Token::CASE)      || check(Token::DEFAULT)  ||
               isAtEnd();
    };

    auto isStmStart = [&]() {
        return check(Token::ID)      || check(Token::PRINT)    ||
               check(Token::RETURN)  || check(Token::IF)       ||
               check(Token::WHILE)   || check(Token::DOWHILE)  ||
               check(Token::BREAK)   || check(Token::SWITCH);
    };

    // Al menos un statement
    if (!isBodyTerminator())
        b->StmList.push_back(parseStm());

    // Statements adicionales:
    // Se separan por ';', pero el ';' es opcional después de un bloque
    // estructurado (if/while) porque estos tienen su propio terminador.
    // Algoritmo: consumir ';' si existe; luego, si hay más statements, parsear.
    while (true) {
        // Consumir ';' opcional
        match(Token::SEMICOL);

        // Fin del body
        if (isBodyTerminator()) break;

        // Si hay algo que parsear, parsearlo
        if (isStmStart()) {
            b->StmList.push_back(parseStm());
        } else {
            break;
        }
    }

    return b;
}

// -----------------------------------------------------------------------------
// Stm → AssignStm | PrintStm | ReturnStm | IfStm | WhileStm
// -----------------------------------------------------------------------------
Stm* Parser::parseStm() {
    // ---- Asignación: ID '=' Exp ----
    if (match(Token::ID)) {
        std::string variable = previous->text;
        if (!match(Token::ASSIGN))
            error("'=' después del identificador '" + variable + "'");
        Exp* e = parseCE();
        return new AssignStm(variable, e);
    }

    // ---- Print: 'print' '(' Exp ')' ----
    if (match(Token::PRINT)) {
        expect(Token::LPAREN);
        Exp* e = parseCE();
        expect(Token::RPAREN);
        return new PrintStm(e);
    }

    // ---- Return: 'return' '(' Exp ')' ----
    if (match(Token::RETURN)) {
        ReturnStm* r = new ReturnStm();
        expect(Token::LPAREN);
        r->e = parseCE();
        expect(Token::RPAREN);
        return r;
    }

    // ---- If: 'if' CE 'then' Body ('else' Body)? 'endif' ----
    if (match(Token::IF)) {
        Exp*  cond = parseCE();
        Body* tb   = nullptr;
        Body* fb   = nullptr;

        if (!match(Token::THEN))
            error("'then' después de la condición del 'if'");
        tb = parseBody();

        if (match(Token::ELSE))
            fb = parseBody();

        if (!match(Token::ENDIF))
            error("'endif' para cerrar el bloque 'if'");

        return new IfStm(cond, tb, fb);
    }

    // ---- While: 'while' CE 'do' Body 'endwhile' ----
    if (match(Token::WHILE)) {
        Exp* cond = parseCE();

        if (!match(Token::DO))
            error("'do' después de la condición del 'while'");

        Body* b = parseBody();

        if (!match(Token::ENDWHILE))
            error("'endwhile' para cerrar el bloque 'while'");

        return new WhileStm(cond, b);
    }

    // ---- DoWhile: 'dowhile' Body 'enddo' CE ----
    if (match(Token::DOWHILE)) {
        Body* b = parseBody();

        if (!match(Token::ENDDO))
            error("'enddo' para cerrar el bloque 'dowhile'");

        Exp* cond = parseCE();
        return new DoWhileStm(b, cond);
    }

    // ---- Break ----
    if (match(Token::BREAK)) {
        return new BreakStm();
    }

    // ---- Switch: 'switch' CE ('case' NUM ':' Body)* ('default' ':' Body)? 'endswitch' ----
    if (match(Token::SWITCH)) {
        Exp* cond = parseCE();
        SwitchStm* sw = new SwitchStm(cond);

        // Parsear cases
        while (check(Token::CASE)) {
            match(Token::CASE);
            if (!check(Token::NUM))
                error("número entero después de 'case'");
            match(Token::NUM);
            int val = std::stoi(previous->text);
            if (!match(Token::COLON))
                error("':' después del valor del 'case'");
            Body* b = parseBody();
            sw->cases.push_back({val, b});
        }

        // Parsear default opcional
        if (match(Token::DEFAULT)) {
            if (!match(Token::COLON))
                error("':' después de 'default'");
            sw->defaultBody = parseBody();
        }

        if (!match(Token::ENDSWITCH))
            error("'endswitch' para cerrar el bloque 'switch'");

        return sw;
    }

    // ---- Sin coincidencia ----
    error("inicio de sentencia: identificador, 'print', 'return', 'if', 'while', 'dowhile', 'break' o 'switch'");
    return nullptr; // inalcanzable; silencia el warning del compilador
}

// =============================================================================
// Reglas de expresiones (precedencia ascendente)
// =============================================================================

// RE → BE (('<' | '>' | '>=' | '<=') BE)?
Exp* Parser::parseRE() {
    Exp* l = parseBE();
    if (match(Token::LE) || match(Token::GT) || match(Token::GEQ) || match(Token::LEQ)) {
        BinaryOp op;
        switch (previous->type) {
            case Token::LE:  op = LE_OP;  break;
            case Token::GT:  op = GT_OP;  break;
            case Token::GEQ: op = GEQ_OP; break;
            case Token::LEQ: op = LEQ_OP; break;
            default:         op = LE_OP;  break;
        }
        Exp* r = parseBE();
        l = new BinaryExp(l, r, op);
    }
    return l;
}

// NE → ('not')? RE
Exp* Parser::parseNE() {
    if (match(Token::NOT)) {
        Exp* e = parseRE();
        return new UnaryExp(e, NOT_OP);
    }
    return parseRE();
}

// AE → NE ('and' NE)*
Exp* Parser::parseAE() {
    Exp* l = parseNE();
    while (match(Token::AND)) {
        Exp* r = parseNE();
        l = new BinaryExp(l, r, AND_OP);
    }
    return l;
}

// CE → AE ('or' AE)*
Exp* Parser::parseCE() {
    Exp* l = parseAE();
    while (match(Token::OR)) {
        Exp* r = parseAE();
        l = new BinaryExp(l, r, OR_OP);
    }
    return l;
}

// BE → E (('+' | '-') E)*
Exp* Parser::parseBE() {
    Exp* l = parseE();
    while (match(Token::PLUS) || match(Token::MINUS)) {
        BinaryOp op = (previous->type == Token::PLUS) ? PLUS_OP : MINUS_OP;
        Exp* r = parseE();
        l = new BinaryExp(l, r, op);
    }
    return l;
}

// E → T (('*' | '/') T)*
Exp* Parser::parseE() {
    Exp* l = parseT();
    while (match(Token::MUL) || match(Token::DIV)) {
        BinaryOp op = (previous->type == Token::MUL) ? MUL_OP : DIV_OP;
        Exp* r = parseT();
        l = new BinaryExp(l, r, op);
    }
    return l;
}

// T → F ('**' F)?
Exp* Parser::parseT() {
    Exp* l = parseF();
    if (match(Token::POW)) {
        Exp* r = parseF();
        l = new BinaryExp(l, r, POW_OP);
    }
    return l;
}

// F → NUM | 'true' | 'false' | '(' CE ')' | ID ('(' Args ')')?
Exp* Parser::parseF() {
    // Número literal
    if (match(Token::NUM))
        return new NumberExp(std::stoi(previous->text));

    // Booleanos como enteros
    if (match(Token::TRUE))
        return new NumberExp(1);
    if (match(Token::FALSE))
        return new NumberExp(0);

    // Expresión entre paréntesis
    if (match(Token::LPAREN)) {
        Exp* e = parseCE();
        expect(Token::RPAREN);
        return e;
    }

    // Identificador o llamada a función
    if (match(Token::ID)) {
        std::string nom = previous->text;
        if (check(Token::LPAREN)) {
            // Llamada a función: ID '(' (CE (',' CE)*)? ')'
            advance(); // consume '('
            FcallExp* fcall = new FcallExp();
            fcall->nombre = nom;
            if (!check(Token::RPAREN)) {
                fcall->argumentos.push_back(parseCE());
                while (match(Token::COMA))
                    fcall->argumentos.push_back(parseCE());
            }
            expect(Token::RPAREN);
            return fcall;
        }
        return new IdExp(nom);
    }

    error("expresión: número, identificador, 'true', 'false' o '('");
    return nullptr; // inalcanzable
}
