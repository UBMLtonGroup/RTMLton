val funnyname = 2+2;

open MLton.PrimThread
open MLton.Thread

val _ = print "The following is a test, ignore:\n"


(*val _ = (thread_main ())

val _ = print "Calling rtinit...\n"
val _ = (rtinit gcstate)*)


val _ = print "\n\nTest app exits... (this will hang until we shut down all threads)\n"
