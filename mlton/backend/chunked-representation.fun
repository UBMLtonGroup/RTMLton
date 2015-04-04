function ChunkedRepresentation (S: REPRESENTATION_STRUCTS): REPRESENTATION =
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
    structure Runtime = Runtimen
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



end
