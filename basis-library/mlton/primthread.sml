(* Copyright (C) 1999-2006, 2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

structure PrimThread : PRIM_THREAD =
struct
    open Primitive
    open Primitive.MLton.Thread
    open MLtonThread

    structure PThread =
    struct
        open Primitive.MLton.Thread
        val savedPre = fn () => savedPre Primitive.MLton.GCState.gcState
        val copyCurrent = fn () => copyCurrent ()
        val copy = fn (base) => copy(base)
    end

    type 'a t = (unit -> 'a) -> unit

    fun thread_main () = (*MLtonThread.run ()*)print "Parallel_run::thread_main running!"
		
    val () = (_export "Parallel_run": (unit -> unit) -> unit;) thread_main

    fun gcSafePoint () : int = Primitive.MLton.Thread.gcSafePoint(1)

end

