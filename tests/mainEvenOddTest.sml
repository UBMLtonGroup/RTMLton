functor BenchTest(Test: MICROBENCH) =
struct
fun run n =
  let
    fun timeme ()=
          let
            val start = Time.toMicroseconds(Time.now())
          in
            (Test.doit n);
            Real.toString(Real.fromLargeInt(Time.toMicroseconds(Time.now())-start)/1000000.00 )
          end



  in
    print (Test.name ^ ": " ^ (timeme ()) ^ " s \n")
  end

end

structure EvenOddTest = BenchTest(EvenOdd)
val () = EvenOddTest.run 1
