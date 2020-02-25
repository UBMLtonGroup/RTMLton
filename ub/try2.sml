exception A;
exception B;
exception C;

fun a n = if n > 0 then raise A else 2
fun b n = if n > 0 then raise B else 2
fun c n = if n > 0 then raise C else 2

val _ = print "hi\n"

fun d x = 
let
in
  a (b (c 2))
end
handle A => (print "got A\n" ; 0)
handle B => (print "got B\n" ; 0)
handle C => (print "got C\n" ; 0)

val _ = d 1
