#pragma once
#include "lexer/token.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace nexus {

    enum class TypeKind {
        I64, F64, Bool, String, Void, Pointer, Struct, Enum, Array, Unknown
    };

    struct Type {

        TypeKind kind=TypeKind::Unknown;

        std::string name;

        std::shared_ptr<Type> pointee;

        std::shared_ptr<Type> element;

        std::size_t arraySize=0;

        static Type i64(){
            return {
                TypeKind::I64,{
                },{
                },{
                },0
            };
        }
        static Type f64(){
            return {
                TypeKind::F64,{
                },{
                },{
                },0
            };
        }
        static Type boolean(){
            return {
                TypeKind::Bool,{
                },{
                },{
                },0
            };
        }
        static Type string(){
            return {
                TypeKind::String,{
                },{
                },{
                },0
            };
        }
        static Type void_(){
            return {
                TypeKind::Void,{
                },{
                },{
                },0
            };
        }
        static Type ptr(Type t){
            Type x;
            x.kind=TypeKind::Pointer;
            x.pointee=std::make_shared<Type>(std::move(t));
            return x;
        }
        static Type named(std::string n){
            Type x;
            x.kind=TypeKind::Struct;
            x.name=std::move(n);
            return x;
        }
        static Type enumerated(std::string n){
            Type x;
            x.kind=TypeKind::Enum;
            x.name=std::move(n);
            return x;
        }
        static Type array(Type t,std::size_t n){
            Type x;
            x.kind=TypeKind::Array;
            x.element=std::make_shared<Type>(std::move(t));
            x.arraySize=n;
            return x;
        }
    };

    inline bool sameType(const Type&a,const Type&b){

        if(a.kind!=b.kind) return false;

        if(a.kind==TypeKind::Struct||a.kind==TypeKind::Enum) return a.name==b.name;

        if(a.kind==TypeKind::Pointer) return a.pointee && b.pointee && sameType(*a.pointee,*b.pointee);

        if(a.kind==TypeKind::Array) return a.arraySize==b.arraySize && a.element && b.element && sameType(*a.element,*b.element);

        return true;

    }

    struct Expr {
        virtual ~Expr()=default;
        using Ptr=std::unique_ptr<Expr>;
    };

    struct IntExpr:Expr{
        long long value;
        explicit IntExpr(long long v):value(v){
        }
    };

    struct FloatExpr:Expr{
        double value;
        explicit FloatExpr(double v):value(v){
        }
    };

    struct BoolExpr:Expr{
        bool value;
        explicit BoolExpr(bool v):value(v){
        }
    };

    struct StringExpr:Expr{
        std::string value;
        explicit StringExpr(std::string v):value(std::move(v)){
        }
    };

    struct VarExpr:Expr{
        std::string name;
        explicit VarExpr(std::string n):name(std::move(n)){
        }
    };

    struct EnumVariantExpr:Expr{
        std::string enumName,variant;
        EnumVariantExpr(std::string e,std::string v):enumName(std::move(e)),variant(std::move(v)){
        }
    };

    struct ArrayExpr:Expr{
        std::vector<Expr::Ptr> elements;
    };

    struct IndexExpr:Expr{
        Expr::Ptr object,index;
        IndexExpr(Expr::Ptr o,Expr::Ptr i):object(std::move(o)),index(std::move(i)){
        }
    };

    struct UnaryExpr:Expr{
        TokenKind op;
        Expr::Ptr rhs;
        UnaryExpr(TokenKind o,Expr::Ptr r):op(o),rhs(std::move(r)){
        }
    };

    struct BinaryExpr:Expr{
        TokenKind op;
        Expr::Ptr lhs,rhs;
        BinaryExpr(TokenKind o,Expr::Ptr l,Expr::Ptr r):op(o),lhs(std::move(l)),rhs(std::move(r)){
        }
    };

    struct CallExpr:Expr{
        std::string callee;
        std::vector<Expr::Ptr> args;
        CallExpr(std::string c,std::vector<Expr::Ptr>a):callee(std::move(c)),args(std::move(a)){
        }
    };

    struct MemberExpr:Expr{
        Expr::Ptr object;
        std::string member;
        MemberExpr(Expr::Ptr o,std::string m):object(std::move(o)),member(std::move(m)){
        }
    };

    struct StructInitExpr:Expr{
        std::string typeName;
        std::vector<std::pair<std::string,Expr::Ptr>> fields;
        explicit StructInitExpr(std::string n):typeName(std::move(n)){
        }
    };

    struct Stmt {
        virtual ~Stmt()=default;
        using Ptr=std::unique_ptr<Stmt>;
    };

    struct Block:Stmt{
        std::vector<Stmt::Ptr> statements;
    };

    struct LetStmt:Stmt{
        std::string name;
        bool mut=false;
        Expr::Ptr init;
        Type type{
        };
    };

    struct AssignStmt:Stmt{
        std::string name;
        Expr::Ptr value;
    };

    struct MemberAssignStmt:Stmt{
        Expr::Ptr object;
        std::string member;
        Expr::Ptr value;
    };

    struct IndexAssignStmt:Stmt{
        Expr::Ptr object,index,value;
    };

    struct ExprStmt:Stmt{
        Expr::Ptr expr;
    };

    struct ReturnStmt:Stmt{
        Expr::Ptr expr;
    };

    struct IfStmt:Stmt{
        Expr::Ptr cond;
        std::unique_ptr<Block> thenBlock;
        std::unique_ptr<Block> elseBlock;
    };

    struct WhileStmt:Stmt{
        Expr::Ptr cond;
        std::unique_ptr<Block> body;
    };

    struct BreakStmt:Stmt{
    };
    struct ContinueStmt:Stmt{
    };

    struct MatchArm {
        Expr::Ptr pattern;
        std::unique_ptr<Block> body;
        bool wildcard=false;
    };

    struct MatchStmt:Stmt{
        Expr::Ptr value;
        std::vector<MatchArm> arms;
    };

    struct Param{
        std::string name;
        Type type;
    };

    struct StructField{
        std::string name;
        Type type;
    };

    struct StructDecl{
        std::string name;
        std::vector<StructField> fields;
    };

    struct EnumDecl{
        std::string name;
        std::vector<std::string> variants;
    };

    struct Function{
        std::string name;
        std::vector<Param> params;
        Type ret;
        std::unique_ptr<Block> body;
        bool external=false;
    };

    struct Program{
        std::vector<std::string> imports;
        std::vector<StructDecl> structs;
        std::vector<EnumDecl> enums;
        std::vector<Function> functions;
    };

}
