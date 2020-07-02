(* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

signature MLTON_GC =
   sig
      val collect: unit -> unit
      val pack: unit -> unit
      val setMessages: bool -> unit
      val setSummary: bool -> unit
      val unpack: unit -> unit

      (* Most meaningful immediately after 'collect()'. *)
      structure Statistics :
         sig
            val bytesAllocated: unit -> IntInf.int
            val numSweeps: unit -> IntInf.int
            val totalGCTime: unit -> IntInf.int
            val numChunksFreed: unit -> IntInf.int
            val maxMutatorPause: unit -> IntInf.int
         end
   end
