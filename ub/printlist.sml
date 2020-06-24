fun map (f, []) = []
  | map (f, h::t) = f(h) :: map (f, t)

val x = [3, 1, 4]

fun printint x' = print (Int.toString(x') ^ "\n");

val _ = map (printint, x);
