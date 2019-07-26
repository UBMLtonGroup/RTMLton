functor SplitBlocks (S: RSSA_TRANSFORM_STRUCTS): RSSA_TRANSFORM =
struct
open S
open Rssa

structure CFunction =
struct
open CFunction Type.BuiltInCFunction
end



fun addDummyTransfer (b as Block.T {args, kind, label, statements, transfer}) =

let



  (*val func = CFunction.gc {maySwitchThreads = false}*)
  val func = CFunction.dummyCCall ()
  

  val stlist = Vector.foldr (statements,[],(op ::)) 
  

  val newBlks = ref []


 fun buildABlock (st,l) = 
     let 
       val lbl = Label.newNoname ()
       val lbl2 = Label.newNoname ()
       val _ = List.push (newBlks, Block.T {args = args,
                       kind = Kind.Jump,
                       label = l,
                       statements = Vector.fromList (st),
                       transfer = (Transfer.CCall
                                       {args = Vector.new3 (Operand.GCState,
                                                            (Operand.word
                                                                 (WordX.zero (WordSize.csize ()))),
                                                            Operand.bool false),
                                        func = func,
                                        return = SOME lbl})} )

    val _ = List.push (newBlks, Block.T { args = Vector.new0 ()
                      , kind = Kind.CReturn {func = func}
                      , label = lbl
                      , statements = Vector.new0 ()
                      , transfer = Transfer.Goto { dst = lbl2
                                                 , args = Vector.new0 () }})
    in
        lbl2    
    end




  fun loop2 (ls,acc,l)= 
    case ls of 
         Statement.Dummy ::tl =>  loop2 (tl,[],buildABlock
         ((List.rev (Statement.Dummy ::acc)),l) )
       | hd :: tl => loop2 (tl,hd ::acc , l)
       | [] => (List.push (newBlks,
                            Block.T {args = args,
                                    kind = kind,
                                    label = l,
                                    statements = Vector.fromList (List.rev acc),
                                    transfer = transfer }) ; List.rev (!newBlks) )




  


(*Transfer to same block wont work since the label that common block would
* return from would be different each time. Besides the starting block will have
* to change for each old blokc and cannot be common*)

    
in 
      loop2 (stlist,[],label)
end

  
 fun handleFunction (f : Function.t) :Function.t = 
 let
  val {args, blocks, name, raises, returns, start} = Function.dest f
  val newBlocks = ref []
  val () = Vector.foreach
               (blocks,
                fn block =>
                   List.push (newBlocks, addDummyTransfer block))
 in
      Function.new { args = args
                   , blocks = Vector.fromList (List.concat (!newBlocks))
                   , name = name
                   , raises = raises
                   , returns = returns
                   , start = start }
  end


fun transform (program as Program.T {functions, handlesSignals, main, objectTypes}) =
  let 
    (*val _ = let open Control
                     in saveToFile
                        ({suffix = concat ["dummyTransfer", ".", "pre.rssa"]},
                         Control.No, program, Control.Layouts Program.layouts)
                     end*)

    val prog = Program.T { functions = List.map (functions, handleFunction)
            , handlesSignals = handlesSignals
            , main = handleFunction (main)
            , objectTypes = objectTypes }
    
    (*val _ = let open Control
                     in saveToFile
                        ({suffix = concat ["dummyTransfer", ".", "post.rssa"]},
                         Control.No, prog, Control.Layouts Program.layouts)
                     end*)
  in 
         prog

  end
 




end
