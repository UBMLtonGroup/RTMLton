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


  fun die (s: string): 'a =
   (PrimitiveFFI.Stdio.print s
    ; PrimitiveFFI.Posix.Process.exit 1
    ; let exception DieFailed
      in raise DieFailed
      end)

  fun dbg (s: string): unit = 
    if false 
    then PrimitiveFFI.Stdio.print (Int.toString (pthreadNum ())^">> "^s)
    else ()


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
     (* val loop: unit -> unit*)
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
         (switch (fn t =>
                  (topLevel := SOME (prepare (t, ()))
                   ; next()))
          ; topLevel := NONE)

      (*fun loop () =
        case empty () of
             true => loop ()
           | false =>  (run () ; loop ())*)
   end


    structure PThread =
    struct
        val _ = dbg "before open\n"
        open Primitive.MLton.Thread
        val _ = dbg "after open\n"
        val savedPre = fn () => savedPre Primitive.MLton.GCState.gcState
        val _ = dbg "After savedPre\n"
        val copyCurrent = fn () => copyCurrent ()
        val _ = dbg "After copyCurrent\n"
        val copy = fn (base) => copy(base)
        val _ = dbg "After copy\n"


         
        val spawnp = fn (f) => NPThread.spawn f
        val run = fn () => NPThread.run ()

    end

  local
    val lock_ = _import "ML_lock": unit -> unit;
    val unlock_ = _import "ML_unlock": unit -> unit;
  in
    fun lock () = lock_ ()
    fun unlock () = unlock_ ()
  end

  local
    val lock__ = _import "User_lock" : int -> unit;
    val unlock__ = _import "User_unlock" : int -> unit;
  in
    fun rtlock a = if a <=9 then lock__ a else print "Invalid lock. Valid locks are [0-9]\n"
    fun rtunlock a = if a<=9 then unlock__ a else print "Invalid lock. Valid locks are [0-9]\n"
  end 



  structure WorkQueue:
    sig
      type 'a t

      val new : unit -> 'a t
      val addWork : 'a t * 'a * int -> unit
      val getWork : 'a t * int -> 'a option
    end = 
    struct
      datatype 'a t = T of 'a list Array.array

      fun new () = T (Array.tabulate(numberOfPThreads (), fn _ => []))
      
      fun addWork (T wq, w, p) = 
      let 
      in
        lock ();
        Array.unsafeUpdate (wq, p, Array.unsafeSub (wq, p) @ [w]);
        unlock ()
      end
      handle Subscript => ( unlock(); die "Invalid priority")


      fun getWork (T wq, p) =
      let in
        lock ();
        case Array.unsafeSub (wq, p) of 
             [] => (unlock(); NONE)
           | w :: l => (Array.unsafeUpdate (wq, p, l); unlock(); SOME w)
      end
      handle Subscript => (unlock (); die "Invalid priority")

    end




    type 'a t = (unit -> 'a) -> unit

    val workQ : (unit->unit) WorkQueue.t = WorkQueue.new ()

    
    fun loop p =
      case WorkQueue.getWork (workQ, p) of
           NONE => loop p
         | SOME w => (dbg "Working .. \n";w () ; loop p)

    fun test () = print "Parallel_run::thread_main running!\n";

    fun pspawn (f: unit->unit, p: int) = WorkQueue.addWork(workQ, f, p) 

    fun thread_main () = (*MLtonThread.run ()print
      "\n\nParallel_run::thread_main running!\n\n"*) ( dbg "looping...\n";
      WorkQueue.addWork(workQ, test, 2); loop (pthreadNum ()) ) 

    val gcstate = Primitive.MLton.GCState.gcState

    val () = (_export "Parallel_run": (unit -> unit) -> unit;) thread_main

(*  val rtinit = _import "RT_init": Primitive.MLton.GCState.t -> unit;*)


    val () = (_import "RT_init": Primitive.MLton.GCState.t -> unit;) Primitive.MLton.GCState.gcState
   


end

