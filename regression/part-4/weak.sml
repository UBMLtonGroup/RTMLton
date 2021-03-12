structure Weak = MLton.Weak
val _ = print "CheckPoint1\n"

val w = Weak.new 13
val _ =
   if isSome (Weak.get w)
      then raise Fail "bug int"
   else ()

(*
fun testIntInf (i: IntInf.int) =
   let
      val w = Weak.new i
      val _ =
         case Weak.get w of
            NONE => raise Fail "bug IntInf"
          | SOME i => print (concat [IntInf.toString i, "\n"])
   in
      ()
   end
val _ = testIntInf 13
val _ = testIntInf 12345678901234567890
*)

fun testInt (i: string) =
   let
      val w = Weak.new i
      val _ =
         case Weak.get w of
            NONE => raise Fail "bug testInt"
          | SOME i => print (concat [i, "\n"])
   in
      ()
   end
val _ = testInt "13"
val _ = testInt "123456789"

val _ = print "CheckPoint2\n"

val r = ref 13
val n = 2
val rs = Array.tabulate (n, ref)
val ws = Array.tabulate (n, fn i => Weak.new (Array.sub (rs, i)))
fun isAlive i = (print "in isAlive\n"; isSome (Weak.get (Array.sub (ws, i))))

val _ = print "CheckPoint3\n"

(*val _ = MLton.GC.collect ()*)
(*thread.sleep ?? *)
val _ = print "CheckPoint4\n"

val _ =
   if isAlive 0 andalso isAlive 1
      then ()
   else raise Fail "bug1"
val _ = print "CheckPoint5\n"

fun clear i = Array.update (rs, i, r)
fun sub i = ! (Array.sub (rs, i))
val _ = print "CheckPoint6\n"

fun pi x = print (concat [Int.toString x, "\n"])
val _ = print "CheckPoint7\n"

val _ = pi (sub 0 + sub 1)
val _ = valOf (Weak.get (Array.sub (ws, 0))) := 12345
val _ = clear 1
val _ = print "CheckPoint8\n"

val ns = MLton.GC.Statistics.numSweeps ()
val _ = MLton.GC.collect ()
val ns2 = MLton.GC.Statistics.numSweeps ()
val _ = print "CheckPoint9"
val _ =
   if isAlive 0 andalso not (isAlive 1) 
      then ()
   else (print ("HHHHHHH \n"); raise Fail "bug2: first")
val _ = pi (sub 0)
val _ = clear 0
val _ = MLton.GC.collect ()
val _ =
   if not (isAlive 0) andalso not (isAlive 1)
      then ()
   else raise Fail "bug2: second"
