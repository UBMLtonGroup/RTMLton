functor BenchTest(Test: MICROBENCH) =
struct
fun run n =
  let
      val timer = Timer.startRealTimer ()
      val () = Test.doit n
  in
      print (Test.name ^ ": " ^ Time.toString (Timer.checkRealTimer timer) ^ "\n")
  end

end

structure FibTest = BenchTest(Fib)
structure MandelbrotTest = BenchTest(Mandelbrot)
structure MatrixTest = BenchTest(MatrixMultiply)
structure Md5Test = BenchTest(Md5)
structure NucleicTest = BenchTest(Nucleic)


val () = NucleicTest.run 5
val () = FibTest.run 5
val () = Md5Test.run 5
val () = MandelbrotTest.run 5
val () = MatrixTest.run 5
