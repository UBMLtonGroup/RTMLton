(*
 try eg:

 ../build/bin/mlton -type-check true -codegen c -cc-opt -O0 -debug true  -verbose 1 arraytest.sml
 ./arraytest @MLton gc-summary max-heap 1000k --
 ./arraytest @MLton gc-summary max-heap 1500k --
 ./arraytest @MLton gc-summary max-heap 1300k --
*)


fun ttime (e, start) = print (e ^ " time taken: " ^ Real.toString(Real.fromLargeInt(Time.toMicroseconds(Time.now())-start)/1000000.00) ^ " s\n")

fun f (x) =
let
   (* val _ = print(Int.toString(x) ^ "\n") *)
   val _ = x * 2
in
   true
end 


fun time_array x = 
let
    val xs = Int.toString(x)
    val start = Time.toMicroseconds(Time.now())
    val a = Array.array(x, 3)
    val _ = ttime(xs ^ " create", start)
    val start = Time.toMicroseconds(Time.now())
    val _ = Array.all f a
    val _ = ttime(xs ^ " walk", start)
in
    ()
end


val _ = time_array 5
val _ = time_array 500
val _ = time_array 500000
val _ = time_array 1687501



