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

		
    fun setPriority (pt : PThread.preThread, prio' : int) : int = Primitive.MLton.Thread.setPriority(Primitive.MLton.GCState.gcState, pt, prio')
    fun getPriority (pt : PThread.preThread) : int = Primitive.MLton.Thread.getPriority(Primitive.MLton.GCState.gcState, pt)

    fun setPriority2 (t : PThread.thread, prio' : int) : int = Primitive.MLton.Thread.setPriority2(Primitive.MLton.GCState.gcState, t, prio')
    fun getPriority2 (t : PThread.thread) : int = Primitive.MLton.Thread.getPriority2(Primitive.MLton.GCState.gcState, t)
    
    fun setRunnable2 (t : PThread.thread) : int = Primitive.MLton.Thread.setRunnable2(Primitive.MLton.GCState.gcState, t)
    fun setRunnable (pt : PThread.preThread) : int = Primitive.MLton.Thread.setRunnable(Primitive.MLton.GCState.gcState, pt)
	  
	  
    fun displayThreadQueue (x : int) : int = Primitive.MLton.Thread.displayThreadQueue(Primitive.MLton.GCState.gcState, x)
end

