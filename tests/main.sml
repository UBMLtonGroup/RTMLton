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
structure NucleicTest = BenchTest(Nucleic)
structure GCBench = BenchTest(GCBench

val () = FibTest.run 5
val () = MandelbrotTest.run 5
val () = Md5Test.run 5
val () = MatrixTest.run 5
val () = NucleicTest.run 5
val () = GCBench.run 5