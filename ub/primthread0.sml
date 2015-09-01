open MLton.PrimThread

fun loop 0 = 0
  | loop (n:int) = 
  let
	in
	    MLton.PrimThread.PThread.copyCurrent() ;
	    loop(n - 1) 
	end


(* val _ = loop 10 ignore *)

val () = MLton.PrimThread.PThread.copyCurrent() 

val sp = MLton.PrimThread.PThread.savedPre()

val p = setPriority(sp, 5)

fun f() = 2

fun e() = 3

val a = ref e

val b = !a()

val _ = print (Int.toString(b))

val p = getPriority(sp)

val _ = print ("Priority is: " ^ Int.toString(p) ^ "\n")


val r = setRunnable(sp)

val _ = print ("Runnable said: " ^ Int.toString(r) ^ "\n")

val rec delay =
   fn 0 => ()
    | n => delay (n - 1)
    
val rec loop =
   fn 0 => ()
    | n => (delay 50000000; loop (n - 1))

val _ = loop 100



val _ = print "Done\n"
