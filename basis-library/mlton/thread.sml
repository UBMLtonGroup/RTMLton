(* Copyright (C) 2014 Matthew Fluet.
 * Copyright (C) 2004-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 *)

structure MLtonThread:> MLTON_THREAD_EXTRA =
struct

structure Prim = Primitive.MLton.Thread

fun die (s: string): 'a =
   (PrimitiveFFI.Stdio.print s
    ; PrimitiveFFI.Posix.Process.exit 1
    ; let exception DieFailed
      in raise DieFailed
      end)


fun dbg (s: string): unit = 
  if false then PrimitiveFFI.Stdio.print (Int.toString (Prim.pthreadNum ())^s) else
    ()

val gcState = Primitive.MLton.GCState.gcState

structure AtomicState =
   struct
      datatype t = NonAtomic | Atomic of int
   end

local
   open Prim
in
   val atomicBegin = atomicBegin
   val atomicEnd = atomicEnd
   val atomicState = fn () =>
      case atomicState () of
         0wx0 => AtomicState.NonAtomic
       | w => AtomicState.Atomic (Word32.toInt w)
end

fun atomically f =
   (atomicBegin (); DynamicWind.wind (f, atomicEnd))

datatype 'a thread =
   Dead
 | Interrupted of Prim.thread
 | New of 'a -> unit
 (* In Paused (f, t), f is guaranteed to not raise an exception. *)
 | Paused of ((unit -> 'a) -> unit) * Prim.thread

datatype 'a t = T of 'a thread ref

structure Runnable =
   struct
      type t = unit t
   end

fun prepend (T r: 'a t, f: 'b -> 'a): 'b t =
   let
      val t =
         case !r of
            Dead => raise Fail "prepend to a Dead thread"
          | Interrupted _ => raise Fail "prepend to a Interrupted thread"
          | New g => New (g o f)
          | Paused (g, t) => Paused (fn h => g (f o h), t)
   in r := Dead
      ; dbg ">>> mltonthread: prepend\n"
      ; T (ref t)
   end

fun prepare (t: 'a t, v: 'a): Runnable.t = (
     dbg ">>> mltonthread: prepare\n"
   ; prepend (t, fn () => v)
   )

fun new f = T (ref (New f))

local
   val maxpri = Prim.numberOfPThreads ()
   val pNum = Prim.pthreadNum
   local
      val () = dbg ">>>> mltonthread: building base\n"
   (* Create one reference per PThread *)
      val func: (unit -> unit) option Array.array = 
        Array.tabulate (maxpri, fn _ => NONE)
      val base: Prim.preThread =
         let
            val () = Prim.copyCurrent ()
            (*should come after copycurrent acc to spoons*)
            val pthread = pNum ()
         in
            case Array.unsafeSub (func, pthread) of
               NONE => Prim.savedPre gcState
             | SOME x =>
                  (* This branch never returns. *)
                  let
                     (* Atomic 1 *)
                     val () = Array.update (func, pthread, NONE)
                     val () = atomicEnd ()
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
            val () = Array.update (func, pNum (), SOME f)
         in
            (   dbg ">>> mltonthread: newThread\n"
              ; Prim.copy base
              )
         end
   end
   val switching = Array.tabulate (maxpri, fn _ => false)
in
   fun 'a atomicSwitch (f: 'a t -> Runnable.t): 'a =
      let val pthread = pNum () in 
      (* Atomic 1 *)
      if Array.unsafeSub (switching, pthread)
         then let
                 val () = atomicEnd ()
                 (* Atomic 0 *)
              in
                 raise Fail "nested Thread.switch"
              end
      else
         let
            val _ = Array.update (switching, pthread, true)
            val r : (unit -> 'a) ref = 
               ref (fn () => die "Thread.atomicSwitch didn't set r.\n")
            val t: 'a thread ref =
               ref (Paused (fn x => r := x, Prim.current gcState))
            fun fail e = (t := Dead
                          ; Array.update (switching, pthread, false)
                          ; atomicEnd ()
                          ; raise e)    
            val (T t': Runnable.t) = f (T t) handle e => fail e
            val primThread =
               case !t' before t' := Dead of
                  Dead => fail (Fail "switch to a Dead thread")
                | Interrupted t => t
                | New g => (atomicBegin (); newThread g)
                | Paused (f, t) => (f (fn () => ()); t)

            val _ = if not (Array.unsafeSub (switching, pthread))
                    then raise Fail "switching switched?"
                    else ()

            val _ = Array.update (switching, pthread, false)
            (* Atomic 1 when Paused/Interrupted, Atomic 2 when New *)
            val _ = Prim.switchTo primThread (* implicit atomicEnd() *)
            (* Atomic 0 when resuming *)
         in
            (dbg ">>> mltonthread: atomicSwitch\n" ; !r ())
         end
      end

   fun switch f =
      (atomicBegin ()
       ; atomicSwitch f)
end

fun fromPrimitive (t: Prim.thread): Runnable.t =
   (dbg ">>> mltonthread: fromPrimitive\n" ; T (ref (Interrupted t)))

fun toPrimitive (t as T r : unit t): Prim.thread =
   case !r of
      Dead => die "Thread.toPrimitive saw Dead.\n"
    | Interrupted t => 
         (r := Dead
          ; t)
    | New _ =>
       (dbg ">>> mltonthread: toPrimitive New\n"
        ;switch
         (fn cur : Prim.thread t =>
          prepare
          (prepend (t, fn () =>
                    switch
                    (fn t' : unit t =>
                     prepare (cur, toPrimitive t'))),
           ()))
           )
    | Paused (f, t) =>
         (r := Dead
          ; dbg ">>> mltonthread: toPrimitive\n"
          ; f (fn () => ()) 
          ; t)


local
   val signalHandler: Prim.thread option ref = ref NONE
   datatype state = Normal | InHandler
   val state: state ref = ref Normal
in
   fun amInSignalHandler () = InHandler = !state

   fun setSignalHandler (f: Runnable.t -> Runnable.t): unit =
      let
         val _ = Primitive.MLton.installSignalHandler ()
         fun loop (): unit =
            let
               (* Atomic 1 *)
               val _ = state := InHandler
               val t = f (fromPrimitive (Prim.saved gcState))
               val _ = state := Normal
               val _ = Prim.finishSignalHandler gcState
               val _ =
                  atomicSwitch
                  (fn (T r) =>
                   let
                      val _ =
                         case !r of
                            Paused (f, _) => f (fn () => ())
                          | _ => raise die "Thread.setSignalHandler saw strange thread"
                   in
                      t
                   end) (* implicit atomicEnd () *)
            in
               loop ()
            end
         val p =
            toPrimitive
            (new (fn () => loop () handle e => MLtonExn.topLevelHandler e))
         val _ = signalHandler := SOME p
      in
         (dbg ">>> mltonthread: setSignalHandler\n" ; Prim.setSignalHandler (gcState, p))
      end

   fun switchToSignalHandler () =
      let
         (* Atomic 0 *)
         val () = atomicBegin ()
         (* Atomic 1 *)
         val () = Prim.startSignalHandler gcState (* implicit atomicBegin () *)
         (* Atomic 2 *)
      in
      (dbg ">>> mltonthread: switchToSignalHandler\n";
         case !signalHandler of
            NONE => raise Fail "no signal handler installed"
          | SOME t => Prim.switchTo t (* implicit atomicEnd() *)
       )
      end
      
end


local

in
   val register: int * (MLtonPointer.t -> unit) -> unit =
      let
         val exports =
            Array.array (Int32.toInt (Primitive.MLton.FFI.numExports),
                         fn _ => raise Fail "undefined export")
         val worker : (Prim.thread * Prim.thread option ref) option ref = ref NONE
         fun mkWorker (): Prim.thread * Prim.thread option ref =
            let
               val thisWorker : (Prim.thread * Prim.thread option ref) option ref = ref NONE
               val savedRef : Prim.thread option ref = ref NONE
               fun workerLoop () =
                  let
                     (* Atomic 1 *)
                     val p = Primitive.MLton.FFI.getOpArgsResPtr ()
                     val _ = atomicEnd ()
                     (* Atomic 0 *)
                     val i = MLtonPointer.getInt32 (MLtonPointer.getPointer (p, 0), 0)
                     val _ =
                        (Array.sub (exports, Int32.toInt i) p)
                        handle e =>
                           (TextIO.output
                            (TextIO.stdErr, "Call from C to SML raised exception.\n")
                            ; MLtonExn.topLevelHandler e)
                     (* Atomic 0 *)
                     val _ = atomicBegin ()
                     (* Atomic 1 *)
                     val _ = worker := !thisWorker
                     val _ = Prim.setSaved (gcState, valOf (!savedRef))
                     val _ = savedRef := NONE
                     val _ = Prim.returnToC () (* implicit atomicEnd() *)
                  in
                    (print ">>>> mltonthread: workerLoop\n"; workerLoop ())
                  end
               val workerThread = toPrimitive (new workerLoop)
               val _ = thisWorker := SOME (workerThread, savedRef)
            in
              (dbg ">>>> mltonthread: mkworker\n"; (workerThread,
              savedRef))
            end
         fun handlerLoop (): unit =
            let
               (* Atomic 2 *)
               val () = dbg ">>>> mltonthread: get saved thread in handler loop\n"
               val saved = Prim.saved gcState
               val (workerThread, savedRef) =
                  case !worker of
                     NONE => mkWorker ()
                   | SOME (workerThread, savedRef) =>
                        (worker := NONE
                         ; (workerThread, savedRef))
               val _ = savedRef := SOME saved
               val _ = Prim.switchTo (workerThread) (* implicit atomicEnd() *)
            in
                ( dbg ">>>> mltonthread: handlerLoop\n"; handlerLoop ())
            end
         val handlerThread = toPrimitive (new handlerLoop)
         val _ = Prim.setCallFromCHandler (gcState, handlerThread)
      in
        (dbg ">>> mltonthread: register\n"; fn (i, f) => Array.update
        (exports, i, f))
      end
end

end
