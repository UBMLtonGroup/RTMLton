functor ChunkedAllocation (S: RSSA_TRANSFORM_STRUCTS): RSSA_TRANSFORM =
struct
open S
open Rssa

structure CFunction =
struct
open CFunction Type.BuiltInCFunction
end

structure Statement =
struct
open Statement

fun objChunksAllocated (s: t): word =
  case s of
      ChunkedObject {numChunks, ...} => numChunks
    | _ => Word.fromInt 0
end

structure Block =
struct
open Block

fun objChunksAllocated (T {statements, ...}): word =
  Vector.fold (statements, Word.fromInt 0,
               fn (s, acc) => Word.+ (Statement.objChunksAllocated s, acc))
end



fun insertChunkedAllocation (b as Block.T {args, kind, label, statements, transfer}) =
  let
      val dontCollect = Label.newNoname ()
      val collect = Label.newNoname ()
      val func = CFunction.gc {maySwitchThreads = false}

      fun gcAndJumpToBlock (to) =
        let
            val collectReturn = Label.newNoname ()
        in
            [ Block.T {args = Vector.new0 (),
                       kind = Kind.Jump,
                       label = collect,
                       statements = Vector.new0 (),
                       transfer = (Transfer.CCall
                                       {args = Vector.new3 (Operand.GCState,
                                                            (Operand.word
                                                                 (WordX.zero (WordSize.csize ()))),
                                                            Operand.bool false),
                                        func = func,
                                        return = SOME collectReturn})}
            , Block.T { args = Vector.new0 ()
                      , kind = Kind.CReturn {func = func}
                      , label = collectReturn
                      , statements = Vector.new0 ()
                      , transfer = Transfer.Goto { dst = to
                                                 , args = Vector.new0 () }} ]
        end
      fun primApp (prim, op1, op2, {collect, dontCollect}) =
        let
            val res = Var.newNoname ()
            val s =
                Statement.PrimApp {args = Vector.new2 (op1, op2),
                                   dst = SOME (res, Type.bool),
                                   prim = prim}
            val transfer =
                Transfer.ifBool
                    (Operand.Var {var = res, ty = Type.bool},
                     {falsee = dontCollect,
                      truee = collect})
        in
            (Vector.new1 s, transfer)
        end

      datatype z = datatype Runtime.GCField.t

      val (ss, ts) = primApp (Prim.wordLt (WordSize.csize (), {signed = false}),
                              Operand.Runtime FLChunks,
                              Operand.constWord Runtime.objChunkSlop (WordSize.csize ()),
                              {collect=collect, dontCollect=dontCollect})
      val startBlock =
          if Word.<= (Block.objChunksAllocated b, Runtime.objChunkSlop)
          then Block.T { args = args
                       , kind = kind
                       , label = label
                       , statements = ss
                       , transfer = ts }
          else Block.T { args = args
                       , kind = kind
                       , label = label
                       , statements = Vector.new0 ()
                       , transfer = Transfer.Goto { dst = collect
                                                  , args = Vector.new0 () }}
  in
      if Block.objChunksAllocated b = Word.fromInt 0
      then [b]
      else [ startBlock ] @
           gcAndJumpToBlock dontCollect @
           [Block.T { args = Vector.new0 ()
                    , kind = Kind.Jump
                    , label = dontCollect
                    , statements = statements
                    , transfer = transfer }]
  end

fun handleFunction (f: Function.t): Function.t =
  let
      val {args, blocks, name, raises, returns, start} = Function.dest f
      val newBlocks = ref []
      val () = Vector.foreach
               (blocks,
                fn block =>
                   List.push (newBlocks, insertChunkedAllocation block))
  in
      Function.new { args = args
                   , blocks = Vector.fromList (List.concat (!newBlocks))
                   , name = name
                   , raises = raises
                   , returns = returns
                   , start = start }
  end

fun transform (program as Program.T {functions, handlesSignals, main, objectTypes}) =
  Program.T { functions = List.map (functions, handleFunction)
            , handlesSignals = handlesSignals
            , main = handleFunction (main)
            , objectTypes = objectTypes }

end
