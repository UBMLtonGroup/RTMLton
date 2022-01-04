

open MLton.PrimThread

fun printit s = print (Int.toString(getMyPriority ())^"] "^s^"\n")
fun gettime () = get_ticks_since_boot ()

fun thread0 () = 
let 
   val onesec = 1000000000
   val _ = printit "In let for thread0. Setting period/deadline to 1 sec\n"
   val _ = set_schedule (onesec, onesec, onesec)
   val prev = ref (gettime ())
   val cur = ref (gettime ())
in
   while true do ( 
      instrument 0;
      cur := gettime();
      printit (Real.toString(Real.-(!cur, !prev)));
      prev := !cur;
      instrument 0;
      wait_for_next_period (); (* after computation finishes, this must be called *)
(*
      dump_instrument_stderr 0;
*)
      ()
   ) end


val _ = printit "Thread 0 : main"
val _ = thread0 ()
val _ = printit "Thread 0 : exit (should not happen)"



