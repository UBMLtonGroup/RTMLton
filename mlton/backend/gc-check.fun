(*
* The goal of gc-check is to ensure that the GC is triggered at approproate
* points and also ensure that any block recieves the requested amount of chunks
* before it starts executing. 
* 
* It assumes that the runtime gives the operand FLChunks, which is a runtime 
* count of number of chunks that the free list contains and TChunks which is the
* total number of chunks that are available to the program. It calculates the 
* number of chunks allocated by block nc
*
* The Checks inserted by this pass are: 
*
* 1. FLChunks > nc then 
*     decrement FLChunks by nc and continue if
*     FLChunks > (0.3 * TChunks) else signal the GC to Run and continue
* 
* 2. FLChunks < nc then 
*     Block and singal GC to run
*     When woken up by GC, do check 1 again
*
* *)


functor GCCheck (S: RSSA_TRANSFORM_STRUCTS): RSSA_TRANSFORM =
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



    fun insertCheck (b as Block.T {args, kind, label, statements, transfer}) =
    let
        val chunksNeeded = Block.objChunksAllocated b
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
                                        {args = Vector.new4 (Operand.GCState,
                                                                (Operand.word
                                                                    (WordX.zero (WordSize.csize ()))),
                                                                Operand.bool false,
                                                                Operand.bool true),
                                            func = func,
                                            return = SOME collectReturn})}
                , Block.T { args = Vector.new0 ()
                        , kind = Kind.CReturn {func = func}
                        , label = collectReturn
                        , statements = Vector.new0 ()
                        , transfer = Transfer.Goto { dst = to
                                                    , args = Vector.new0 () }} ]
            end

        datatype z = datatype Runtime.GCField.t

        fun decFreeChunks n =
            let
                val flChunks = Operand.Runtime FLChunks
                val res = Var.newNoname ()
                val resTy = Operand.ty flChunks
            in
                [Statement.PrimApp {args = (Vector.new2
                                                (flChunks, Operand.constWord n (WordSize.word32))),
                                    dst = SOME (res, resTy),
                                    prim = Prim.wordSub WordSize.word32},
                    Statement.Move {dst = flChunks,
                                    src = Operand.Var {ty = resTy, var = res}}]
            end


        fun incReserved n =
            let
                val reserved = Operand.Runtime Reserved
                val res = Var.newNoname ()
                val resTy = Operand.ty reserved
            in
                [Statement.PrimApp {args = (Vector.new2
                                                (reserved, Operand.constWord n (WordSize.word32))),
                                        dst = SOME (res, resTy),
                                        prim = Prim.wordAdd WordSize.word32},
                    Statement.Move {dst = reserved,
                                    src = Operand.Var {ty = resTy, var = res}}]
            end


        fun primApp (prim, op1, op2, {ifTrue, ifFalse}) =
            let
                val res = Var.newNoname ()
                val s =
                    Statement.PrimApp {args = Vector.new2 (op1, op2),
                                    dst = SOME (res, Type.bool),
                                    prim = prim}
            val transfer =
                    Transfer.ifBool
                        (Operand.Var {var = res, ty = Type.bool},
                        {falsee = ifFalse,
                        truee = ifTrue})
            in
                (s, transfer)
            end


        val lockstmt = Statement.PrimApp {args = Vector.new1 (Operand.Runtime FLLock),
                                        dst = NONE,
                                        prim = Prim.lockfl}

        val unlockstmt = Statement.PrimApp {args = Vector.new1 (Operand.Runtime FLLock),
                                            dst = NONE,
                                            prim = Prim.unlockfl}

        fun blockingGC blLbl  check =
        let
            val return = Label.newNoname ()
            val (ss, ts) = primApp (Prim.wordLt (WordSize.csize (), {signed = false}),
                                Operand.Runtime FLChunks,
                                Operand.constWord chunksNeeded (WordSize.csize ()),
                                {ifTrue=blLbl, ifFalse=check})
        in
            [ Block.T {args = Vector.new0 (),
                        kind = Kind.Jump,
                        label = blLbl,
                        statements = Vector.new1 (unlockstmt),
                        transfer = (Transfer.CCall
                                        {args = Vector.new4 (Operand.GCState,
                                                                (Operand.word
                                                                    (WordX.zero (WordSize.csize ()))),
                                                                Operand.bool true,
                                                                Operand.bool true),
                                            func = func,
                                            return = SOME return})}
                , Block.T { args = Vector.new0 ()
                        , kind = Kind.CReturn {func = func}
                        , label = return
                        , statements = Vector.new2 (lockstmt,ss)
                        , transfer = ts} ]


        end



        val startBlock =
        let
            val check = Label.newNoname ()
            val block = Label.newNoname ()
            val reserved = Operand.Runtime Reserved
            val res = Var.newNoname ()
            val resTy = Operand.ty reserved
            val sAdd = Statement.PrimApp {args = (Vector.new2
                                            (reserved, Operand.constWord chunksNeeded (WordSize.word32))),
                                        dst = SOME (res, resTy),
                                        prim = Prim.wordAdd WordSize.word32}

            val (ss, ts) = primApp (Prim.wordLt (WordSize.csize (), {signed = false}),
                                    Operand.Runtime FLChunks,
                                    Operand.Var {var = res, ty = resTy},
                                    {ifTrue=block, ifFalse=check})
            val (ss2, ts2) = primApp (Prim.wordLt (WordSize.csize (), {signed = false}),
                                    Operand.Runtime FLChunks,
                                    Operand.Runtime HeuristicChunks,
                                    {ifTrue=collect, ifFalse=dontCollect})
        in
            [ Block.T { args = args
                        , kind = kind
                        , label = label
                        , statements = Vector.new3 (lockstmt, sAdd, ss)
                        , transfer = ts }
               ,
                Block.T { args = Vector.new0 ()
                        , kind = Kind.Jump
                        , label = check
                        , statements = Vector.fromList ([ss2]  @  (incReserved chunksNeeded)
                                                                    @[ unlockstmt])
                        , transfer = ts2 }
            ] @ blockingGC block check

        end
    in
      if chunksNeeded = Word.fromInt 0
      then [b]
      else startBlock  @
           gcAndJumpToBlock dontCollect @
           [Block.T { args = Vector.new0 ()
                    , kind = Kind.Jump
                    , label = dontCollect
                    , statements = statements
                    , transfer = transfer }]
    end

    (*
    This function is applied to each function in the program. 
    
    "Function.dest f" is defined in mlton/backend/rssa.sig
    it matches the pattern of rssa.Function and extracts the
    fields. it 'destructs' f into its component fields.

     *)
    fun handleFunction (f: Function.t): Function.t =
    let
          val {args, blocks, name, raises, returns, start} = Function.dest f
          val newBlocks = ref []
          val () = Vector.foreach
                   (blocks,
                    fn block =>
                       List.push (newBlocks, insertCheck block))
    in
          Function.new { args = args
                       , blocks = Vector.fromList (List.concat (!newBlocks))
                       , name = name
                       , raises = raises
                       , returns = returns
                       , start = start }
    end

    (*
    Create a new program, replacing the functions with a new list of functions
    where each has been transformed using handleFunction 
     *)
    fun transform (program as Program.T {functions, handlesSignals, main, objectTypes}) =
        Program.T { functions = List.map (functions, handleFunction)
                  , handlesSignals = handlesSignals
                  , main = handleFunction (main)
                  , objectTypes = objectTypes }

end
