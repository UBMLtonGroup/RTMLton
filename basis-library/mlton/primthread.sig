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
        type preThread = PreThread.t
        val savedPre: unit -> preThread
     end

     type 'a t
   
     val primthreadtest: string -> int

 end
