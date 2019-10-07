structure Fib =
struct
val name = "Fib"
val rec fib =
 fn 0 => 0
  | 1 => 1
  | n => fib (n - 1) + fib (n - 2)

fun doit n =
  if n = 0
  then ()
  else let
      val _ = if 165580141 <> fib 41
              then raise Fail "bug"
              else print ("Done\n")
  in
      doit (n - 1)
  end
end


fun timeme f x = 
let
  val start = Time.toMicroseconds(Time.now())
in
  (f x); IntInf.toString(Time.toMicroseconds(Time.now())-start)
end

val _ = print("Time Taken = "^ (timeme Fib.doit 1)^"\n")
