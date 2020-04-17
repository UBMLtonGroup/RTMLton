
structure Pos = 
struct
  type t = real*real*real

  val zero = (0.0,0.0,0.0)
  val errorcode = (0.001,0.001,0.001)
  
  (*Used for quality of reals as well*)
  fun nearlyEqual (a, b, eps) =
    let 
      val absA = Real.abs a
      val absB = Real.abs b
      val diff = Real.abs (a - b)
    in
      Real.== (a, b) orelse
     ( if Real.== (a, 0.0) orelse
          Real.== (b, 0.0) orelse
          diff < Real.minNormalPos
       then diff < eps * Real.minNormalPos
       else diff / Real.min (absA + absB, Real.maxFinite) < eps )
    end

  fun eq ((a:real,b:real,c:real):t, (d:real,e:real,f:real):t) =
  let
    val (x:real ,y:real ,z :real)= (a,b,c) 
    val (w:real,r:real,s:real)=(d,e,f)
  in
    if (nearlyEqual(a,d,0.1) andalso nearlyEqual(b,e,0.1) andalso
    nearlyEqual(c,f,0.1)) then true else false
  end



  fun getx(x,y,z)=x
  fun gety(x,y,z)=y
  fun getz(x,y,z)=z

  fun getString(p :t)=
    let
        val (a:real,b:real,c:real) = p
    in
        "(" ^ (Real.fmt (StringCvt.FIX NONE) a ) ^","^ (Real.fmt (StringCvt.FIX NONE) b) ^","^ (Real.fmt (StringCvt.FIX NONE) c) ^")"
    end

 end
