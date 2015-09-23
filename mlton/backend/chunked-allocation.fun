functor ChunkedAllocation (S: RSSA_TRANSFORM_STRUCTS): RSSA_TRANSFORM =
struct
open S
open Rssa

fun insertChunkedAllocation (Block.T {args, kind, label, statements, transfer}) =
  [Block.T { args = args
           , kind = kind
           , label = label
           , statements = statements
           , transfer = transfer } ]

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
