// slang-ast-val.h

#pragma once

#include "slang-ast-base.h"

namespace Slang {

// Syntax class definitions for compile-time values.

// A compile-time integer (may not have a specific concrete value)
class IntVal : public Val 
{
    SLANG_ABSTRACT_CLASS(IntVal)
};

// Trivial case of a value that is just a constant integer
class ConstantIntVal : public IntVal 
{
    SLANG_CLASS(ConstantIntVal)

    IntegerLiteralValue value;

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();

protected:
    ConstantIntVal(IntegerLiteralValue inValue)
        : value(inValue)
    {}

};

// The logical "value" of a reference to a generic value parameter
class GenericParamIntVal : public IntVal 
{
    SLANG_CLASS(GenericParamIntVal)

    DeclRef<VarDeclBase> declRef;

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);

protected:
    GenericParamIntVal(DeclRef<VarDeclBase> inDeclRef)
        : declRef(inDeclRef)
    {}
};

    /// An unknown integer value indicating an erroneous sub-expression
class ErrorIntVal : public IntVal 
{
    SLANG_CLASS(ErrorIntVal)

    // TODO: We should probably eventually just have an `ErrorVal` here
    // and have all `Val`s that represent ordinary values hold their
    // `Type` so that we can have an `ErrorVal` of any type.

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

// A witness to the fact that some proposition is true, encoded
// at the level of the type system.
//
// Given a generic like:
//
//     void example<L>(L light)
//          where L : ILight
//     { ... }
//
// a call to `example()` needs two things for us to be sure
// it is valid:
//
// 1. We need a type `X` to use as the argument for the
//    parameter `L`. We might supply this explicitly, or
//    via inference.
//
// 2. We need a *proof* that whatever `X` we chose conforms
//    to the `ILight` interface.
//
// The easiest way to make such a proof is by construction,
// and a `Witness` represents such a constructive proof.
// Conceptually a proposition like `X : ILight` can be
// seen as a type, and witness prooving that proposition
// is a value of that type.
//
// We construct and store witnesses explicitly during
// semantic checking because they can help us with
// generating downstream code. By following the structure
// of a witness (the structure of a proof) we can, e.g.,
// navigate from the knowledge that `X : ILight` to
// the concrete declarations that provide the implementation
// of `ILight` for `X`.
// 
class Witness : public Val 
{
    SLANG_ABSTRACT_CLASS(Witness)
};

// A witness that one type is a subtype of another
// (where by "subtype" we include both inheritance
// relationships and type-conforms-to-interface relationships)
//
// TODO: we may need to tease those apart.
class SubtypeWitness : public Witness 
{
    SLANG_ABSTRACT_CLASS(SubtypeWitness)

    Type* sub = nullptr;
    Type* sup = nullptr;
};

class TypeEqualityWitness : public SubtypeWitness 
{
    SLANG_CLASS(TypeEqualityWitness)

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

// A witness that one type is a subtype of another
// because some in-scope declaration says so
class DeclaredSubtypeWitness : public SubtypeWitness 
{
    SLANG_CLASS(DeclaredSubtypeWitness)

    DeclRef<Decl> declRef;

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

// A witness that `sub : sup` because `sub : mid` and `mid : sup`
class TransitiveSubtypeWitness : public SubtypeWitness 
{
    SLANG_CLASS(TransitiveSubtypeWitness)

    // Witness that `sub : mid`
    SubtypeWitness* subToMid = nullptr;

    // Witness that `mid : sup`
    DeclRef<Decl> midToSup;

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

// A witness taht `sub : sup` because `sub` was wrapped into
// an existential of type `sup`.
class ExtractExistentialSubtypeWitness : public SubtypeWitness 
{
    SLANG_CLASS(ExtractExistentialSubtypeWitness)

    // The declaration of the existential value that has been opened
    DeclRef<VarDeclBase> declRef;

    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

// A witness that `sub : sup`, because `sub` is a tagged union
// of the form `A | B | C | ...` and each of `A : sup`,
// `B : sup`, `C : sup`, etc.
//
class TaggedUnionSubtypeWitness : public SubtypeWitness 
{
    SLANG_CLASS(TaggedUnionSubtypeWitness)

    // Witnesses that each of the "case" types in the union
    // is a subtype of `sup`.
    //
    List<Val*> caseWitnesses;


    // Overrides should be public so base classes can access
    bool _equalsValOverride(Val* val);
    String _toStringOverride();
    HashCode _getHashCodeOverride();
    Val* _substituteImplOverride(ASTBuilder* astBuilder, SubstitutionSet subst, int* ioDiff);
};

} // namespace Slang
