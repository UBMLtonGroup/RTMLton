(* n-threads.sml
 * 
 * create N rt threads using pspawn
 * notes: N must be equal to the number of threads (MAXPRI) 
 *        configured in the run time
 *)
open MLton.PrimThread

fun printit2 s = ()
fun printit s = print (Int.toString(getMyPriority ())^"] "^s^"\n")
fun gettime () = get_ticks_since_boot ()

val runtime    = 1000 (* 1 ms *)
val deadline   = 4000 (* 4 ms *)
val period     = 8000 (* 8 ms *)

fun checkDeadline (cur : real, dl : int) =
   let
      val dl_in_secs = Real./(Real.fromInt(dl), 1000.0)
   in
      if (Real.>(cur, dl_in_secs)) then
         printit "DEADLINE MISSED"
      else ()
   end



fun threadN n =
let
   val threadname = "thread #"^Int.toString(n)
   val prev = ref (gettime ())
   val cur = ref (gettime ())
   (* reminder: sched_runtime <= sched_deadline <= sched_period  *)
   val _ = set_schedule (runtime, deadline, period, 0) (* runtime, deadline, period, allowedtopacks *)
in
   while true do (
      cur := gettime();
      printit (threadname^": runtime "^Real.toString(Real.-(!prev, !cur)));
      checkDeadline (Real.-(!cur, !prev), deadline);
      prev := !cur;
      wait_for_next_period false (* after computation finishes, this must be called but on rtems does nothng*)
   )
end

(* note: thread #1 is the GC *)

val _ = printit "create thread 2";

val _ = pspawn (fn () => let in threadN 2 end, 2)

val _ = printit "create thread 3";

val _ = pspawn (fn () => let in threadN 3 end, 3)

val _ = printit "create thread 4";

val _ = pspawn (fn () => let in threadN 4 end, 4)

val _ = printit "create thread 5";

val _ = pspawn (fn () => let in threadN 5 end, 5)


val _ = printit "thread 0: Main loop ";

fun loop () = (printit "main loop"; Posix.Process.sleep (Time.fromSeconds 1) ; loop ())
val _ = loop ()
