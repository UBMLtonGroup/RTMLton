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
    structure Prim = Primitive.MLton.Thread
    open MLtonThread
    
    fun die (s: string): 'a =
   (PrimitiveFFI.Stdio.print s
    ; PrimitiveFFI.Posix.Process.exit 1
    ; let exception DieFailed
      in raise DieFailed
      end)

  structure Queue:
   sig
      type 'a t

      val new: unit -> 'a t
      val enque: 'a t * 'a -> unit
      val deque: 'a t -> 'a option
   end =
   struct
      datatype 'a t = T of {front: 'a list ref, back: 'a list ref}

      fun new () = T {front = ref [], back = ref []}

      fun enque (T {back, ...}, x) = back := x :: !back

      fun deque (T {front, back}) =
         case !front of
            [] => (case !back of
                      [] => NONE
                    | l => let val l = rev l
                           in case l of
                              [] => raise Fail "deque"
                            | x :: l => (back := []; front := l; SOME x)
                           end)
          | x :: l => (front := l; SOME x)
   end

  structure NPThread:
   sig
      val exit: unit -> 'a
      val run: unit -> unit
      val spawn: (unit -> unit) -> unit
      val yield: unit -> unit
   end =
   struct
      open MLton
      open MLtonThread

      val topLevel: Runnable.t option ref = ref NONE

      local
         val threads: Runnable.t Queue.t = Queue.new ()
      in
         fun ready (t: Runnable.t) : unit =
            Queue.enque(threads, t)
         fun next () : Runnable.t =
            case Queue.deque threads of
               NONE => valOf (!topLevel)
             | SOME t => t
      end

      fun 'a exit (): 'a = switch (fn _ => next ())

      fun new (f: unit -> unit): Runnable.t =
         prepare
         (MLtonThread.new (fn () => ((f () handle _ => exit ())
                                ; exit ())),
          ())

      fun schedule t = (ready t; next ())

      fun yield (): unit = switch (fn t => schedule (prepare (t, ())))

      val spawn = ready o new

      fun run(): unit =
         (switchRT (fn t =>
                  (topLevel := SOME (prepare (t, ()))
                   ; next()))
          ; topLevel := NONE)
   end

    structure PThread =
    struct
        open Prim
        open MLtonThread
        val savedPre = fn () => savedPre Primitive.MLton.GCState.gcState
        val copyCurrent = fn () => copyCurrent ()
        val copy = fn (base) => copy(base)
        
        local
            val tf = SOME (fn () => print("Almost there\n"))
            val func: (unit -> unit) option ref = ref tf
            val base: Prim.preThread =
              let
                val () = copyCurrent ()
              in
                    case !func of
                    NONE => savedPre ()
                    | SOME x =>
                        (* This branch never returns. *)
                        let
                     (* Atomic 1 *)
                            val _ = print("in Base some x\n");
                            val () = func := NONE
                            (*val () = atomicEnd ()*)
                     (* Atomic 0 *)
                        in
                            (x () handle e => MLtonExn.topLevelHandler e)
                            ; die "Thread didn't exit properly.\n"
                        end
              end
        in
            fun newThread (f: unit -> unit) : Prim.thread =
                let
                    (* Atomic 2 *)
                     val () = func := SOME f
                 in
                   (print("In newThread\n");
                    Prim.copy base)
                end
        end

         
        val spawn = fn (f) => NPThread.spawn f
        val run = fn () => NPThread.run ()
          

    end

    type 'a t = (unit -> 'a) -> unit

    fun thread_main () = (*MLtonThread.run ()*)print "Parallel_run::thread_main running!"
		
    val () = (_export "Parallel_run": (unit -> unit) -> unit;) thread_main

    fun myPriority () : int = Prim.myPriority(Primitive.MLton.GCState.gcState)
    fun setBooted () : int = 
      let
        val _ = Prim.setBooted(1,Primitive.MLton.GCState.gcState);
      in
        Prim.yield(Primitive.MLton.GCState.gcState)
      end

    fun gcSafePoint () : int = Prim.gcSafePoint(1)

end

