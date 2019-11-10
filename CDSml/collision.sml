
structure Collision = 
struct 
  (*aircraft one, aircraft two, position where collision happened*)
  type t = string * string  * Pos.t

  fun create (a1,a2,intersection:Pos.t) = (a1,a2,intersection)

  fun empty () :t list = []

  fun getLocation (c :t) = 
  let
    val (one,two,pos) = c
  in
    pos
  end

  fun getAircraftOne(c :t) = 
  let
    val (one,two,pos) =c
  in
    one
  end

  fun getAircraftTwo(c :t) = 
  let
    val (one,two,pos) = c
  in 
    two
  end

end 
