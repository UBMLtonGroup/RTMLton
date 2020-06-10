(*
 try eg:

 ../build/bin/mlton -type-check true -codegen c -cc-opt -O0 -debug true  -verbose 1 arraytest.sml
 ./arraytest @MLton gc-summary max-heap 1000k --
 ./arraytest @MLton gc-summary max-heap 1500k --
 ./arraytest @MLton gc-summary max-heap 1300k --
*)

fun f (x) =
let
   val _ = print(Int.toString(x) ^ "\n")
in
   true
end 

val a = Array.array(500000,3);
val _ = Array.all f a


val b = Array.array(300,13);
val _ = Array.all f b



