functor ChunkedRepresentation (S: REPRESENTATION_STRUCTS): REPRESENTATION =
struct

open S

local
    open Rssa
in
    structure Block = Block
    structure Kind = Kind
    structure Label = Label
    structure ObjectType = ObjectType
    structure Operand = Operand
    structure ObjptrTycon = ObjptrTycon
    structure Prim = Prim
    structure RealSize = RealSize
    structure Runtime = Runtime
    structure Scale = Scale
    structure Statement = Statement
    structure Switch = Switch
    structure Transfer = Transfer
    structure Type = Type
    structure Var = Var
    structure WordSize = WordSize
    structure WordX = WordX
end

structure S = Ssa
local
    open Ssa
in
   structure Base = Base
   structure Con = Con
   structure ObjectCon = ObjectCon
   structure Prod = Prod
   structure Tycon = Tycon
end

datatype z = datatype Operand.t
datatype z = datatype Statement.t
datatype z = datatype Transfer.t

structure Rep =
   struct
      datatype rep = NonObjptr
                   | Objptr
      datatype t = T of { rep: rep
                        , ty: Type.t}
      fun layout (T {rep, ty}) =
         let
            open Layout
         in
            record [ ("rep",
                      case rep of
                          NonObjptr => str "NonObjptr"
                        | Objptr  => str "Objptr")
                   , ("ty", Type.layout ty) ]
         end

      local
         fun make f (T r) = f r
      in
         val ty = make #ty
         val rep = make #rep
      end

      fun equals (r, r') = Type.equals (ty r, ty r')

      val equals =
         Trace.trace2
         ("ChunkedRepresentation.Rep.equals", layout, layout, Bool.layout)
         equals

      fun nonObjptr ty = T { rep = NonObjptr
                           , ty = ty}

      val bool = nonObjptr Type.bool

      val width = Type.width o ty

      val unit = T { rep = NonObjptr
                   , ty = Type.unit}

      fun isObjptr (T {rep, ...}) =
         case rep of
            Objptr => true
          | _ => false
   end

structure Component =
   struct
      datatype t = Direct of { index: int
                             , rep: Rep.t }
      fun layout (Direct {index, rep}) =
         let
            open Layout
         in
             seq [ str "Direct "
                 , record [ ("index", Int.layout index)
                          , ("rep", Rep.layout rep)] ]
         end
      fun rep (Direct {rep, ...}) = rep
      val ty = Rep.ty o rep
      fun equals (Direct {rep = r1, ...}, Direct {rep = r2, ...}) =
        Rep.equals (r1, r2)
   end

fun compute(program as Ssa.Program.T {datatypes, ...}) =
  let
      fun diagnostic () = ()
      fun object {args, con, dst, objectTy, oper} = []
      val objectTypes = Vector.fromList []
      fun select {base, baseTy, dst, offset} = []
      fun toRtype (t: S.Type.t): Type.t option = NONE
      fun update {base, baseTy, offset, value} = []
      fun genCase _ = Error.bug "ChunkedRepresentation.compute.genCase"
  in
      { diagnostic = diagnostic
      , genCase = genCase
      , object = object
      , objectTypes = objectTypes
      , select = select
      , toRtype = toRtype
      , update = update
      }
  end

end
