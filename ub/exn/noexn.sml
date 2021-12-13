val cmd = CommandLine.name ()
val args = CommandLine.arguments ()

fun intFromString(s) = case Int.fromString(s) of SOME x => x | NONE => 0;

fun loop n =
   if n = 0
      then true
   else (loop(n - 1) = (print "z"; true))

val _ = loop (intFromString(hd(args))) = (print "done\n" ; true)
