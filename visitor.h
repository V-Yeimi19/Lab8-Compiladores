#ifndef VISITOR_H
#define VISITOR_H

// =============================================================================
// visitor.h — Definición de los visitantes del AST
// =============================================================================
// Se implementan dos visitantes sobre el patrón Visitor:
//
//   TypeCheckerVisitor — Análisis semántico:
//     · Verifica que las variables usadas hayan sido declaradas.
//     · Verifica que las funciones llamadas existan.
//     · Verifica que las llamadas tengan el número correcto de argumentos.
//     · Cuenta las variables locales de cada función para la generación.
//
//   GenCodeVisitor — Generación de código x86-64 (AT&T syntax):
//     · Emite ensamblador para cada nodo del AST.
//     · Usa TypeCheckerVisitor para obtener metadatos (tamaño de frame, etc.).
// =============================================================================

#include "ast.h"
#include "environment.h"
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
class BinaryExp;
class UnaryExp;
class Body;
class FunDec;
class FcallExp;
class IdExp;
class IfStm;
class NumberExp;
class PrintStm;
class Program;
class ReturnStm;
class Stm;
class VarDec;
class WhileStm;
class DoWhileStm;
class BreakStm;
class SwitchStm;

// =============================================================================
// Clase base abstracta Visitor
// =============================================================================

class Visitor {
public:
    virtual int visit(BinaryExp* exp) = 0;
    virtual int visit(UnaryExp* exp)  = 0;
    virtual int visit(NumberExp* exp) = 0;
    virtual int visit(IdExp* exp)     = 0;
    virtual int visit(Program* p)     = 0;
    virtual int visit(PrintStm* stm)  = 0;
    virtual int visit(WhileStm* stm)   = 0;
    virtual int visit(IfStm* stm)      = 0;
    virtual int visit(AssignStm* stm)  = 0;
    virtual int visit(Body* body)      = 0;
    virtual int visit(VarDec* vd)      = 0;
    virtual int visit(FcallExp* fc)    = 0;
    virtual int visit(ReturnStm* r)    = 0;
    virtual int visit(FunDec* fd)      = 0;
    virtual int visit(DoWhileStm* stm) = 0;
    virtual int visit(BreakStm* stm)   = 0;
    virtual int visit(SwitchStm* stm)  = 0;
};

// =============================================================================
// TypeCheckerVisitor — Análisis semántico
// =============================================================================

class TypeCheckerVisitor : public Visitor {
public:
    // Mapa función → número total de variables en el frame (parámetros + locales)
    std::unordered_map<std::string, int> funcontador;

    // Mapa función → número de parámetros (para verificar aridad en llamadas)
    std::unordered_map<std::string, int> funAridad;

    // Contador de variables locales del cuerpo actual (acumulado por visit(VarDec))
    int locales;

    // Entorno de variables: lleva el alcance (scope) actual
    Environment<int> entorno;

    // Nombre de la función que se está analizando (para mensajes de error)
    std::string funcionActual;

    // Punto de entrada del análisis semántico
    int TypeChecker(Program* program);

    // ---- Visitas ----
    int visit(BinaryExp* exp) override;
    int visit(UnaryExp* exp)  override;
    int visit(NumberExp* exp) override;
    int visit(IdExp* exp)     override;
    int visit(Program* p)     override;
    int visit(PrintStm* stm)  override;
    int visit(AssignStm* stm) override;
    int visit(WhileStm* stm)   override;
    int visit(IfStm* stm)      override;
    int visit(Body* body)      override;
    int visit(VarDec* vd)      override;
    int visit(FcallExp* fc)    override;
    int visit(ReturnStm* r)    override;
    int visit(FunDec* fd)      override;
    int visit(DoWhileStm* stm) override;
    int visit(BreakStm* stm)   override;
    int visit(SwitchStm* stm)  override;
};

// =============================================================================
// GenCodeVisitor — Generación de código ensamblador x86-64
// =============================================================================

class GenCodeVisitor : public Visitor {
private:
    std::ostream& out;      // Stream de salida (archivo .s)

public:
    TypeCheckerVisitor tipos;   // Resultado del análisis semántico

    // Mapa función → tamaño del frame (obtenido del TypeChecker)
    std::unordered_map<std::string, int> funcontador;

    // Mapa variable local → offset desde %rbp
    std::unordered_map<std::string, int> memoria;

    // Conjunto de variables globales
    std::unordered_map<std::string, bool> memoriaGlobal;

    int  offset        = -8;    // Próximo offset disponible en el frame local
    int  labelcont     = 0;     // Contador de etiquetas únicas para if/while
    bool entornoFuncion = false; // ¿Estamos dentro de una función?
    std::string nombreFuncion;  // Nombre de la función actual
    std::vector<std::string> breakLabels;  // etiquetas de salida de loops (para break)

    GenCodeVisitor(std::ostream& out) : out(out) {}

    // Punto de entrada de la generación
    int generar(Program* program);

    // ---- Visitas ----
    int visit(BinaryExp* exp) override;
    int visit(UnaryExp* exp)  override;
    int visit(NumberExp* exp) override;
    int visit(IdExp* exp)     override;
    int visit(Program* p)     override;
    int visit(PrintStm* stm)  override;
    int visit(AssignStm* stm) override;
    int visit(WhileStm* stm)   override;
    int visit(IfStm* stm)      override;
    int visit(Body* body)      override;
    int visit(VarDec* vd)      override;
    int visit(FcallExp* fc)    override;
    int visit(ReturnStm* r)    override;
    int visit(FunDec* fd)      override;
    int visit(DoWhileStm* stm) override;
    int visit(BreakStm* stm)   override;
    int visit(SwitchStm* stm)  override;
};

#endif // VISITOR_H
