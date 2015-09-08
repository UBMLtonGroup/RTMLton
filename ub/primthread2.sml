open MLton.PrimThread

fun die (s: string): 'a = let in print s ; raise Fail "child failed" end
      
local
    val func: (unit -> unit) option ref = ref NONE
    val base: PThread.preThread =
       let
          val () = MLton.PrimThread.PThread.copyCurrent ()
       in
          case !func of
             NONE => let val _ = print "in parent-thread\n" in MLton.PrimThread.PThread.savedPre () end
           | SOME x =>
                (* This branch never returns. *)
              let
                 (* Atomic 1 *)
                 val () = func := NONE
                 (* Atomic 0 *)
              in
                 print "in sub-thread: calling X\n"
                 ; x () 
                 ; die "Thread didn't exit properly.\n"
              end
       end
in
    fun newThread (f: unit -> unit) : PThread.thread =
       let
          (* Atomic 2 *)
          val () = func := SOME f
       in
          print "in newThread\n" ;
          MLton.PrimThread.PThread.copy base
       end
end


fun e () = print "In function e()\n"

val T  = newThread e
val _ = setPriority2(T, 5)
val _ = setRunnable2(T)


(* delay the parent for 5s or so *)

val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (delay 50000000; loop (n - 1))

val _ = loop 100

val _ = print "Done\n"
