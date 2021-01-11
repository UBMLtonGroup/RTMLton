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
val _ = print "before open\n"
        open Primitive.MLton.Thread
val _ = print "after open\n"
        val savedPre = fn () => savedPre Primitive.MLton.GCState.gcState
val _ = print "After savedPre\n"
        val copyCurrent = fn () => copyCurrent ()
val _ = print "After copyCurrent\n"
        val copy = fn (base) => copy(base)
val _ = print "After copy\n"
    end

    type 'a t = (unit -> 'a) -> unit

    fun thread_main () = (*MLtonThread.run ()*) print "\n\nParallel_run::thread_main running!\n\n"

    val () = (_export "Parallel_run": (unit -> unit) -> unit;) thread_main
    val rtinit = _import "RT_init": Primitive.MLton.GCState.t -> unit;

    val gcstate = Primitive.MLton.GCState.gcState

    (*
    val () = (_import "RT_init": Primitive.MLton.GCState.t -> unit;) Primitive.MLton.GCState.gcState
    *)
end

