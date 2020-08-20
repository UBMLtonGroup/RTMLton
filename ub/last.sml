(* try to force a large stacklet chain *)

fun last [x] = x
  | last (_::xs) = last xs

fun factorial n =
  if n <= 0 then 1
  else n * factorial (n-1)

val s = List.tabulate(1000, fn x => Int.toString(x));
val l = last s;
print l;

val x = factorial 100;
print (Int.toString(x));
