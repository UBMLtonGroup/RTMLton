structure Weak = MLton.Weak

val w = Weak.new 13
val _ =
   if isSome (Weak.get w)
      then raise Fail "weak1: bug int"
   else print "weak1: ok\n"

fun testInt (i: int) =
   let
      val w = Weak.new i
      val _ =
         case Weak.get w of
            NONE => raise Fail "weak2: bug testInt"
          | SOME i => print (concat ["weak2a: ", Int.toString i, "\n"])
   in
      print ("weak2: " ^ Int.toString(i) ^ "\n")
   end
val _ = testInt 13
val _ = testInt 123456789
      

