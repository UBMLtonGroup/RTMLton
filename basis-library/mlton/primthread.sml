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

    structure PThread =
    struct
        open Primitive.MLton.Thread
        val savedPre = fn () => savedPre Primitive.MLton.GCState.gcState
        val copyCurrent = fn () => copyCurrent ()
    end

    type 'a t = (unit -> 'a) -> unit

	  fun setPriority (pt : PThread.preThread, prio' : int) : int = 
	  let
	    val _ = print "PrimThread.setPriority\n"
	  in
	  	Primitive.MLton.Thread.setPriority(Primitive.MLton.GCState.gcState, pt, prio')
	  end

end

