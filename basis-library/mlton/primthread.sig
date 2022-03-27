(* Copyright (C) 1999-2005, 2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

signature PRIM_THREAD =
   sig

      structure PreThread : 
      sig 
         type t 
      end
      
      structure Thread :
      sig
         type t
      end
      
      structure PThread :
      sig
         type preThread = PreThread.t
         type thread = Thread.t
         val savedPre: unit -> preThread
         val copyCurrent: unit -> unit
         val copy: preThread -> Thread.t   
         val spawnp: (unit->unit) -> unit
         val run: unit ->unit      
      end

      type 'a t

      val get_ticks_since_boot : unit -> real
      val dump_instrument_stderr : int -> unit
      val instrument : int -> unit

      val rtlock : int -> unit
      val rtunlock : int -> unit
      val pspawn: (unit->unit) * int -> unit
      val thread_main: unit -> unit
      val thread_main_rtems: unit -> unit
      val gcstate: Primitive.MLton.GCState.t

      val getMyPriority: unit -> int
      val set_schedule: int * int * int * int -> unit
      val schedule_yield: bool -> unit
      val wait_for_next_period: bool -> unit

 end
