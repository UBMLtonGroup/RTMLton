(* at this point, basis is booting, once callFromCHandler
   is set, the following SML code will execute via
   the trampoline in posix thread 0. If you do not
   see the print output, then the trampoline isnt working *)
open OS.Process
open MLton 
open PrimThread

val a = 9320
val _ = print "test2 was running and is going to sleep\n"

fun inf_sleep t= OS.Process.sleep t

val _ = inf_sleep (Time.fromSeconds 5)

(*fun fib n =
  if n < 3 then 
    1
  else
    fib (n-1) + fib (n-2)

fun printfib n = print ( Int.toString (fib (n)) ^ "\n ************** \n" )
  *)
val _ = PrimThread.setBooted()

(*local
    open Thread
in
    val _ = MLton.Thread.spawn (fn () => printfib (10))
end
*)

fun pause () = (print "sleep\n"; inf_sleep (Time.fromSeconds 5)
                ;PrimThread.gcSafePoint(); pause ())

val _ = pause ()


