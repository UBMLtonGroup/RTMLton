(* Copyright (C) 2009 Matthew Fluet.
 * Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

signature RUNTIME_STRUCTS =
   sig
   end

signature RUNTIME =
   sig
      include RUNTIME_STRUCTS

      structure GCField:
         sig
            datatype t =
               AtomicState
             | CardMapAbsolute
             | CurrentThread
             | CurSourceSeqsIndex
             | ExnStack
             | Frontier (* The place where the next object is allocated. *)
             | UMFrontier (* The place where the next object is allocated on the uniform memory heap. *)
             | Limit (* frontier + heapSize - LIMIT_SLOP *)
             | LimitPlusSlop (* frontier + heapSize *)
             | MaxFrameSize
             | SignalIsPending
             | StackBottom
             | StackLimit (* Must have StackTop <= StackLimit *)
             | StackTop (* Points at the next available byte on the stack. *)
             | FLChunks
             | RTSync
             | StackTopStash
             | HeuristicChunks
             | FLLock
             | Reserved

            val layout: t -> Layout.t
            val offset: t -> Bytes.t (* Field offset in struct GC_state. *)
            val setOffsets: {atomicState: Bytes.t,
                             cardMapAbsolute: Bytes.t,
                             currentThread: Bytes.t,
                             curSourceSeqsIndex: Bytes.t,
                             exnStack: Bytes.t,
                             frontier: Bytes.t,
                             umfrontier: Bytes.t,
                             limit: Bytes.t,
                             limitPlusSlop: Bytes.t,
                             maxFrameSize: Bytes.t,
                             signalIsPending: Bytes.t,
                             stackBottom: Bytes.t,
                             stackLimit: Bytes.t,
                             stackTop: Bytes.t,
                             flChunks: Bytes.t,
                             rtSync: Bytes.t ,
                             stackTopStash: Bytes.t,
                             heuristicChunks: Bytes.t,
                             flLock: Bytes.t,
                             reserved: Bytes.t} -> unit
            val setSizes: {atomicState: Bytes.t,
                           cardMapAbsolute: Bytes.t,
                           currentThread: Bytes.t,
                           curSourceSeqsIndex: Bytes.t,
                           exnStack: Bytes.t,
                           frontier: Bytes.t,
                           umfrontier: Bytes.t,
                           limit: Bytes.t,
                           limitPlusSlop: Bytes.t,
                           maxFrameSize: Bytes.t,
                           signalIsPending: Bytes.t,
                           stackBottom: Bytes.t,
                           stackLimit: Bytes.t,
                           stackTop: Bytes.t,
                           flChunks: Bytes.t,
                           rtSync:Bytes.t ,
                           stackTopStash : Bytes.t,
                           heuristicChunks:Bytes.t,
                           flLock: Bytes.t,
                           reserved: Bytes.t} -> unit
            val size: t -> Bytes.t (* Field size in struct GC_state. *)
            val toString: t -> string
         end
      structure RObjectType:
         sig
            datatype t =
               Array of {hasIdentity: bool,
                         bytesNonObjptrs: Bytes.t,
                         numObjptrs: int}
             | Normal of {hasIdentity: bool,
                          bytesNonObjptrs: Bytes.t,
                          numObjptrs: int}
             | Stack
             | Weak of {gone: bool}
         end

      val arrayLengthOffset: unit -> Bytes.t
      val arrayLengthSize: unit -> Bytes.t
      val headerOffset: unit -> Bytes.t
      val headerSize: unit -> Bytes.t
      val headerToTypeIndex: word -> int
      val labelSize: unit -> Bytes.t
      val limitSlop: Bytes.t
      val maxFrameSize: Bytes.t
      val cpointerSize: unit -> Bytes.t
      val objptrSize: unit -> Bytes.t
      val typeIndexToHeader: int -> word
      val objChunkSize: unit -> Bytes.t
      val objChunkSlop: word
   end
