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


(*This pass checks the number of bytes allocated by each block and forcibly
* inserts a GC_collect point whenever free chunks in free list is lesser than
* chunk slop*)

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


fun addDummyTransfer (b as Block.T {args, kind, label, statements, transfer}) =

let



  (*val func = CFunction.gc {maySwitchThreads = false}*)
  val func = CFunction.dummyCCall ()
  (*fun head ls = 
    case ls of
         hd :: _ => hd
       | [] => []

  fun tail ls = 
    case ls of
         _ :: tl => tl
       | []=> []
  
  fun loop (i,acc,acc2) = 
    if ( i = (Vector.length statements))
    then 
      List.rev acc2
    else
    let 
      val s = Vector.sub (statements,i)
    in
      case s of 
           Dummy => loop (i+1,[], (List.rev acc) :: acc2)
         | _ => loop (i+1, (s::acc),acc2)
    end

  val stl = loop (0,[],[])

  val (startBlock,rest,l) = 
    let 
      val lbl = Label.newNoname () 
      val lbl2 = Label.newNoname ()

      val blk = [ Block.T {args = args,
                       kind = Kind.Jump,
                       label = label,
                       statements = Vector.fromList (head stl),
                       transfer = (Transfer.CCall
                                       {args = Vector.new3 (Operand.GCState,
                                                            (Operand.word
                                                                 (WordX.zero (WordSize.csize ()))),
                                                            Operand.bool false),
                                        func = func,
                                        return = SOME lbl})}
            , Block.T { args = Vector.new0 ()
                      , kind = Kind.CReturn {func = func}
                      , label = lbl
                      , statements = Vector.new0 ()
                      , transfer = Transfer.Goto { dst = lbl2
                                                 , args = Vector.new0 () }} ]
    in
      (blk,tail stl,lbl2)
    end
    *)



(*  fun split (st,bl,l) = 
    case st of 
        hd :: [] =>
           let 
             val blk = [ Block.T {args = Vector.new0 (),
                       kind = kind,
                       label = l,
                       statements = Vector.fromList hd,
                       transfer = transfer }]
           in 
             bl @blk
           end
           
       |hd :: tl => 
         let
            val lbl = Label.newNoname ()
            val lbl2 = Label.newNoname ()
            val blk = [ Block.T {args = Vector.new0 (),
                       kind = Kind.Jump,
                       label = l,
                       statements = Vector.fromList hd,
                       transfer = (Transfer.CCall
                                       {args = Vector.new3 (Operand.GCState,
                                                            (Operand.word
                                                                 (WordX.zero (WordSize.csize ()))),
                                                            Operand.bool false),
                                        func = func,
                                        return = SOME lbl})}
            , Block.T { args = Vector.new0 ()
                      , kind = Kind.CReturn {func = func}
                      , label = lbl
                      , statements = Vector.new0 ()
                      , transfer = Transfer.Goto { dst = lbl2
                                                 , args = Vector.new0 () }} ]

         in
            split (tl, bl@ blk, lbl2)
         end

       | [] => bl *)
   




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
  (*if (Vector.exists 
    (statements, fn s => case s of 
                            Dummy =>true
                          | _ => false)) 
    then split(rest,startBlock,l) else [b]*)
    loop2 (stlist,[],label)
end

  
  
  
        




fun handleFunction2 (f : Function.t) :Function.t = 
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


fun transform2 (program as Program.T {functions, handlesSignals, main, objectTypes}) =
  let 
    val _ = let open Control
                     in saveToFile
                        ({suffix = concat ["dummyTransfer", ".", "pre.rssa"]},
                         Control.No, program, Control.Layouts Program.layouts)
                     end

    val prog = Program.T { functions = List.map (functions, handleFunction2)
            , handlesSignals = handlesSignals
            , main = handleFunction2 (main)
            , objectTypes = objectTypes }
    
    val _ = let open Control
                     in saveToFile
                        ({suffix = concat ["dummyTransfer", ".", "post.rssa"]},
                         Control.No, prog, Control.Layouts Program.layouts)
                     end
  in 
         prog

  end

fun transform (program as Program.T {functions, handlesSignals, main, objectTypes}) =
   transform2 (Program.T { functions = List.map (functions, handleFunction)
            , handlesSignals = handlesSignals
            , main = handleFunction (main)
            , objectTypes = objectTypes })




end (*End of Block structure*)
