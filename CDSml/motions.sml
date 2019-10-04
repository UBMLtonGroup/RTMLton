


structure Motion = 
struct 
  type t = (string * Pos.t * Pos.t);

  val motions : t list = [];
  val add = op ::

  fun getfirstPosition (m :t ) =
  let
    val (cs,Posone,Postwo) = m
  in
    Posone
  end

 fun getsecondPosition (m :t ) =
  let
    val (cs,Posone,Postwo) = m
  in
    Postwo
  end

  fun getAircraft(m :t) = 
  let 
    val (cs,_,_) = m
  in
    cs
  end

  fun printMotion(m :t) = 
  let
    val (cs,p1,p2) = m
  in
    print("Plane "^ cs ^ " going from " ^ Pos.getString(p1) ^ " to "^
    Pos.getString(p2) ^"\n")
  end

  fun printListOfMotions([] :t list) = print ("No motions in list\n")
    |printListOfMotions(h :: tl) = (printMotion(h); printListOfMotions(tl))



  (*returns the intersection of two motions if exists, returns Pos.errocode
  * otherwise *)
  fun findIntersection(one :t, two :t) = 
  let
    val i1 = getfirstPosition(one)
    val f1 = getsecondPosition(one)
    val i2 = getfirstPosition(two)
    val f2 = getsecondPosition(two)
    val r = Constants.proximity_radius
    val vx1 = Pos.getx(f1) - Pos.getx(i1)
    val vx2 = Pos.getx(f2) - Pos.getx(i2)
    val vy1 = Pos.gety(f1) - Pos.gety(i1)
    val vy2 = Pos.gety(f2) - Pos.gety(i2)
    val vz1 = Pos.getz(f1) - Pos.getz(i1)
    val vz2 = Pos.getz(f2) - Pos.getz(i2)
    (* this test is not geometrical 3-d intersection test, it takes the fact that the aircraft move
       into account ; so it is more like a 4d test
       (it assumes that both of the aircraft have a constant speed over the tested interval)
       we thus have two points, each of them moving on its line segment at constant speed ; we are looking
       for times when the distance between these two points is smaller than r 
       V1 is vector of aircraft 1
       V2 is vector of aircraft 2
       if a = 0 iff the planes are moving in parallel and have the same speed (can be zero - they may not be moving at all)
       a = (V2 - V1)^T * (V2 - V1) = < (V2 - V1), (V2 - V1) > =  sqrt( || V2 - V1 || )
    *)
    val a = (vx2 - vx1) * (vx2 - vx1) + (vy2 - vy1) * (vy2 - vy1) + (vz2 - vz1) * (vz2 - vz1);
    
    val b = 2.0 * (Pos.getx(i2) * vx2 - Pos.getx(i2) * vx1 - Pos.getx(i1) * vx2 + Pos.getx(i1) * vx1 + Pos.gety(i2) * vy2 - Pos.gety(i2) * vy1 - Pos.gety(i1) * vy2 + Pos.gety(i1) * vy1 + 
            Pos.getz(i2) * vz2 - Pos.getz(i2) * vz1 - Pos.getz(i1) * vz2 + Pos.getz(i1) * vz1);

    val c = ~r * r + (Pos.getx(i2) - Pos.getx(i1)) * (Pos.getx(i2) - Pos.getx(i1)) + (Pos.gety(i2) - Pos.gety(i1)) * (Pos.gety(i2) - Pos.gety(i1)) + (Pos.getz(i2) - Pos.getz(i1))* (Pos.getz(i2) - Pos.getz(i1));

    val discr = b*b - 4.0 *a *c

    (*the left side*)
    val v1 = ( ~b - Math.sqrt(discr) ) / (2.0 * a)
    (*the right side*)
    val v2 = (~b + Math.sqrt(discr)) / (2.0 * a)

    val x1col = Pos.getx(i1) + vx1 * (v1 + v2) / 2.0;
    val y1col = Pos.gety(i1) + vy1 * (v1 + v2) / 2.0;
    val z1col = Pos.getz(i1) + vz1 * (v1 + v2) / 2.0;
    
    val dist = Math.sqrt((Pos.getx(i2) - Pos.getx(i1)) * (Pos.getx(i2) - Pos.getx(i1)) + (Pos.gety(i2) - Pos.gety(i1)) * (Pos.gety(i2) - Pos.gety(i1)) + (Pos.getz(i2) - Pos.getz(i1)) * (Pos.getz(i2) - Pos.getz(i1)))

  in
    if not (Real.compare(a,0.0) = EQUAL) then
      (* we are first looking for instances of time when the planes are exactly r from each other
	 at least one plane is moving ; if the planes are moving in parallel, they do not have constant speed
	 if the planes are moving in parallel, then
	     if the faster starts behind the slower, we can have 2, 1, or 0 solutions
	     if the faster plane starts in front of the slower, we can have 0 or 1 solutions
	
         if the planes are not moving in parallel, then
	
         point P1 = I1 + vV1
	 point P2 = I2 + vV2
	   - looking for v, such that dist(P1,P2) = || P1 - P2 || = r
			
	 it follows that || P1 - P2 || = sqrt( < P1-P2, P1-P2 > )
	   0 = -r^2 + < P1 - P2, P1 - P2 >
	  from properties of dot product
	   0 = -r^2 + <I1-I2,I1-I2> + v * 2<I1-I2, V1-V2> + v^2 *<V1-V2,V1-V2>
	   so we calculate a, b, c - and solve the quadratic equation
	   0 = c + bv + av^2
	  
           b = 2 * <I1-I2, V1-V2>
           
           c = -r^2 + (I2 - I1)^T * (I2 - I1)
      *
      * *)
      ( if discr < 0.0 then Pos.errorcode else 
        ( if (v1 <= v2 andalso (v1 <= 1.0 andalso 1.0 <= v2 orelse v1 <= 0.0 andalso 0.0 <= v2 orelse 0.0 <= v1 andalso v2 <= 1.0)) then
            (* calculate the location of the collision; if it is outside of the bounds of the Simulation, don't do anything! *)
            ( if (z1col > Constants.MIN_Z andalso z1col <= Constants.MAX_Z
            andalso x1col >= Constants.MIN_X andalso x1col <= Constants.MAX_X
            andalso y1col >= Constants.MIN_Y andalso y1col <= Constants.MAX_Y)
              then (x1col,y1col,z1col) else Pos.errorcode )
          else
            Pos.errorcode 
        )
      )
    else (* a = 0.0*)
        (
            (* the planes have the same speeds and are moving in parallel (or they are not moving at all)
	       they  thus have the same distance all the time ; we calculate it from the initial point
	       dist = || i2 - i1 || = sqrt(  ( i2 - i1 )^T * ( i2 - i1 ) )
            *)

            if dist <=r then getfirstPosition(one) else Pos.errorcode 

        
        )    

  end 


end
