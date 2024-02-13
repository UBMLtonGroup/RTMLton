val _ = print "something\n"
val _ = print "more stuff\n"

open MLton.PrimThread

(* arg is thread number *)
val _ = (     dump_instrument_stderr 0;
      dump_instrument_stderr 1)

