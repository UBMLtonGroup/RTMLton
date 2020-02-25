exception A;
exception B;
exception C;

fun a n = if n > 0 then raise A else 2
fun b n = if n > 0 then raise B else 2
fun c n = if n > 0 then raise C else 2

val _ = print "hi\n"

val _ = a (b (c 2))
  handle A => (print "got A\n" ; 0)
       | B => (print "got B\n" ; 0)
       | C => (print "got C\n" ; 0)


