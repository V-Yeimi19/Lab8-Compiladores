// =============================================================================
// scanner.cpp — Implementación del Scanner (analizador léxico)
// =============================================================================

#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include "scanner.h"
#include "token.h"

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

Scanner::Scanner(const char* s) : input(s), first(0), current(0) {}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------

Scanner::~Scanner() {}

// -----------------------------------------------------------------------------
// Función auxiliar
// -----------------------------------------------------------------------------

static bool is_white_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

// -----------------------------------------------------------------------------
// nextToken — produce el siguiente token de la entrada
// -----------------------------------------------------------------------------

Token* Scanner::nextToken() {
    // Saltar espacios en blanco
    while (current < input.length() && is_white_space(input[current]))
        current++;

    // Fin de la entrada
    if (current >= input.length())
        return new Token(Token::END);

    char c = input[current];
    first = current;

    // ---- Números enteros ----
    if (isdigit(c)) {
        current++;
        while (current < input.length() && isdigit(input[current]))
            current++;
        return new Token(Token::NUM, input, first, current - first);
    }

    // ---- Identificadores y palabras reservadas ----
    if (isalpha(c)) {
        current++;
        while (current < input.length() && isalnum(input[current]))
            current++;
        std::string lexema = input.substr(first, current - first);

        if (lexema == "sqrt")     return new Token(Token::SQRT,     input, first, current - first);
        if (lexema == "print")    return new Token(Token::PRINT,    input, first, current - first);
        if (lexema == "if")       return new Token(Token::IF,       input, first, current - first);
        if (lexema == "then")     return new Token(Token::THEN,     input, first, current - first);
        if (lexema == "else")     return new Token(Token::ELSE,     input, first, current - first);
        if (lexema == "endif")    return new Token(Token::ENDIF,    input, first, current - first);
        if (lexema == "while")    return new Token(Token::WHILE,    input, first, current - first);
        if (lexema == "do")       return new Token(Token::DO,       input, first, current - first);
        if (lexema == "endwhile") return new Token(Token::ENDWHILE, input, first, current - first);
        if (lexema == "var")      return new Token(Token::VAR,      input, first, current - first);
        if (lexema == "true")     return new Token(Token::TRUE,     input, first, current - first);
        if (lexema == "false")    return new Token(Token::FALSE,    input, first, current - first);
        if (lexema == "fun")      return new Token(Token::FUN,      input, first, current - first);
        if (lexema == "endfun")   return new Token(Token::ENDFUN,   input, first, current - first);
        if (lexema == "return")   return new Token(Token::RETURN,   input, first, current - first);
        if (lexema == "and")      return new Token(Token::AND,      input, first, current - first);
        if (lexema == "or")       return new Token(Token::OR,       input, first, current - first);
        if (lexema == "not")      return new Token(Token::NOT,      input, first, current - first);
        if (lexema == "dowhile")  return new Token(Token::DOWHILE,  input, first, current - first);
        if (lexema == "enddo")    return new Token(Token::ENDDO,    input, first, current - first);
        if (lexema == "break")      return new Token(Token::BREAK,     input, first, current - first);
        if (lexema == "switch")     return new Token(Token::SWITCH,    input, first, current - first);
        if (lexema == "case")       return new Token(Token::CASE,      input, first, current - first);
        if (lexema == "default")    return new Token(Token::DEFAULT,   input, first, current - first);
        if (lexema == "endswitch")  return new Token(Token::ENDSWITCH, input, first, current - first);

        return new Token(Token::ID, input, first, current - first);
    }

    // ---- Operadores y delimitadores ----
    if (strchr("+/-*();=,<>:!&|", c)) {
        Token* token = nullptr;
        switch (c) {
            case '+': token = new Token(Token::PLUS,   c); break;
            case '-': token = new Token(Token::MINUS,  c); break;
            case '/': token = new Token(Token::DIV,    c); break;
            case '(': token = new Token(Token::LPAREN, c); break;
            case ')': token = new Token(Token::RPAREN, c); break;
            case ';': token = new Token(Token::SEMICOL,c); break;
            case ',': token = new Token(Token::COMA,   c); break;
            case ':': token = new Token(Token::COLON,  c); break;
            case '!': token = new Token(Token::NOT, c); break;
            case '<':
                if (current + 1 < input.length() && input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::LEQ, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::LE, c);
                }
                break;
            case '>':
                if (current + 1 < input.length() && input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::GEQ, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::GT, c);
                }
                break;
            case '*':
                if (current + 1 < input.length() && input[current + 1] == '*') {
                    current++;
                    token = new Token(Token::POW, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::MUL, c);
                }
                break;
            case '&':
                if (current + 1 < input.length() && input[current + 1] == '&') {
                    current++;
                    token = new Token(Token::AND, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::ERR, c);
                }
                break;
            case '|':
                if (current + 1 < input.length() && input[current + 1] == '|') {
                    current++;
                    token = new Token(Token::OR, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::ERR, c);
                }
                break;
            case '=':
                if (current + 1 < input.length() && input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::EQ, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::ASSIGN, c);
                }
                break;
            case '!':
                if (current + 1 < input.length() && input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::NEQ, input, first, current + 1 - first);
                } else {
                    token = new Token(Token::ERR, c);
                }
                break;
        }
        current++;
        return token;
    }

    // ---- Carácter no reconocido ----
    Token* err = new Token(Token::ERR, c);
    current++;
    return err;
}

// -----------------------------------------------------------------------------
// ejecutar_scanner — tokeniza un archivo y escribe los resultados
// -----------------------------------------------------------------------------

int ejecutar_scanner(Scanner* scanner, const std::string& InputFile) {
    // Construir nombre del archivo de salida
    std::string outputName = InputFile;
    size_t pos = outputName.find_last_of('.');
    if (pos != std::string::npos)
        outputName = outputName.substr(0, pos);
    outputName += "_tokens.txt";

    std::ofstream outFile(outputName);
    if (!outFile.is_open()) {
        std::cerr << "Error: no se pudo abrir el archivo de salida: " << outputName << std::endl;
        return 1;
    }

    outFile << "Scanner\n" << std::endl;

    Token* tok;
    while (true) {
        tok = scanner->nextToken();

        if (tok->type == Token::ERR) {
            outFile << *tok << std::endl;
            outFile << "Error léxico: carácter inválido '" << tok->text << "'\n";
            outFile << "\nScanner no exitoso\n";
            delete tok;
            outFile.close();
            return 1;
        }

        outFile << *tok << std::endl;

        if (tok->type == Token::END) {
            outFile << "\nScanner exitoso\n";
            delete tok;
            outFile.close();
            return 0;
        }

        delete tok;
    }
}
