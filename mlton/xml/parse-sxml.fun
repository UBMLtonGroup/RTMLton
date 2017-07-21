(* Copyright (C) 2017 Jason Carr.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

functor ParseSxml (S: PARSE_SXML_STRUCTS): PARSE_SXML =
struct
   open S
   open XmlTree
   structure P = Parse
   open P.Ops
   infix 1 <|> >>=
   infix 2 <&>
   infix  3 <*> <* *>
   infixr 4 <$> <$$> <$$$> <$

   fun isInfixChar b = case List.index
      (String.explode "!%&$#+-/:<=>?@\\~'^|*",
       fn c => b = c) of
          SOME _ => true
        | NONE   => false

   fun isIdentFirst b = Char.isAlpha b orelse b = #"'"
   fun isIdentRest b = Char.isAlphaNum b orelse b = #"'" orelse b = #"_" orelse b = #"."

  val stringToken = (fn (x, y) => [x, y]) <$$> (P.char #"\\", P.next) <|>
                     (fn x      => [x]   ) <$> P.next
   fun skipComments () = P.any
      [P.string "(*" *> P.cut (P.manyCharsFailing(P.string "*)" <|> P.string "(*") *>
            ((P.string "*)" <|> "" <$ P.delay skipComments) *> P.each [P.next]
          <|> P.failCut "Closing comment")),
       List.concat <$> P.each
         [P.each [P.char #"\""],
          List.concat <$> P.manyFailing(stringToken, P.char #"\""),
          P.each [P.char #"\""]],
       P.each [P.next]]

   val space = P.sat(P.next, Char.isSpace)
   val spaces = P.many(space)
   fun token s = P.notFollowedBy
      (P.string s,
       (P.char #"_") <|> (P.sat(P.next,Char.isAlphaNum))) <* spaces
   fun symbol s = P.notFollowedBy
      (P.string s,
       (P.sat(P.next,fn b => isInfixChar b orelse b = #"_"))) <* spaces

   val clOptions = P.manyCharsFailing(P.string "Datatypes:")

   fun 'a makeNameResolver(f: string -> 'a): string -> 'a =
      let
         val hash = String.hash
         val map = HashSet.new{hash= hash o #1}
         fun eq x (a: string * 'a) = String.equals(x, #1 a)
      in
         fn x => (#2 o HashSet.lookupOrInsert)(map, hash x, eq x, fn () => (x, f x))
      end


   val ident = (
      String.implode <$> (P.any
         [(op ::) <$$>
             (P.sat(P.next, isIdentFirst),
              P.many (P.sat(P.next, isIdentRest))),
          List.append <$$>
             (P.many1 (P.sat(P.next, isInfixChar)),
              (op ::) <$$> (P.char #"_", P.many (P.sat(P.next, Char.isDigit))) <|> P.pure []
              (* just for collecting _0 *)
              )])) <|> P.failCut "identifier"



   (* parse a tuple of parsers which must begin with a paren but may be unary *)
   fun tupleOf p = Vector.fromList <$>
      (P.char #"(" *> P.sepBy(spaces *> p, P.char #",") <* P.char #")")

   fun vectorOf p = Vector.fromList <$>
      (P.char #"[" *> P.sepBy(spaces *> p, P.char #",") <* P.char #"]")

   fun doneRecord' () = P.char #"{" <* P.many(P.delay doneRecord' <|> P.failing(P.char #"}") *> P.next) <* P.char #"}"
   val doneRecord = doneRecord' ()
   fun fromRecord name p = P.peek
      (P.char #"{" *> P.many (() <$ P.delay doneRecord' <|> () <$ P.failing (token name <* symbol "=") <* P.next)
       *> token name *> symbol "=" *> p)

   fun casesOf(con, left, right) = Vector.fromList <$> P.sepBy1
      (left <* spaces <* token "=>" >>= (fn l =>
         right >>= (fn r => con (l, r))),
       spaces)

   fun optionOf p = SOME <$> (token "Some" *> P.cut(p)) <|> NONE <$ token "None"


   fun possibly t = t >>= (fn x => case x of NONE => P.fail "Syntax error"
                                           | SOME y => P.pure y)
   val parseInt = possibly ((Int.fromString o String.implode) <$>
         P.many (P.sat(P.next, Char.isDigit))) <|> P.failCut "integer"
   (* too many arguments for the maps, curried to use <*> instead *)
   fun makeTyp resolveTycon (args, ident) = Type.con (resolveTycon ident, args)

   local
      fun typ' resolveTycon () = (makeTyp resolveTycon) <$$>
         (((tupleOf (P.delay (typ' resolveTycon))) <|> P.pure (Vector.new0 ())),
         (spaces *> ident <* spaces))
   in
      fun typ resolveTycon = typ' resolveTycon ()
   end

   val ctype = (P.any o List.map)
               (CType.all, fn ct =>
                ct <$ token (CType.toString ct))

   fun makeCon resolveCon (name, arg) = {con = resolveCon name, arg = arg}

   (* parse in a constructor (to Con.t) *)
   fun constructor resolveCon resolveTycon = (makeCon resolveCon) <$$>
      (ident <* spaces,
      P.optional (token "of" *> (typ resolveTycon) <* spaces) )


   fun makeDt resolveTycon (tycon, cons) =
      {tyvars = Vector.new0 (),
       tycon = resolveTycon tycon, cons = cons}

   fun datatyp resolveCon resolveTycon = (makeDt resolveTycon) <$$>
      ((spaces *> ident <* spaces <* symbol "="),
       (Vector.fromList <$> P.sepBy1
          ((constructor resolveCon resolveTycon) <* spaces,
           P.char #"|" *> spaces)))

   fun datatypes resolveCon resolveTycon =
      token "Datatypes:" *> Vector.fromList <$> P.many (datatyp resolveCon resolveTycon)

   fun overflow resolveVar = P.string "Overflow:" *> spaces *> (
          (P.string "Some" *> spaces *> SOME <$> resolveVar <$> ident <* spaces) <|>
          (NONE <$ P.string "None" <* spaces))


   val parseString = possibly ((String.fromString o String.implode o List.concat) <$>
         (P.char #"\"" *> (P.manyFailing(stringToken, P.char #"\"")) <* P.char #"\""))
   val parseIntInf = possibly ((IntInf.fromString o String.implode) <$>
         P.many (P.sat(P.next, fn c => Char.isDigit c orelse c = #"~")))
   fun makeReal s = (case (RealX.make s) of NONE => NONE | x => x) handle Fail _ => NONE
   fun parseReal sz = possibly (makeReal <$$> (String.implode <$>
         List.concat <$> P.each
         [P.many (P.sat(P.next, fn c => Char.isDigit c orelse c = #"~")),
          P.char #"." *> P.pure [#"."] <|> P.pure [],
          P.many (P.sat(P.next, fn c => Char.isDigit c orelse c = #"E" orelse c = #"~"))],
         P.pure sz))
   val parseHex = P.fromReader (IntInf.scan(StringCvt.HEX, P.toReader P.next))
   val parseBool = true <$ token "true" <|> false <$ token "false"

   fun makeWord typ int =
      if Tycon.isWordX typ
         then P.pure (WordX.fromIntInf(int, (Tycon.deWordX typ)))
         else P.fail "Invalid word"
   val parseWord8Vector = WordXVector.fromVector <$$>
        (P.pure {elementSize=WordSize.word8},
         P.char #"#" *> vectorOf (parseHex >>= makeWord (Tycon.word WordSize.word8)))

   fun exp resolveCon resolveTycon resolveVar =
      let
         fun makeLet(decs, result) = Exp.make {decs=decs, result=result}
         fun makeExnDec(ca) = Dec.Exception ca
         fun makeValDec(var, ty, exp) = Dec.MonoVal {exp=exp, ty=ty, var=var}
         fun makeFunDecs(lambdas) = Dec.Fun {decs=Vector.fromList lambdas, tyvars=Vector.new0 ()}
         fun makeFunDec((var, ty), lambda) = {lambda=lambda, ty=ty, var=var}
         val var = resolveVar <$> ident <* spaces
         val typedvar = (fn (x,y) => (x,y)) <$$>
            (var,
             symbol ":" *> (typ resolveTycon) <* spaces)
         fun makeVarExp var = VarExp.T {var=var, targs = Vector.new0 ()}
         val varExp =
            P.failing (token "in" <|> token "exception" <|> token "val") *>
            makeVarExp <$> var
         fun makeApp(func, arg) = {arg=arg, func=func}
         fun makeConApp(con, targs, arg) = {arg=arg, con=con, targs=targs}
         fun conApp v = makeConApp <$$$>
            (resolveCon <$> ident <* spaces,
             P.pure (Vector.new0 ()),
             P.optional v)
         val conAppExp = token "new" *> P.cut (conApp varExp)
         fun constExp typ =
            if Tycon.isWordX typ then
               Const.Word <$> (P.string "0x" *> parseHex >>= makeWord typ) <|> P.failCut "word"
            else if Tycon.isRealX typ then
               Const.Real <$> parseReal (Tycon.deRealX typ) <|> P.failCut "real"
            else if Tycon.isIntX typ then
               Const.IntInf <$> parseIntInf <|> P.failCut "integer"
            else if Tycon.equals(typ, Tycon.vector) then
               (* assume it's a word8 vector *)
               P.any
               [Const.string <$> parseString,
                Const.wordVector <$> parseWord8Vector,
                P.failCut "string constant"]

            else
               P.fail "constant"
         fun makeRaise(NONE, exn) = {exn=exn, extend=false}
           | makeRaise(SOME _, exn) = {exn=exn, extend=true}
         val raiseExp = token "raise" *> P.cut (makeRaise <$$> (P.optional (token "extend"), varExp <* spaces))
         fun makeHandle(try, catch, handler) = {catch=catch, handler=handler, try=try}
         fun makeLambda(mayInline, (var, typ), exp) = Lambda.make {arg=var, argType=typ, body=exp, mayInline=mayInline}

         val parseConvention = CFunction.Convention.Cdecl <$ token "cdecl" <|>
                                    CFunction.Convention.Stdcall <$ token "stdcall"
         fun makeRuntimeTarget bytes ens mayGC maySwitch modifies readsSt writesSt =
            CFunction.Kind.Runtime ({bytesNeeded=bytes, ensuresBytesFree=ens,
            mayGC=mayGC, maySwitchThreads=maySwitch, modifiesFrontier=modifies,
            readsStackTop=readsSt, writesStackTop=writesSt})
         val parseRuntimeTarget = makeRuntimeTarget
            <$> fromRecord "bytesNeeded" (optionOf parseInt)
            <*> fromRecord "ensuresBytesFree" parseBool
            <*> fromRecord "mayGC" parseBool
            <*> fromRecord "maySwitchThreads" parseBool
            <*> fromRecord "modifiesFrontier" parseBool
            <*> fromRecord "readsStackTop" parseBool
            <*> fromRecord "writesStackTop" parseBool
            <* doneRecord
         val parseKind = CFunction.Kind.Impure <$ token "Impure" <|>
                         CFunction.Kind.Pure <$ token "Pure" <|>
                         token "Runtime" *> P.cut parseRuntimeTarget

         val parsePrototype = (fn x => x) <$$>
            (fromRecord "args" (tupleOf ctype),
             fromRecord "res" (optionOf ctype)) <* doneRecord
         val parseSymbolScope = P.any
            [CFunction.SymbolScope.External <$ token "external",
             CFunction.SymbolScope.Private <$ token "private",
             CFunction.SymbolScope.Public <$ token "public"]


         val parseTarget = CFunction.Target.Indirect <$ symbol "<*>" <|>
                           CFunction.Target.Direct <$> ident
         fun makeFFI args conv kind prototype return symbolScope target =
            Prim.ffi (CFunction.T
               {args=args, convention=conv, kind=kind,
                prototype=prototype, return=return, symbolScope=symbolScope,
                target = target})
         val resolveFFI = token "FFI" *> P.cut(
            makeFFI
            <$> fromRecord "args" (tupleOf (typ resolveTycon))
            <*> fromRecord "convention" parseConvention
            <*> fromRecord "kind" parseKind
            <*> fromRecord "prototype" parsePrototype
            <*> fromRecord "return" (typ resolveTycon)
            <*> fromRecord "symbolScope" parseSymbolScope
            <*> fromRecord "target" parseTarget
            <* doneRecord)
         fun makeFFISym name cty symbolScope = Prim.ffiSymbol {name=name, cty=cty, symbolScope=symbolScope}
         val resolveFFISym = token "FFI_Symbol" *> P.cut(
            makeFFISym
            <$> fromRecord "name" ident
            <*> fromRecord "cty" (optionOf ctype)
            <*> fromRecord "symbolScope" parseSymbolScope
            <* doneRecord)

         fun resolvePrim p = case Prim.fromString p
            of SOME p' => P.pure p'
             | NONE => P.fail ("valid primitive, got " ^ p)
         fun makePrimApp(prim, targs, args) = {args=args, prim=prim, targs=targs}
         val primAppExp = token "prim" *> P.cut (makePrimApp <$$$>
            (P.any [
               resolveFFI,
               resolveFFISym,
               (ident <* spaces >>= resolvePrim)],
             (vectorOf (typ resolveTycon) <|> P.pure (Vector.new0 ())) <* spaces,
             tupleOf varExp <* spaces))
         fun makeSelect(offset, var) = {offset=offset, tuple=var}
         val selectExp = symbol "#" *> P.cut(makeSelect <$$>
            (parseInt <* spaces,
             varExp))
         val profileExp = (ProfileExp.Enter <$> (token "Enter" *> SourceInfo.fromC <$> P.info) <|>
                           ProfileExp.Leave <$> (token "Leave" *> SourceInfo.fromC <$> P.info ))
            <* P.char #"<" <* P.manyCharsFailing(P.char #">") <* P.char #">" <* spaces
         fun makeConCases var (cons, def) =
            {test=var,
             cases=Cases.Con cons,
             default=Option.map(def, fn x => (x, Region.bogus))}
         fun makeWordCases var s (wds, def) =
            {test=var,
             cases=Cases.Word (case s of
                 8 => Type.WordSize.word8
               | 16 => Type.WordSize.word16
               | 32 => Type.WordSize.word32
               | 64 => Type.WordSize.word64
               | _ => raise Fail "makeWordCases" (* can't happen *)
               , wds),
             default=Option.map(def, fn x => (x, Region.bogus))}
         fun makePat(con, exp) = P.pure (Pat.T con, exp)
         fun makeCaseWord size (int, exp) = case size of
             (* this is repetetive, but it's a bit awkward to rework around the fail *)
            8 => P.pure ((WordX.fromIntInf(int, Type.WordSize.word8)), exp)
          | 16 => P.pure ((WordX.fromIntInf(int, Type.WordSize.word16)), exp)
          | 32 => P.pure ((WordX.fromIntInf(int, Type.WordSize.word32)), exp)
          | 64 => P.pure ((WordX.fromIntInf(int, Type.WordSize.word64)), exp)
          | _ => P.fail "valid word size for cases (8, 16, 32 or 64)"


         fun exp' () = makeLet <$$>
            (token "let" *>
             P.many (dec ()) <* token "in", varExp <* P.string "end")
         and dec () = P.any
            [token "exception" *>
             P.cut (makeExnDec <$> (constructor resolveCon resolveTycon)),
             token "val" *> token "rec" *>
             P.cut (makeFunDecs <$>
                    P.many (makeFunDec <$$> (typedvar <* symbol "=" <* spaces, lambdaExp ()))),
             token "val" *>
             P.cut (makeValDec <$>
                    (typedvar >>= (fn (var, ty) =>
                     (symbol "=" *> primexp ty <* spaces) >>= (fn primexp =>
                      P.pure (var, ty, primexp)))))]
         and primexp typ = P.any
            [PrimExp.Case <$> casesExp (),
             PrimExp.ConApp <$> conAppExp,
             PrimExp.Lambda <$> lambdaExp (),
             (* const must come before select due to vector constants *)
             PrimExp.Const <$> constExp (Type.tycon typ),
             PrimExp.Handle <$> handleExp (),
             PrimExp.PrimApp <$> primAppExp,
             PrimExp.Profile <$> profileExp,
             PrimExp.Raise <$> raiseExp,
             PrimExp.Select <$> selectExp,
             PrimExp.Tuple <$> (tupleOf varExp) <* spaces,
             (* put these last, they just take identifiers so they're pretty greedy *)
             (* App *must* procede var, due to greediness *)
             PrimExp.App <$> makeApp <$$> (varExp, varExp),
             PrimExp.Var <$> varExp]
         and handleExp () = makeHandle <$$$>
            (P.delay exp' <* spaces,
             token "handle" *> P.cut typedvar,
             P.cut (token "=>" *> P.delay exp' <* spaces)
             )
         and lambdaExp () = token "fn" *> P.cut(makeLambda <$$$>
            (false <$ (token "noinline") <|> P.pure true,
             typedvar,
             token "=>" *> P.delay exp' <* spaces))
         and casesExp () = P.string "case" *>
            P.optional parseInt <* P.many1 space >>= (fn size => P.cut(
               varExp <* token "of" <* spaces >>= (fn var =>
                  case size of
                      NONE => makeConCases var <$$>
                        (casesOf(makePat, conApp typedvar, P.delay exp'),
                         spaces *> P.optional(token "_" *> token "=>" *> P.delay exp'))
                    | SOME s => makeWordCases var s <$$>
                        (casesOf(makeCaseWord s, P.string "0x" *> parseHex, P.delay exp'),
                         spaces *> P.optional(token "_" *> token "=>" *> P.delay exp'))
                      )))
      in
         exp' ()
      end

      fun body resolveCon resolveTycon resolveVar = P.string "Body:" *> spaces
         *> exp resolveCon resolveTycon resolveVar
         (*pure (Exp.fromPrimExp (PrimExp.Tuple (Vector.new0 ()),
   Type.tuple (Vector.new0 ())))*)

   fun makeProgram(datatypes, overflow, body) =
      Program.T
         {body = body,
          datatypes = datatypes,
          overflow = overflow}


   val program : Program.t Parse.t =
      let
         fun strip_unique s  = P.parse
            (String.implode <$> P.manyCharsFailing(
               P.char #"_" *> P.many1 (P.sat(P.next, Char.isDigit)) *> P.failing P.next),
             Stream.fromList (String.explode s))
         val resolveCon0 = makeNameResolver(Con.newString o strip_unique)
         fun resolveCon ident =
            case List.peek ([Con.falsee, Con.truee, Con.overflow, Con.reff], fn con =>
                            ident = Con.toString con) of
               SOME con => con
             | NONE => resolveCon0 ident
         val resolveTycon0 = makeNameResolver(Tycon.newString o strip_unique)
         fun resolveTycon ident =
            case List.peekMap (Tycon.prims, fn {name, tycon, ...} =>
                               if ident = name then SOME tycon else NONE) of
               SOME con => con
             | NONE => if ident = "unit"
                          then Tycon.tuple
                       else resolveTycon0 ident
         val resolveVar = makeNameResolver(Var.newString o strip_unique)
      in
         P.compose(skipComments (),
         clOptions *>
         (makeProgram <$$$>
            (datatypes resolveCon resolveTycon,
            overflow resolveVar,
            body resolveCon resolveTycon resolveVar <* spaces <* P.failing P.next)))
            (* failing next to check for end of file *)
      end

   fun parse s = P.parse(program, s)
end
