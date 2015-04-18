datatype MyChunk = MyChunk of {
    field1 : int
  , field2 : int
  , field3 : int
  , field4 : int
  , field5 : int
  , field6 : int
  , field7 : int
  , field8 : int
  , fieldi1 : int
  , fieldi2 : int
  , fieldi3 : int
  , fieldi4 : int
  , fieldi5 : int
  , fieldi6 : int
  , fieldi7 : int
  , fieldi8 : int
  , fieldj8 : int
  , fieldk8 : int
  , fieldl8 : int
}


fun print_chunk() =
  let 
    val pt = fn i => ((print o Int.toString) i;
                      print "\n")
    val c = MyChunk {
        field1 = 2147483647
      , field2 = 2147483647
      , field3 = 2147483647
      , field4 = 2147483647
      , field5 = 2147483647
      , field6 = 2147483647
      , field7 = 2147483647
      , field8 = 2147483647
      , fieldi1 = 2147483647
      , fieldi2 = 2147483647
      , fieldi3 = 2147483647
      , fieldi4 = 2147483647
      , fieldi5 = 2147483647
      , fieldi6 = 2147483647
      , fieldi7 = 2147483647
      , fieldi8 = 2147483647
      , fieldj8 = 2147483647
      , fieldk8 = 2147483647
      , fieldl8 = 2147483647
    }

    val d = MyChunk {
        field1 = 2147483646
      , field2 = 2147483645
      , field3 = 2147483644
      , field4 = 2147483641
      , field5 = 2147483642
      , field6 = 2147483643
      , field7 = 2147483644
      , field8 = 2147483645
      , fieldi1 = 2147483646
      , fieldi2 = 2147483642
      , fieldi3 = 2147483641
      , fieldi4 = 2147483633
      , fieldi5 = 2147483641
      , fieldi6 = 2147483643
      , fieldi7 = 2147483642
      , fieldi8 = 2147483632
      , fieldj8 = 2147483632
      , fieldk8 = 2147483632
      , fieldl8 = 2147483632
    }
    val MyChunk {
        field1
      , field2
      , field3
      , field4
      , field5
      , field6
      , field7
      , field8
      , fieldi1 
      , fieldi2 
      , fieldi3 
      , fieldi4 
      , fieldi5 
      , fieldi6 
      , fieldi7 
      , fieldi8 
      , fieldj8 
      , fieldk8 
      , fieldl8  } = d
  in (
      pt  field1;
      pt  field2;
      pt  field3;
      pt  field4;
      pt  field5;
      pt  field6;
      pt  field7;
      pt  field8;
      pt  fieldi1;
      pt  fieldi2;
      pt  fieldi3;
      pt  fieldi4;
      pt  fieldi5;
      pt  fieldi6;
      pt  fieldi7;
      pt  fieldi8;
      pt  fieldj8;
      pt  fieldk8;
      pt  fieldl8
      )
  end

val () = print_chunk ()
