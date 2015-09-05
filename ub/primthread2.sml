open MLton.PrimThread

fun die (s: string): 'a = let in print s ; raise Fail "child failed" end
      
local
    val func: (unit -> unit) option ref = ref NONE
    val base: PThread.preThread =
       let
          val () = MLton.PrimThread.PThread.copyCurrent ()
       in
          case !func of
             NONE => let val _ = print "parent?\n" in  MLton.PrimThread.PThread.savedPre () end
           | SOME x =>
                (* This branch never returns. *)
              let
                 (* Atomic 1 *)
                 val () = func := NONE
                 (* Atomic 0 *)
              in
		 print "calling X\n"
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
	val a = ref e
	val () = MLton.PrimThread.PThread.copyCurrent() 
	val sp = MLton.PrimThread.PThread.savedPre()
	
	(* this should show the thread is in queue #0 *)
	
	val _ = displayThreadQueue(0)
	val p = setPriority(sp, 5)
	
	(* this should show the thread moved to queue #5 *)
	
	val _ = displayThreadQueue(0)
	val p = getPriority(sp)
	val _ = print ("Priority is: " ^ Int.toString(p) ^ "\n")
	val r = setRunnable(sp)
	val _ = print ("Runnable said: " ^ Int.toString(r) ^ "\n")
	val _  = newThread e
	(* if we are in the thread, then call !a() otherwise dont *)

val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (delay 50000000; loop (n - 1))

val _ = loop 100

val _ = print "Done\n"
