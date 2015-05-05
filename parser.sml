(* Lambda calculus definitions *)
datatype Expression = Variable of { name : string }
                    | Abstraction of { argument : string,
                                       body : Expression }
                    | Application of { function : Expression,
                                       argument : Expression }

fun toString (Variable { name })  = "Var(" ^ name ^ ")"
  | toString (Abstraction { argument, body }) =
    String.concat [ "Abs(", argument, ".", toString body, ")" ]
  | toString (Application { function, argument }) =
    String.concat ["App(", toString function, " ", toString argument, ")" ]

(* Parser datatype *)
datatype 'b Parser = Parser of { runParser: char list -> (char list * 'b) option }

fun parse (Parser {runParser}, stream) = runParser stream

(* Monad interface *)
infixr 5 >>=
fun p >>= f =
    let
        fun doParse (stream) =
            case parse (p, stream) of
                SOME (tls, r) => parse (f r, tls)
              | NONE => NONE
    in
        Parser { runParser = doParse }
    end

infixr 5 >>
fun p1 >> p2 = p1 >>= (fn _ => p2)

fun return (v) = Parser {
        runParser = fn (stream) => SOME (stream, v)
    }

val fail = Parser { runParser = fn _ => NONE }

(* Functor *)
infix 3 <$>
fun f <$> p = p >>= (fn x => return (f x))

(* Alternative *)
infix 3 <|>
fun p1 <|> p2  = Parser {
      runParser = fn stream =>
                     case parse (p1, stream) of
                         SOME r => SOME r
                       | NONE => parse (p2, stream)
  }

(* Generic parser combinators *)
fun many (p) =
    let
        fun doParse (st) = case parse (p, st) of
                               NONE => (st, [])
                            |  SOME (tls, a) =>
                               let
                                   val (rst, ss) = doParse tls
                               in
                                   (rst, a :: ss)
                               end
    in
        Parser { runParser = fn stream => SOME (doParse stream) }
    end

fun satisfy (pred) = Parser {
        runParser = fn (stream) =>
                       case stream of
                          a::tls => if (pred a)
                                    then SOME (tls, a)
                                    else NONE
                        | _  => NONE
    }

fun between (pb, p) = Parser {
        runParser = fn (stream) =>
                       case parse (pb >> p, stream) of
                           SOME (tls, r) => parse (pb >> return r, tls)
                         | NONE =>  NONE
    }

fun many1 (p) = p >>= (fn x =>
                many p >>= (fn res =>
                return (x :: res)))

fun delay (f) = Parser {
      runParser = fn (stream) => parse (f (), stream)
  }

(* Char stream parser *)
fun chr (c: char): char Parser =
  let
     fun p (stream) =
         case stream of
             (a :: tls) => if a = c
                           then SOME (tls, a)
                           else NONE
           | _ => NONE
  in
      Parser { runParser = p }
  end

val oneOf = foldr (op <|>) fail o map chr

val space = chr #" "
val spaces = many space
val token = between (spaces, many1 (satisfy Char.isAlpha))

fun charlist (a :: t) =
    chr a >>= (fn c => charlist t >>= (fn ts => return (c::ts)))
  | charlist [] = return []

fun string str = charlist (String.explode str)

fun bracket (bo, bc, p) = bo >> p >>= (fn r =>
                          bc >> return r)

fun braces p = bracket (spaces >> chr #"(", spaces >> chr #")", p)

fun tok s = spaces >>
            string s >>= (fn st =>
            spaces >>
            return (String.implode st))

(* Lambda calculus parser *)
fun abs () = tok "fn" >>
             token >>= (fn a =>
             tok "." >>
             expr () >>= (fn e =>
             return (Abstraction {
                          argument = String.implode a
                        , body = e })))

and arg () = var ()

and var () = token >>= (fn v =>
             return (Variable { name = String.implode v }))

and expr () = delay abs <|> delay app <|> delay paren <|> var ()

and app () = (delay paren <|> var ()) >>= (fn e1 =>
             delay expr >>= (fn e2 =>
             return (Application { function = e1
                                 , argument = e2 })))

and paren () = tok "(" >> delay expr >>= (fn e =>
               tok ")" >> return e)

(* Testing *)
fun lambdaCalc s = case parse (expr (), String.explode s) of
                       SOME (_, r) => toString r
                     | NONE => "FAIL"

val () = print (lambdaCalc "(fn x . x) (x x)")
val () = print "\n"
val () = print "Before collect\n"
val () = MLton.GC.collect()
val () = print (lambdaCalc "(fn x . x) (x x) (x x)")
val () = print "\n"
val () = MLton.GC.collect()
val () = print "Collected\n"

