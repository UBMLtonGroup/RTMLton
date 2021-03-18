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

structure FibTest = BenchTest(Fib)
structure MandelbrotTest = BenchTest(Mandelbrot)
structure Md5Test = BenchTest(Md5)
structure MatrixTest = BenchTest(MatrixMultiply)
structure BarnesHutTest = BenchTest(BarnesHut)
structure EvenOddTest = BenchTest(EvenOdd)
structure TailFibTest = BenchTest(TailFib)
structure ImpForTest = BenchTest(ImpFor)
structure PeekTest = BenchTest(Peek)
structure PsdesTest = BenchTest(PsdesRandom)
structure TakTest = BenchTest(Tak)
structure FlatArrayTest = BenchTest(FlatArray)



val () = FibTest.run 5
val () = TailFibTest.run 5
val () = MandelbrotTest.run 5
val () = Md5Test.run 5
val () = BarnesHutTest.run 5
val () = EvenOddTest.run 5
val () = FlatArrayTest.run 5
val () = ImpForTest.run 5
val () = PeekTest.run 5
val () = PsdesTest.run 2
val () = TakTest.run 5
val () = MatrixTest.run 5
