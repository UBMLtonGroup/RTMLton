
structure Voxel=
struct 
  type t = (real*real)

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

 fun printVoxel(v :t) = 
  let
    val (x,y) = v
  in
    print("Voxel : x coordinate "^Real.toString(x)^" y coordinate "^Real.toString(y) ^"\n")
  end

 fun eq ((a:real,b:real), (d:real,e:real)) =
  let
    val (x:real ,y:real)= (a,b) 
    val (w:real,r:real)=(d,e)
  in
    if (nearlyEqual(a,d,0.01) andalso nearlyEqual(b,e,0.01)) then true else false
  end

end

structure VoxelMap  = 
struct 

  type t = Voxel.t * Motion.t list ref;

  fun makemap () : t list = [];


  fun getmotions (v,[]) = ref []
    | getmotions(v,(voxel,ml):: tl) = 
    if Voxel.eq(v,voxel) then ml else getmotions(v,tl)
     
  fun getLength(v)= List.length(v)

(*Takes a voxel as key, motion and Map, adds motion to list of motions in Map
* indexed at voxel
* returns updated map*)
  fun put(v:Voxel.t, m:Motion.t,map:t list) = 
  let
    val motionlist = getmotions(v,map)
    val ml = !motionlist
  in
    if List.null(ml) then ((v, ref [m]) :: map ) else (motionlist := (m :: ml); map)
  end 


  fun retmotion ([]) = []
    |retmotion ((voxel,ml) :: tl) = (ml; retmotion(tl))

  fun getv([]) = []
    | getv((voxel,ml) ::tl) =  !ml :: getv(tl) 

  
  fun getvalues(map) =  getv (!map)
 
end

