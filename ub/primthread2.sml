open MLton.PrimThread

val _ = print "primthread2 starts\n"

fun die (s: string): 'a = let in print s ; raise Fail "child failed" end


structure Runnable =
   struct
      type t = unit t
   end
   
local
	local
	    val func: (unit -> unit) option ref = ref NONE
	    val base: PThread.preThread =
	       let
	       		val _ = print "1. copy current..\n";
	          val () = MLton.PrimThread.PThread.copyCurrent ()
	       in
	          case !func of
	             NONE => let val _ = print "2. in parent-thread\n" in MLton.PrimThread.PThread.savedPre () end
	           | SOME x =>
	                (* This branch never returns. *)
	              let
	                 (* Atomic 1 *)
	                 val _ = print "in child let\n";
	                 val () = func := NONE
	                 (* Atomic 0 *)
	              in
	                 print "in sub-thread: calling X\n"
	                 ; x () 
	                 ; die "Thread didn't exit properly.\n"
	              end
	       end
	in
	    fun newThread (f: unit -> unit): PThread.thread =
	       let
	          (* Atomic 2 *)
	          val _ = print "4. setting func to f\n" ;
	          val () = func := SOME f
	       in
	          print "5. in newThread, copy'ing the latest thread\n" ;
	          MLton.PrimThread.PThread.copy base
	       end
	end
in
  fun doit (f: unit -> unit): int = 
  let
  	val t = newThread f
  	val _ = setPriority2(t, 5)
  	val _ = setRunnable2(t)
  in
  	print "hi\n" ; 1
  end
end

fun e () = print "In function e()\n"

val _ = print "3. calling newThread\n"

val T  = doit e


(* delay the parent for 5s or so *)

val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (delay 50000000; loop (n - 1))

val _ = loop 100

val _ = print "Done\n"
