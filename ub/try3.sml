val cmd = CommandLine.name ()
val args = CommandLine.arguments ()

fun intFromString(s) = case Int.fromString(s) of SOME x => x | NONE => 0;

exception A;
exception B;
exception C;

fun c n = if n > 1 then raise C else 8
fun b n = if n > 2 then raise B else c(n) handle C => 7
fun a n = if n > 3 then raise A else b(n) handle B => 6

fun doit n = a n
  handle A => (print "got A\n" ; 0)
       | B => (print "got B\n" ; 0)
       | C => (print "got C\n" ; 0)


val () =
  if List.length args = 0
  then let
       in
          print cmd ; print " [heap bytes]\n" ;
          ()
       end
  else (print ("rv: " ^ Int.toString(doit (intFromString(hd(args)))) ^ "\n") ; ())

