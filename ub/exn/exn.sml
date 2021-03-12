val cmd = CommandLine.name ()
val args = CommandLine.arguments ()

fun intFromString(s) = case Int.fromString(s) of SOME x => x | NONE => 0;

exception E

fun loop n =
   if n = 0
      then raise E
   else (loop(n - 1) handle e => (print "z"; raise e))

val _ = loop (intFromString(hd(args))) handle _ => print "done\n"
