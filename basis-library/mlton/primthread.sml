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
        val copy = fn (base) => copy(base)
    end

    type 'a t = (unit -> 'a) -> unit

		fun gcState = Primitive.MLton.GCState.gcState
		
	  fun setPriority (pt : PThread.preThread, prio' : int) : int = 
	  let
	  in
	  	Primitive.MLton.Thread.setPriority(Primitive.MLton.GCState.gcState, pt, prio')
	  end

	  fun setRunnable (pt : PThread.preThread) : int = 
	  let
	  in
	  	Primitive.MLton.Thread.setRunnable(Primitive.MLton.GCState.gcState, pt)
	  end
	  
	  fun getPriority (pt : PThread.preThread) : int = 
	  let
	  in
	  	Primitive.MLton.Thread.getPriority(Primitive.MLton.GCState.gcState, pt)
	  end
	  
	  fun displayThreadQueue (x : int) : int = 
	  let
	  in
	  	Primitive.MLton.Thread.displayThreadQueue(Primitive.MLton.GCState.gcState, x)
	  end
	  
	  
end

