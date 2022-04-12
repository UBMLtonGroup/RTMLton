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
        val reserve_allocation = Label.newNoname ()

        val gc_collect_func = CFunction.gc {maySwitchThreads = false}
        val gc_reserve_allocation = CFunction.reserveAllocation {maySwitchThreads = false}

        fun reserveAndJumpToBlock (to) = 
            let
                val collectReturn = Label.newNoname ()
            in
                [ Block.T {args = Vector.new0 (),
                        kind = Kind.Jump,
                        label = reserve_allocation,
                        statements = Vector.new0 (),
                        transfer = (Transfer.CCall
                                        {args = Vector.new2 (Operand.GCState,  Operand.constWord chunksNeeded (WordSize.word32)), (*   Operand.word (WordX.fromIntInf (Word.toIntInf (chunksNeeded, WordSize.csize)))), *)
                                            func = gc_reserve_allocation,
                                            return = SOME collectReturn })}
                , Block.T { args = Vector.new0 ()
                          , kind = Kind.CReturn {func = gc_reserve_allocation}
                          , label = collectReturn
                          , statements = Vector.new0 ()
                          , transfer = Transfer.Goto { dst = to
                                                     , args = Vector.new0 () }} ]
            end

        datatype z = datatype Runtime.GCField.t

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

        val startBlock =
             Block.T { args = args
                        , kind = kind
                        , label = label
                        , statements = Vector.new0 ()
                        , transfer = Transfer.Goto { dst = reserve_allocation
                                                    , args = Vector.new0 () }}

    in
        if chunksNeeded = Word.fromInt 0
        then [b]
        else [startBlock] @
            reserveAndJumpToBlock dontCollect @
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

        val _ = TextIO.output (TextIO.stdErr, ("gc-check.fun: "))
        val _ = Layout.outputl (Func.layout(Function.name f), Out.error) (* JEFF *)
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
