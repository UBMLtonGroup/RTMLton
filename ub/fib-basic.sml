fun fib 0 = 0
    | fib 1 = 1
    | fib n = fib (n-2) + fib (n-1)

fun fib_tail n = 
    let
    fun fib' (0,a,b) = a
      | fib' (n,a,b) = fib' (n-1,a+b,a)
    in
    fib' (n,0,1)
    end

val _ = fib 6
