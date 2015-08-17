(* Copyright (C) 1999-2005, 2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

signature PRIM_THREAD =
   sig

      type 'a t
(*
			type preThread = PreThread.t

			val savedPre: unit -> preThread
	*)

   val primthreadtest: string -> int

   end
