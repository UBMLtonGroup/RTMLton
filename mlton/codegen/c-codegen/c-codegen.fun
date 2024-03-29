(* Copyright (C) 2009,2014-2015 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

functor CCodegen (S: C_CODEGEN_STRUCTS): C_CODEGEN =
struct

open S

open Machine

datatype z = datatype RealSize.t
datatype z = datatype WordSize.prim

local
   open Runtime
in
   structure GCField = GCField
end

structure Kind =
   struct
      open Kind

      fun isEntry (k: t): bool =
         case k of
            Cont _ => true
          | CReturn {func, ...} => CFunction.mayGC func
          | Func => true
          | Handler _ => true
          | _ => false



      fun toString(k: t):string = 
        case k of 
             Cont _ => "Cont"
           | CReturn _ => "CReturn"
           | Func => "Func"
           | Handler _ => "Handler"
           | Jump => "Jump"

   end


structure Transfer = 
struct 
  open Transfer

  fun toString (tr: t):string =
    case tr of 
         Arith _ => "Arith"
       | CCall {func, return, ...} => "CCall"
       | Call _ => "Call"
       | Goto _ => "Goto"
       | Raise => "Raise"
       | Return => "Return"
       | Switch _ => "Switch" 

end


val traceGotoLabel = Trace.trace ("CCodegen.gotoLabel", Label.layout, Unit.layout)

structure C =
   struct
      val truee = "TRUE"
      val falsee = "FALSE"

      fun bool b = if b then truee else falsee

      fun args (ss: string list): string
         = concat ("(" :: List.separate (ss, ", ") @ [")"])

      fun callNoSemi (f: string, xs: string list, print: string -> unit): unit
         = (print f
            ; print " ("
            ; (case xs
                  of [] => ()
                | x :: xs => (print x
                              ; List.foreach (xs,
                                             fn x => (print ", "; print x))))
            ; print ")")

      fun call (f, xs, print) =
         (callNoSemi (f, xs, print)
          ; print ";\n")

      fun int (i: int) =
         if i >= 0
            then Int.toString i
         else concat ["-", Int.toString (~ i)]

      val bytes = int o Bytes.toInt

      fun string s =
         let val quote = "\""
         in concat [quote, String.escapeC s, quote]
         end

      fun word (w: Word.t) = "0x" ^ Word.toString w

      fun push (size: Bytes.t, print) =
         call ("\tPush", [bytes size], print)
   end

structure RealX =
   struct
      open RealX

      fun toC (r: t): string =
         let
            (* The main difference between SML reals and C floats/doubles is that
             * SML uses "~" while C uses "-".
             *)
            val s =
               String.translate (toString r,
                                 fn #"~" => "-" | c => String.fromChar c)
            (* Also, inf is spelled INFINITY and nan is NAN in C. *)
            val s =
               case s of
                  "-inf" => "-INFINITY"
                | "inf"  => "INFINITY"
                | "nan"  => "NAN"
                | other  => other
         in
            case size r of
               R32 => concat ["(Real32)", s]
             | R64 => s
         end
   end

structure WordX =
   struct
      open WordX

      fun toC (w: t): string =
         let
            fun doit s =
               concat ["(Word", s, ")(", toString w, "ull)"]
         in
            case WordSize.prim (size w) of
               W8 => doit "8"
             | W16 => doit "16"
             | W32 => doit "32"
             | W64 => doit "64"
         end
   end

structure WordXVector =
   struct
      local
         structure Z = WordX
      in
         open WordXVector
         structure WordX = Z
      end

      fun toC (v: t): string =
         let
            fun string () =
               concat ["(pointer)",
                       C.string (String.implode (toListMap (v, WordX.toChar)))]
            fun vector s =
               concat ["(pointer)((Word", s, "[]){",
                       String.concatWith (toListMap (v, WordX.toC), ","),
                       "})"]
         in
            case WordSize.prim (elementSize v) of
               W8 => string ()
             | W16 => vector "16"
             | W32 => vector "32"
             | W64 => vector "64"
         end
   end

structure Operand =
   struct
      open Operand

      fun isMem (z: t): bool =
         case z of
            ArrayOffset _ => true
          | Cast (z, _) => isMem z
          | Contents _ => true
          | Offset _ => true
          | ChunkedOffset _ => true
          | StackOffset _ => true
          | _ => false

    fun isStack (z: t): bool =
         case z of
            ArrayOffset _ => false
          | Cast (z, _) => isStack z
          | Contents _ => false
          | Offset _ => false
          | ChunkedOffset _ => false
          | StackOffset _ => true
          | _ => false
   end

fun implementsPrim (p: 'a Prim.t): bool =
   let
      datatype z = datatype Prim.Name.t
   in
      case Prim.name p of
         CPointer_add => true
       | CPointer_diff => true
       | CPointer_equal => true
       | CPointer_fromWord => true
       | CPointer_lt => true
       | CPointer_sub => true
       | CPointer_toWord => true
       | FFI_Symbol _ => true
       | Real_Math_acos _ => true
       | Real_Math_asin _ => true
       | Real_Math_atan _ => true
       | Real_Math_atan2 _ => true
       | Real_Math_cos _ => true
       | Real_Math_exp _ => true
       | Real_Math_ln _ => true
       | Real_Math_log10 _ => true
       | Real_Math_sin _ => true
       | Real_Math_sqrt _ => true
       | Real_Math_tan _ => true
       | Real_abs _ => true
       | Real_add _ => true
       | Real_castToWord _ => true
       | Real_div _ => true
       | Real_equal _ => true
       | Real_ldexp _ => true
       | Real_le _ => true
       | Real_lt _ => true
       | Real_mul _ => true
       | Real_muladd _ => true
       | Real_mulsub _ => true
       | Real_neg _ => true
       | Real_qequal _ => false
       | Real_rndToReal _ => true
       | Real_rndToWord _ => true
       | Real_round _ => true
       | Real_sub _ => true
       | Word_add _ => true
       | Word_addCheck _ => true
       | Word_andb _ => true
       | Word_castToReal _ => true
       | Word_equal _ => true
       | Word_extdToWord _ => true
       | Word_lshift _ => true
       | Word_lt _ => true
       | Word_mul _ => true
       | Word_mulCheck _ => true
       | Word_neg _ => true
       | Word_negCheck _ => true
       | Word_notb _ => true
       | Word_orb _ => true
       | Word_quot (_, {signed}) => not signed
       | Word_rem (_, {signed}) => not signed
       | Word_rndToReal _ => true
       | Word_rol _ => true
       | Word_ror _ => true
       | Word_rshift _ => true
       | Word_sub _ => true
       | Word_subCheck _ => true
       | Word_xorb _ => true
       | _ => false
   end

fun creturn (t: Type.t): string =
   concat ["CReturn", CType.name (Type.toCType t), "[PTHREAD_NUM]"]

fun outputIncludes (includes, print) =
   (print "#define _ISOC99_SOURCE\n"
    ; List.foreach (includes, fn i => (print "#include <";
                                       print i;
                                       print ">\n"))
    ; print "\n")

fun declareProfileLabel (l, print) =
   C.call ("DeclareProfileLabel", [ProfileLabel.toString l], print)

fun declareGlobals (prefix: string, print) =
   let
      (* gcState can't be static because stuff in mlton-lib.c refers to
       * it.
       *)
      val _ = print (concat [prefix, "struct GC_state gcState;\n"])
      val _ =
         List.foreach
         (CType.all, fn t =>
          let
             val s = CType.toString t
          in
             print (concat [prefix, s, " global", s,
                            "[", C.int (Global.numberOfType t), "];\n"])
             ; print (concat [prefix, s, " CReturn", CType.name t, "[PTHREAD_MAX];\n"])
          end)
      val _ =
         print (concat [prefix, "Pointer globalObjptrNonRoot [",
                        C.int (Global.numberOfNonRoot ()),
                        "];\n"])
   in
      ()
   end

fun outputDeclarations
   {additionalMainArgs: string list,
    includes: string list,
    print: string -> unit,
    program = (Program.T
               {frameLayouts, frameOffsets, maxFrameSize,
                objectTypes, profileInfo, reals, vectors, ...}),
    rest: unit -> unit
    }: unit =
   let
      fun declareExports () =
         Ffi.declareExports {print = print}
      fun declareLoadSaveGlobals () =
         let
            val _ = 
               print "void Copy_globalObjptrs(int f, int t) { memcpy(globalObjptr[t], globalObjptr[f], sizeof(globalObjptr[t])); }\n"
            val _ =
               (print "static int saveGlobals (FILE *f) {\n"
                ; (List.foreach
                   (CType.all, fn t =>
                    print (concat ["\tSaveArray (global",
                                   CType.toString t, ", f);\n"])))
                ; print "\treturn 0;\n}\n")
            val _ =
               (print "static int loadGlobals (FILE *f) {\n"
                ; (List.foreach
                   (CType.all, fn t =>
                    print (concat ["\tLoadArray (global",
                                   CType.toString t, ", f);\n"])))
                ; print "\treturn 0;\n}\n")
         in
            ()
         end
      fun declareVectors () =
         (print "BeginVectorInits\n"
          ; (List.foreach
             (vectors, fn (g, v) =>
              (C.callNoSemi ("VectorInitElem",
                             [C.int (Bytes.toInt
                                     (WordSize.bytes
                                      (WordXVector.elementSize v))),
                              C.int (Global.index g),
                              C.int (WordXVector.length v),
                              WordXVector.toC v],
                             print)
                 ; print "\n")))
          ; print "EndVectorInits\n")
      fun declareReals () =
         (print "static void real_Init() {\n"
          ; List.foreach (reals, fn (g, r) =>
                          print (concat ["\tglobalReal",
                                         RealSize.toString (RealX.size r),
                                         "[", C.int (Global.index g), "] = ",
                                         RealX.toC r, ";\n"]))
          ; print "}\n")
      fun declareFrameOffsets () =
         Vector.foreachi
         (frameOffsets, fn (i, v) =>
          (print (concat ["static uint16_t frameOffsets", C.int i, "[] = {"])
           ; print (C.int (Vector.length v))
           ; Vector.foreach (v, fn i => (print ","; print (C.bytes i)))
           ; print "};\n"))
      fun declareArray (ty: string,
                        name: string,
                        v: 'a vector,
                        toString: int * 'a -> string) =
         (print (concat ["uint32_t ", name, "_len = ", Int.toString(Vector.length v), ";\n"]) (* frameLayouts_len *)
          ; print (concat ["static ", ty, " ", name, "[] = {\n"])
          ; Vector.foreachi (v, fn (i, x) =>
                             print (concat ["\t", toString (i, x), ", /* ", Int.toString(i), " */\n"]))
          ; print "};\n")
      fun declareFrameLayouts () =
         declareArray ("struct GC_frameLayout", "frameLayouts", frameLayouts,
                       fn (_, {frameOffsetsIndex, isC, size}) =>
                       concat ["{",
                               if isC then "C_FRAME" else "ML_FRAME",
                               ", frameOffsets", C.int frameOffsetsIndex,
                               ", ", C.bytes size,
                               "}"])
      fun declareAtMLtons () =
         declareArray ("char*", "atMLtons", !Control.atMLtons, C.string o #2)
      fun declareObjectTypes () =
         declareArray
         ("struct GC_objectType", "objectTypes", objectTypes,
          fn (_, ty) =>
          let
             datatype z = datatype Runtime.RObjectType.t
             val (tag, hasIdentity, bytesNonObjptrs, numObjptrs) =
                case ObjectType.toRuntime ty of
                   Array {hasIdentity, bytesNonObjptrs, numObjptrs} =>
                      ("ARRAY_TAG", hasIdentity,
                       Bytes.toInt bytesNonObjptrs, numObjptrs)
                 | Normal {hasIdentity, bytesNonObjptrs, numObjptrs} =>
                      ("NORMAL_TAG", hasIdentity,
                       Bytes.toInt bytesNonObjptrs, numObjptrs)
                 | Stack =>
                      ("STACK_TAG", false, 0, 0)
                 | Weak {gone} =>
                      let
                         val bytesObjptr =
                            Bits.toBytes (Control.Target.Size.objptr ())
                         val bytesNonObjptrs =
                            let
                               val align =
                                  case !Control.align of
                                     Control.Align4 => Bytes.fromInt 4
                                   | Control.Align8 => Bytes.fromInt 8
                               val bytesCPointer =
                                  Bits.toBytes (Control.Target.Size.cpointer ())
                               val bytesHeader =
                                  Bits.toBytes (Control.Target.Size.header ())

                               val bytesObject =
                                  Bytes.+ (bytesHeader,
                                  Bytes.+ (bytesCPointer,
                                           bytesObjptr))
                               val bytesTotal =
                                  Bytes.align (bytesObject, {alignment = align})
                               val bytesPad = Bytes.- (bytesTotal, bytesObject)
                            in
                               Bytes.+ (bytesPad, bytesCPointer)
                            end
                         val (bytesNonObjptrs, bytesObjptr) =
                            (Bytes.toInt bytesNonObjptrs,
                             Bytes.toInt bytesObjptr)
                         val (bytesNonObjptrs, numObjptrs) =
                            if gone
                               then (bytesNonObjptrs + bytesObjptr, 0)
                            else (bytesNonObjptrs, 1)
                      in
                         ("WEAK_TAG", false, bytesNonObjptrs, numObjptrs)
                      end
          in
             concat ["{ ", tag, ", ",
                     C.bool hasIdentity, ", ",
                     C.int bytesNonObjptrs, ", ",
                     C.int numObjptrs, " }"]
          end)
      fun declareMLtonMain () =
         let
            val align =
               case !Control.align of
                  Control.Align4 => 4
                | Control.Align8 => 8
            val magic =
               let
                  val version = String.hash Version.version
                  val random = Random.word ()
               in
                  Word.orb
                  (Word.<< (version, Word.fromInt (Word.wordSize - 8)),
                   Word.>> (random, Word.fromInt 8))
               end
            val profile =
               case !Control.profile of
                  Control.ProfileNone => "PROFILE_NONE"
                | Control.ProfileAlloc => "PROFILE_ALLOC"
                | Control.ProfileCallStack => "PROFILE_NONE"
                | Control.ProfileCount => "PROFILE_COUNT"
                | Control.ProfileDrop => "PROFILE_NONE"
                | Control.ProfileLabel => "PROFILE_NONE"
                | Control.ProfileTimeField => "PROFILE_TIME_FIELD"
                | Control.ProfileTimeLabel => "PROFILE_TIME_LABEL"
            val gcmodule =
            	case !Control.gcModule of
            		Control.GCModuleNone => "GC_NONE"
            	 |  Control.GCModuleDefault => "GC_DEFAULT"
         in
            C.callNoSemi (case !Control.format of
                             Control.Archive => "MLtonLibrary"
                           | Control.Executable => "MLtonMain"
                           | Control.LibArchive => "MLtonLibrary"
                           | Control.Library => "MLtonLibrary",
                          [C.int align,
                           C.word magic,
                           C.bytes maxFrameSize,
                           C.bool (!Control.markCards),
                           profile,
                           C.bool (!Control.profileStack),
                           gcmodule]
                          @ additionalMainArgs,
                          print)
            ; print "\n"
         end
      fun declareMain () =
         if !Control.emitMain andalso !Control.format = Control.Executable
            then List.foreach
                 (["int main (int argc, char* argv[]) {",
                   "return (MLton_main (argc, argv));",
                   "}"], fn s => (print s; print "\n"))
         else ()
      fun declareProfileInfo () =
         let
            fun doit (ProfileInfo.T {frameSources, labels, names, sourceSeqs,
                                     sources}) =
               (Vector.foreach (labels, fn {label, ...} =>
                                declareProfileLabel (label, print))
                ; (Vector.foreachi
                   (sourceSeqs, fn (i, v) =>
                    (print (concat ["static uint32_t sourceSeq",
                                    Int.toString i,
                                    "[] = {"])
                     ; print (C.int (Vector.length v))
                     ; Vector.foreach (v, fn i =>
                                       (print (concat [",", C.int i])))
                     ; print "};\n")))
                ; declareArray ("uint32_t*", "sourceSeqs", sourceSeqs, fn (i, _) =>
                                concat ["sourceSeq", Int.toString i])
                ; declareArray ("GC_sourceSeqIndex", "frameSources", frameSources, C.int o #2)
                ; (declareArray
                   ("struct GC_sourceLabel", "sourceLabels", labels,
                    fn (_, {label, sourceSeqsIndex}) =>
                    concat ["{(pointer)&", ProfileLabel.toString label, ", ",
                            C.int sourceSeqsIndex, "}"]))
                ; declareArray ("char*", "sourceNames", names, C.string o #2)
                ; declareArray ("struct GC_source", "sources", sources,
                                fn (_, {nameIndex, successorsIndex}) =>
                                concat ["{ ", Int.toString nameIndex, ", ",
                                        Int.toString successorsIndex, " }"]))
         in
            case profileInfo of
               NONE => doit ProfileInfo.empty
             | SOME z => doit z
         end
   in
      outputIncludes (includes, print)
      ; declareGlobals ("PRIVATE ", print)
      ; declareExports ()
      ; declareLoadSaveGlobals ()
      ; declareVectors ()
      ; declareReals ()
      ; declareFrameOffsets ()
      ; declareFrameLayouts ()
      ; declareObjectTypes ()
      ; declareProfileInfo ()
      ; declareAtMLtons ()
      ; rest ()
      ; declareMLtonMain ()
      ; declareMain ()
   end

structure Type =
   struct
      open Type

      fun toC (t: t): string =
         CType.toString (Type.toCType t)
   end

structure StackOffset =
   struct
      open StackOffset

      fun toString (T {offset, ty}): string =
         concat ["S", C.args [Type.toC ty, C.bytes offset]]

     fun shift (T {offset, ty}, size): t =
         T {offset = Bytes.- (offset, size),
            ty = ty}
   end

fun contents (ty, z) = concat ["/*Contents*/ C", C.args [Type.toC ty, z]]

fun declareFFI (Chunk.T {blocks, ...}, {print: string -> unit}) =
   let
      val seen = String.memoize (fn _ => ref false)
      fun doit (name: string, declare: unit -> string): unit =
         let
            val r = seen name
         in
            if !r
               then ()
            else (r := true; print (declare ()))
         end
   in
      Vector.foreach
      (blocks, fn Block.T {statements, transfer, ...} =>
       let
          datatype z = datatype CFunction.SymbolScope.t
          val _ =
             Vector.foreach
             (statements, fn s =>
              case s of
                 Statement.PrimApp {prim, ...} =>
                    (case Prim.name prim of
                        Prim.Name.FFI_Symbol {name, cty, symbolScope} =>
                           doit
                           (name, fn () =>
                            concat [case symbolScope of
                                       External => "EXTERNAL "
                                     | Private => "PRIVATE "
                                     | Public => "PUBLIC ",
                                    "extern ",
                                    case cty of
                                       SOME x => CType.toString x
                                     | NONE => "void",
                                    " ",
                                    name,
                                    ";\n"])
                      | _ => ())
               | _ => ())
          val _ =
             case transfer of
                Transfer.CCall {func, ...} =>
                   let
                      datatype z = datatype CFunction.Target.t
                      val CFunction.T {target, ...} = func
                   in
                      case target of
                         Direct "Thread_returnToC" => ()
                       | Direct name =>
                            doit (name, fn () =>
                                  concat [CFunction.cPrototype func, ";\n"])
                       | Indirect => ()
                   end
              | _ => ()
       in
          ()
       end)
   end

val inCritical = ref false

fun output {program as Machine.Program.T {chunks,
                                          frameLayouts,
                                          main = {chunkLabel, label}, ...},
            outputC: unit -> {file: File.t,
                              print: string -> unit,
                              done: unit -> unit}} =
   let
      datatype status = None | One | Many
      val {get = labelInfo: Label.t -> {block: Block.t,
                                        chunkLabel: ChunkLabel.t,
                                        frameIndex: int option,
                                        status: status ref,
                                        layedOut: bool ref},
           set = setLabelInfo, ...} =
         Property.getSetOnce
         (Label.plist, Property.initRaise ("CCodeGen.info", Label.layout))
      val entryLabels: (Label.t * int) list ref = ref []
      val indexCounter = Counter.new (Vector.length frameLayouts)
      val _ =
         List.foreach
         (chunks, fn Chunk.T {blocks, chunkLabel, ...} =>
          Vector.foreach
          (blocks, fn b as Block.T {kind, label, ...} =>
           let
              fun entry (index: int) =
                 List.push (entryLabels, (label, index))
              val frameIndex =
                 case Kind.frameInfoOpt kind of
                    NONE => (if Kind.isEntry kind
                                then entry (Counter.next indexCounter)
                             else ()
                             ; NONE)
                  | SOME (FrameInfo.T {frameLayoutsIndex, ...}) =>
                       (entry frameLayoutsIndex
                        ; SOME frameLayoutsIndex)
           in
              setLabelInfo (label, {block = b,
                                    chunkLabel = chunkLabel,
                                    frameIndex = frameIndex,
                                    layedOut = ref false,
                                    status = ref None})
           end))
      val a = Array.fromList (!entryLabels)
      val () = QuickSort.sortArray (a, fn ((_, i), (_, i')) => i <= i')
      val entryLabels = Vector.map (Vector.fromArray a, #1)
      val labelChunk = #chunkLabel o labelInfo
      val {get = chunkLabelIndex: ChunkLabel.t -> int, ...} =
         Property.getSet (ChunkLabel.plist,
                          Property.initFun (let
                                               val c = Counter.new 0
                                            in
                                               fn _ => Counter.next c
                                            end))
      val chunkLabelToString = C.int o chunkLabelIndex
      fun declareChunk (Chunk.T {chunkLabel, ...}, print) =
         C.call ("DeclareChunk",
                 [chunkLabelToString chunkLabel],
                 print)
      val {get = labelIndex, set = setLabelIndex, ...} =
         Property.getSetOnce (Label.plist,
                              Property.initRaise ("index", Label.layout))
      val _ =
         Vector.foreachi (entryLabels, fn (i, l) => setLabelIndex (l, i))
      fun labelToStringIndex (l: Label.t): string =
         let
            val s = C.int (labelIndex l)
         in
            if 0 = !Control.Native.commented
               then s
            else concat [s, " /* ", Label.toString l, " */"]
         end
      val handleMisaligned =
         let
            open Control
         in
            !align = Align4
            andalso (case !Control.Target.arch of
                        Target.HPPA => true
                      | Target.Sparc => true
                      | _ => false)
         end
      val handleMisaligned =
         fn ty =>
         handleMisaligned
         andalso (Type.equals (ty, Type.real R64)
                  orelse Type.equals (ty, Type.word WordSize.word64))
      fun addr z = concat ["&(", z, ")"]
      fun fetch (z, ty) =
         concat [CType.toString (Type.toCType ty),
                 "_fetch(", addr z, ")"]
      fun move' ({dst, src}, ty) =
         concat ["/*BeginCriticalSection 4*/", CType.toString (Type.toCType ty),
                 "_move(", addr dst, ", ", addr src, ");\n"]
      fun store ({dst, src}, ty) =
         concat ["/*BeginCriticalSection 3*/", CType.toString (Type.toCType ty),
                 "_store(", addr dst, ", ", src, ");\n"]


      fun stackletWrite({dbase,sbase}, ty :Type.t, dst, src) :string =
        (
          case (hd(String.explode dbase)) of
          #"S" => (concat [

                           "\n\tSTACKWRITE(",
                           dbase,
                           ", ",
                           src,
                           ");\n"

                           ])(*
                           "\n\tSTACKLET_",
                           dbase, " = ", src, ";\n" ] )*)
         | _   => ""
        )


      fun strip s = String.implode (List.allButLast (String.explode s)) 

      fun writeBarrier( {dbase,sbase},{dst,src},ty :Type.t,oper :string) :string = 
        case(Type.isObjptr ty)
          of true => 
          ( case hd(String.explode dst) of
                #"P" => oper
               | #"S" => concat ["WB(",CType.toString (Type.toCType ty),",","0xdeadbeef",",",sbase,",",dst,",",src,", {",(strip oper),"} );\n"]
               | #"N" => concat ["WB(",CType.toString (Type.toCType ty),",","0xdeadbeef",",",sbase,",",dst,",",src,", {",(strip oper),"} );\n"]
               | _ => concat ["WB(",CType.toString (Type.toCType ty),",",dbase,",",sbase,",",dst,",",src,", {",(strip oper),"} );\n"]
          )
        |   false => oper

      fun move {dst: string, dstIsMem: bool,
                src: string, srcIsMem: bool,
                dbase:string, sbase:string,
                ty: Type.t, inCrit : bool ref}: string =
         case dst of
         "Frontier" => 
	         if handleMisaligned ty then (
	            inCrit := true ;
	            case (dstIsMem, srcIsMem) of
	               (false, false) => concat [dst, " = ", src, ";\n"]
	             | (false, true) => concat [dst, " = ", fetch (src, ty), ";\n"]
	             | (true, false) => store ({dst = dst, src = src}, ty)
	             | (true, true) => move' ({dst = dst, src = src}, ty)
	         ) else (
	            inCrit := true ;
	            print "Weird Frontier write?" ;
	            concat [dst, " = ", src, ";\n"]
	         )
	        | _ => 
	          case !inCrit of
	          true => (
	              inCrit := false ;
			         if handleMisaligned ty then
			            case (dstIsMem, srcIsMem) of
			               (false, false) => concat [dst, " = ", src, ";\n"]
			             | (false, true) => concat [dst, " = ", fetch (src, ty), ";\n"]
			             | (true, false) => store ({dst = dst, src = src}, ty)
			             | (true, true) => move' ({dst = dst, src = src}, ty)
			         else (
			            inCrit := false ;
			            concat["\n\t", writeBarrier({dbase = dbase, sbase = sbase},
			                                        {dst = dst,src = src},
			                                        ty,
			                                        concat [dst, " = ", src, ";\n"])
			                  ]
			         )
			       )
		        | false =>
			         if handleMisaligned ty then
			            case (dstIsMem, srcIsMem) of
			               (false, false) => concat [dst, " = ", src, ";\n"]
			             | (false, true) => concat [dst, " = ", fetch (src, ty), ";\n"]
			             | (true, false) => store ({dst = dst, src = src}, ty)
			             | (true, true) => move' ({dst = dst, src = src}, ty)
			         else (
                               writeBarrier({dbase = dbase, sbase = sbase},
                                            {dst = dst,src = src},
                                            ty,
                                            concat [dst, " = ", src, ";\n"])
                               
			         )
      local
         datatype z = datatype Operand.t
         fun toString (z: Operand.t): string =
            case z of
               ArrayOffset {base, index, offset, scale, ty} =>
               concat ["X", C.args [Type.toC ty,
                                    toString GCState,
                                    toString base,
                                    toString index,
                                    Scale.toString scale,
                                    C.bytes offset]]
             | Cast (z, ty) => concat ["(", Type.toC ty, ")", toString z]
             | Contents {oper, ty} => contents (ty, toString oper)
             | Frontier => "Frontier"
             | GCState => "GCState"
             | Global g =>
                  if Global.isRoot g
                     then concat ["G",
                                  C.args [Type.toC (Global.ty g),
                                          Int.toString (Global.index g)]]
                  else concat ["GPNR", C.args [Int.toString (Global.index g)]]
             | Label l => labelToStringIndex l
             | Null => "NULL"
             | Offset {base, offset, ty} =>
                  concat ["O", C.args [Type.toC ty,
                                       toString base,
                                       C.bytes offset]]
             | ChunkedOffset {base, offset, ty, size} =>
                  concat [ "CHOFF"
                         , C.args [ toString GCState
                                  , Type.toC ty
                                  , toString base
                                  , C.bytes offset
                                  , C.bytes size ]]
             | Real r => RealX.toC r
             | Register r =>
                  concat [Type.name (Register.ty r), "_",
                          Int.toString (Register.index r), "[PTHREAD_NUM]"]
             | StackOffset s => StackOffset.toString s
             | StackTop => "StackTop"
             | Word w => WordX.toC w
             | ChunkExnHandler h => concat ["ChunkExnHandler"]
             | ChunkExnLink => concat ["ChunkExnLink"]

        fun nextstackletoffset(ops: Operand.t, currentframesize: Bytes.t option):string = 
          case ops of 
               StackOffset (so as StackOffset.T {offset, ty, ...}) =>
                                        (case currentframesize of 
                                             NONE => toString ops
                                           | SOME size=>  
                                                    if Bytes.>= (offset,size)
                                                    then "N"^toString (StackOffset (StackOffset.shift(so,size)) )
                                                    else toString ops
                                        )
             | Cast (z, ty) => if Operand.isStack z 
                                then concat ["(", Type.toC ty, ")", nextstackletoffset (z,currentframesize)]
                                else toString ops
             | _ => toString ops

        fun issrcinnextchunk(src: Operand.t, framesize:Bytes.t option):bool = 
          case src of 
               StackOffset (so as StackOffset.T {offset, ty, ...}) => 
                    (case framesize of 
                            NONE => false
                          | SOME size => if Bytes.>= (offset,size)
                                         then true
                                         else false
                    )
            | Cast (z, ty) => if Operand.isStack z
                              then issrcinnextchunk (z,framesize)
                              else false
            | _ => false

        fun getbase(x: Operand.t):string = 
          case x of 
               Offset {base, offset, ty} => 
                    toString base
            |   ArrayOffset {base, index, offset, scale, ty} =>
                    toString x (*we want to inspect the contents of X macro not
                    base of array. Thus just spits out the X macro.*)
            |   ChunkedOffset {base, offset, ty, size} =>
                    toString base
            | _ => toString x
      in
         val operandToString = toString
         val getBase = getbase
         val nextStackletOffset = nextstackletoffset
         val isSrcInNextChunk =  issrcinnextchunk
      end
      fun fetchOperand (z: Operand.t): string =
         if handleMisaligned (Operand.ty z) andalso Operand.isMem z then
            fetch (operandToString z, Operand.ty z)
         else
            operandToString z
      fun outputStatement (s, print,pushSize,contFrameSize) =
         let
            datatype z = datatype Statement.t

         in
            case s of
               Noop => ()
             | _ =>
                  (print "\t"
                   ; (case s of
                         Move {dst, src} =>
                            let 
                              val srcInNC = isSrcInNextChunk(src,contFrameSize)
                            in
                              print
                              (move {dst = if srcInNC
                                           then operandToString dst
                                           else nextStackletOffset (dst,pushSize),
                                     dstIsMem = Operand.isMem dst,
                                     dbase = if Operand.isStack dst 
                                                andalso (not srcInNC) 
                                             then nextStackletOffset (dst,pushSize)
                                             else getBase dst,
                                     src = nextStackletOffset (src,contFrameSize),
                                     srcIsMem = Operand.isMem src,
                                     sbase = if Operand.isStack src 
                                              then nextStackletOffset (src,contFrameSize)
                                              else getBase src,
                                     ty = Operand.ty dst,
                                     inCrit = inCritical})
                            end
                       | Noop => ()
                       | PrimApp {args, dst, prim} =>
                            let
                               fun call (): string =
                                  concat
                                  [Prim.toString prim,
                                   " (",
                                   concat
                                   (List.separate
                                    (Vector.toListMap (args, fetchOperand),
                                     ", ")),
                                   ")"]
                               fun app (): string =
                                  case Prim.name prim of
                                     Prim.Name.FFI_Symbol {name, ...} =>
                                        concat
                                        ["((",CType.toString CType.CPointer,
                                         ")(&", name, "))"]
                                   | _ => call ()
                            in
                               case dst of
                                  NONE => (print (app ())
                                           ; print ";\n")
                                | SOME dst =>
                                     print (move {dst = nextStackletOffset (dst,pushSize),
                                                  dstIsMem = Operand.isMem dst,
                                                  dbase = if Operand.isStack dst 
                                                          then nextStackletOffset (dst,pushSize)
                                                          else getBase dst,
                                                  src = app (),
                                                  srcIsMem = false,
                                                  sbase = app (),
                                                  ty = Operand.ty dst,
                                                  inCrit = inCritical})
                            end
                       | ProfileLabel l =>
                            C.call ("ProfileLabel", [ProfileLabel.toString l],
                                    print)
                            ))
         end
      val amTimeProfiling =
         !Control.profile = Control.ProfileTimeField
         orelse !Control.profile = Control.ProfileTimeLabel
      fun outputChunk (chunk as Chunk.T {chunkLabel, blocks, regMax, ...}) =
         let
            val {done, print, ...} = outputC ()
            fun declareChunks () =
               let
                  val {get, ...} =
                     Property.get (ChunkLabel.plist,
                                   Property.initFun (fn _ => ref false))
                  val _ =
                     Vector.foreach
                     (blocks, fn Block.T {transfer, ...} =>
                      case transfer of
                         Transfer.Call {label, ...} =>
                            get (labelChunk label) := true
                       | _ => ())
                  val _ =
                     List.foreach
                     (chunks, fn c as Chunk.T {chunkLabel, ...} =>
                      if ! (get chunkLabel)
                         then declareChunk (c, print)
                      else ())
               in
                  ()
               end
            fun declareProfileLabels () =
               Vector.foreach
               (blocks, fn Block.T {statements, ...} =>
                Vector.foreach
                (statements, fn s =>
                 case s of
                    Statement.ProfileLabel l => declareProfileLabel (l, print)
                  | _ => ()))
            (* Count how many times each label is jumped to. *)
            fun jump l =
               let
                  val {status, ...} = labelInfo l
               in
                  case !status of
                     None => status := One
                   | One => status := Many
                   | Many => ()
               end
            fun force l = #status (labelInfo l) := Many
            val _ =
                Vector.foreach
                (blocks, fn Block.T {kind, label, transfer, ...} =>
                 let
                    val _ = if Kind.isEntry kind then jump label else ()
                    datatype z = datatype Transfer.t
                 in
                    case transfer of
                       Arith {overflow, success, ...} =>
                          (jump overflow; jump success)
                     | CCall {func, return, ...} =>
                          if CFunction.maySwitchThreads func
                             then ()
                          else Option.app (return, jump)
                     | Call {label, ...} => jump label
                     | Goto dst => jump dst
                     | Raise => ()
                     | Return => ()
                     | Switch s => Switch.foreachLabel (s, jump)
                 end)
            fun push (return: Label.t, size: Bytes.t) =
               (print ("\t /*push-return: " ^ (Label.toString return) ^ " */")
                ; print (move {dst = (StackOffset.toString
                                      (StackOffset.T
                                       {offset = Bytes.- (size, Runtime.labelSize ()),
                                        ty = Type.label return})),
                               dstIsMem = true,
                               dbase = (StackOffset.toString
                                      (StackOffset.T
                                       {offset = Bytes.- (size, Runtime.labelSize ()),
                                        ty = Type.label return})),
                               src = operandToString (Operand.Label return),
                               srcIsMem = false,
                               sbase = getBase (Operand.Label return),
                               ty = Type.label return, inCrit = inCritical})
                ; C.push (size, print) ; print ("\t /*push-return:done */\n")
                ; if amTimeProfiling
                     then print "\tFlushStackTop();\n"
                  else ())
            fun copyArgs (args: Operand.t vector): string list * (unit -> unit) =
               let
                  fun usesStack z =
                     case z of
                        Operand.ArrayOffset {base, index, ...} =>
                           (usesStack base) orelse (usesStack index)
                      | Operand.Cast (z, _) =>
                           (usesStack z)
                      | Operand.Contents {oper, ...} =>
                           (usesStack oper)
                      | Operand.Offset {base, ...} =>
                           (usesStack base)
                      | Operand.ChunkedOffset {base, ...} =>
                           (usesStack base)
                      | Operand.StackOffset _ => true
                      | _ => false
               in
                  if Vector.exists (args, usesStack)
                     then
                        let
                           val _ = print "\t{\n"
                           val c = Counter.new 0
                           val args =
                              Vector.toListMap
                              (args, fn z =>
                               if usesStack z
                                  then
                                     let
                                        val ty = Operand.ty z
                                        val tmp =
                                           concat ["tmp",
                                                   Int.toString (Counter.next c)]
                                        val _ =
                                           print
                                           (concat
                                            ["\t", Type.toC ty, " ", tmp, " = ",
                                             fetchOperand z, "; /*J1*/\n"])
                                     in
                                        tmp
                                     end
                               else fetchOperand z)
                        in
                           (args, fn () => print "\t}\n")
                        end
                  else (Vector.toListMap (args, fetchOperand),
                        fn () => ())
               end
            val tracePrintLabelCode =
               Trace.trace
               ("CCodegen.printLabelCode",
                fn {block, layedOut, ...} =>
                Layout.record [("block", Label.layout (Block.label block)),
                               ("layedOut", Bool.layout (!layedOut))],
                Unit.layout)
            fun maybePrintLabel l =
               if ! (#layedOut (labelInfo l))
                  then ()
               else gotoLabel l
            and gotoLabel arg =
               traceGotoLabel
               (fn l =>
                let
                   val info as {layedOut, ...} = labelInfo l
                in
                   if !layedOut
                      then print (concat ["\tgoto ", Label.toString l, ";\n"])
                   else printLabelCode info
                end) arg
            and printLabelCode arg =
               tracePrintLabelCode
               (fn {block = Block.T {kind, label = l, live, statements,
                                     transfer, ...},
                    layedOut, status, ...} =>
                let
                  val _ = layedOut := true
                  val _ =
                     case !status of
                        Many =>
                           let
                              val s = Label.toString l
                           in
                              print s
                              ; print ":\n"
                           end
                      | _ => ()

                  
                 val _ = print ("/*Kind: "^Kind.toString(kind)^
                            " Transfer: "^Transfer.toString(transfer)^" */\n") 

                 val pushSize = (case transfer of
                                 Transfer.Call {label, return, ...} =>
                                             (case return of
                                                    NONE => NONE
                                                  | SOME{return,size, ...}  =>
                                                      SOME size)
                                | _ => NONE)

                 val contFrameSize = (case kind of 
                                    Kind.Cont {frameInfo, ...}=> SOME (Program.frameSize (program, frameInfo))
                                  | _ => NONE)



                  fun pop (fi: FrameInfo.t) =
                     (C.push (Bytes.~ (Program.frameSize (program, fi)), print)
                      ; if amTimeProfiling
                           then print "\tFlushStackTop();\n"
                        else ())
                  val _ =
                     case kind of
                        Kind.Cont {frameInfo, ...} => pop frameInfo
                      | Kind.CReturn {dst, frameInfo, ...} =>
                           (case frameInfo of
                               NONE => ()
                             | SOME fi => pop fi
                            ; (Option.app
                               (dst, fn x =>
                                let
                                   val x = Live.toOperand x
                                   val ty = Operand.ty x
                                in
                                  print
                                   (concat
                                    ["\t",
                                     move {dst = nextStackletOffset (x,pushSize),
                                           dstIsMem = Operand.isMem x,
                                           dbase = if Operand.isStack x
                                                   then nextStackletOffset(x,pushSize)
                                                   else getBase x,
                                           src = creturn ty,
                                           srcIsMem = false,
                                           sbase = creturn ty,
                                           ty = ty, inCrit = inCritical}])
                                end)))
                      | Kind.Func => ()
                      | Kind.Handler {frameInfo, ...} => pop frameInfo
                      | Kind.Jump => ()
                  val _ =
                     if 0 = !Control.Native.commented
                        then ()
                     else print (let open Layout
                                 in toString
                                    (seq [str "\t/* live: ",
                                          Vector.layout Live.layout live,
                                          str " */\n"])
                                 end)
                  val _ = Vector.foreach (statements, fn s =>
                                          outputStatement (s,print,pushSize,contFrameSize))
                  val _ = outputTransfer (transfer, l)
               in ()
               end) arg
            and outputTransfer (t, source: Label.t) =
               let
                  fun iff (test, a, b) =
                     (force a
                      ; C.call ("\tBNZ", [test, Label.toString a], print)
                      ; gotoLabel b
                      ; maybePrintLabel a)
                  datatype z = datatype Transfer.t
               in
                  case t of
                     Arith {prim, args, dst, overflow, success, ...} =>
                        let
                           val prim =
                              let
                                 datatype z = datatype Prim.Name.t
                                 fun const i =
                                    case Vector.sub (args, i) of
                                       Operand.Word _ => true
                                     | _ => false
                                 fun const0 () = const 0
                                 fun const1 () = const 1
                              in
                                 case Prim.name prim of
                                    Word_addCheck _ =>
                                       concat [Prim.toString prim,
                                               if const0 ()
                                                  then "CX"
                                               else if const1 ()
                                                       then "XC"
                                                    else ""]
                                  | Word_mulCheck _ => Prim.toString prim
                                  | Word_negCheck _ => Prim.toString prim
                                  | Word_subCheck _ =>
                                       concat [Prim.toString prim,
                                               if const0 ()
                                                  then "CX"
                                               else if const1 ()
                                                       then "XC"
                                                    else ""]
                                  | _ => Error.bug "CCodegen.outputTransfer: Arith"
                              end
                           val _ = force overflow
                        in
                           print "\t"
                           ; C.call (prim,
                                     operandToString dst
                                     :: (Vector.toListMap (args, operandToString)
                                         @ [Label.toString overflow]),
                                     print)
                           ; gotoLabel success
                           ; maybePrintLabel overflow
                        end
                   | CCall {args, frameInfo, func, return} =>
                        let
                           val CFunction.T {return = returnTy,
                                            target, ...} = func
                           val (args, afterCall) =
                              case frameInfo of
                                 NONE =>
                                    (Vector.toListMap (args, fetchOperand),
                                     fn () => ())
                               | SOME frameInfo =>
                                    let
                                       val size =
                                          Program.frameSize (program, frameInfo)
                                       val res = copyArgs args
                                       val _ = push (valOf return, size)
                                    in
                                       res
                                    end
                           val _ =
                              if CFunction.modifiesFrontier func
                                 then print "\tFlushFrontier();\n"
                              else ()
                           val _ =
                              if CFunction.readsStackTop func
                                 then print "\tFlushStackTop();\n"
                              else ()
                           val _ = print "\t"
                           val _ =
                              if Type.isUnit returnTy
                                 then ()
                              else print (concat [creturn returnTy, " = /*J2*/ "])
                           datatype z = datatype CFunction.Target.t
                           val _ =
                              case target of
                                 Direct name => C.call (name, args, print)
                               | Indirect =>
                                    let
                                       val (fptr,args) =
                                          case args of
                                             (fptr::args) => (fptr, args)
                                           | _ => Error.bug "CCodegen.outputTransfer: CCall,Indirect"
                                       val name =
                                          concat ["(*(",
                                                  CFunction.cPointerType func,
                                                  " ", fptr, "))"]
                                    in
                                       C.call (name, args, print)
                                    end
                           val _ = afterCall ()
                           val _ =
                              if CFunction.modifiesFrontier func
                                 then print "\tCacheFrontier();\n"
                              else ()
                           val _ =
                              if CFunction.writesStackTop func
                                 then print "\tCacheStackTop();\n"
                              else ()
                           val _ =
                              if CFunction.maySwitchThreads func
                                 then print "\tReturn();\n"
                              else Option.app (return, gotoLabel)
                        in
                           ()
                        end
                   | Call {label, return, ...} =>
                        let
                           val dstChunk = labelChunk label
                           val _ =
                              case return of
                                 NONE => ()
                               | SOME {return, size, ...} =>
                                    push (return, size)
                        in
                           if ChunkLabel.equals (labelChunk source, dstChunk)
                              then gotoLabel label
                           else
                              C.call ("\tFarJump",
                                      [chunkLabelToString dstChunk,
                                       labelToStringIndex label],
                                      print)
                        end
                   | Goto dst => gotoLabel dst
                   | Raise => C.call ("\tRaise", [], print)
                   | Return => C.call ("\tReturn", [], print)
                   | Switch switch =>
                        let
                           fun bool (test: Operand.t, t, f) =
                              iff (operandToString test, t, f)
                           fun doit {cases: (string * Label.t) vector,
                                     default: Label.t option,
                                     test: Operand.t}: unit =
                              let
                                 val test = operandToString test
                                 fun switch (cases: (string * Label.t) vector,
                                             default: Label.t): unit =
                                    (print "switch ("
                                     ; print test
                                     ; print ") {\n"
                                     ; (Vector.foreach
                                        (cases, fn (n, l) => (print "case "
                                                              ; print n
                                                              ; print ":\n"
                                                              ; gotoLabel l)))
                                     ; print "default:\n"
                                     ; gotoLabel default
                                     ; print "}\n")
                              in
                                 case (Vector.length cases, default) of
                                    (0, NONE) =>
                                       Error.bug "CCodegen.outputTransfers: Switch"
                                  | (0, SOME l) => gotoLabel l
                                  | (1, NONE) =>
                                       gotoLabel (#2 (Vector.sub (cases, 0)))
                                  | (_, NONE) =>
                                       switch (Vector.dropPrefix (cases, 1),
                                               #2 (Vector.sub (cases, 0)))
                                  | (_, SOME l) => switch (cases, l)
                              end
                           val Switch.T {cases, default, test, ...} = switch
                           fun normal () =
                              doit {cases = Vector.map (cases, fn (c, l) =>
                                                        (WordX.toC c, l)),
                                    default = default,
                                    test = test}
                        in
                           if 2 = Vector.length cases
                              andalso Option.isNone default
                              then
                                 let
                                    val (c0, l0) = Vector.sub (cases, 0)
                                    val (c1, l1) = Vector.sub (cases, 1)
                                    val i0 = WordX.toIntInf c0
                                    val i1 = WordX.toIntInf c1
                                 in
                                    if i0 = 0 andalso i1 = 1
                                       then bool (test, l1, l0)
                                    else if i0 = 1 andalso i1 = 0
                                            then bool (test, l0, l1)
                                         else normal ()
                                 end
                           else normal ()
                        end
               end
            fun declareRegisters () =
               List.foreach
               (CType.all, fn t =>
                let
                   val pre = concat ["\t", CType.toString t, " ",
                                     CType.name t, "_"]
                in
                   Int.for (0, 1 + regMax t, fn i =>
                            print (concat [pre, C.int i, "[PTHREAD_MAX];\n"]))
                end)
            fun outputOffsets () =
               List.foreach
               ([("ExnStackOffset", GCField.ExnStack),
                 ("CurrentThreadOffset", GCField.CurrentThread),
                 ("FrontierOffset", GCField.Frontier),
                 ("StackDepthOffset", GCField.StackDepth),
                 ("CurrentFrameOffset", GCField.CurrentFrame),
                 ("RTSyncOffset",GCField.RTSync)],
                fn (name, f) =>
                print (concat ["#define ", name, " ",
                               Bytes.toString (GCField.offset f), "\n"]))
         in
            outputIncludes (["c-chunk.h"], print)
            ; outputOffsets ()
            ; declareGlobals ("PRIVATE extern ", print)
            ; declareFFI (chunk, {print = print})
            ; declareChunks ()
            ; declareProfileLabels ()
            ; C.callNoSemi ("Chunk", [chunkLabelToString chunkLabel], print)
            ; print "\n"
            ; declareRegisters ()
            ; C.callNoSemi ("ChunkSwitch", [chunkLabelToString chunkLabel],
                            print)
            ; print "\n"
            ; Vector.foreach (blocks, fn Block.T {kind, label, transfer, ...} =>
                              if Kind.isEntry kind
                                 then (print "case "
                                       ; print (labelToStringIndex label)
                                       ; print ":\n"
                                       ; print ("/*Kind: "^Kind.toString(kind)^
                                       "Transfer: "^Transfer.toString(transfer)^
                                       " */\n")
                                       ; gotoLabel label)
                              else ())
            ; print "EndChunk\n"
            ; done ()
         end
      val additionalMainArgs =
         [chunkLabelToString chunkLabel,
          labelToStringIndex label]
      val {print, done, ...} = outputC ()
      fun rest () =
         (List.foreach (chunks, fn c => declareChunk (c, print))
          ; print "PRIVATE struct cont ( *nextChunks []) () = {"
          ; Vector.foreach (entryLabels, fn l =>
                            let
                               val {chunkLabel, ...} = labelInfo l
                            in
                               print "\t"
                               ; C.callNoSemi ("Chunkp",
                                               [chunkLabelToString chunkLabel],
                                               print)
                               ; print ",\n"
                            end)
          ; print "};\n")
      val _ =
         outputDeclarations {additionalMainArgs = additionalMainArgs,
                             includes = ["c-main.h"],
                             program = program,
                             print = print,
                             rest = rest}
      val _ = done ()
      val _ = List.foreach (chunks, outputChunk)
   in
      ()
   end

end

