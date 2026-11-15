import ifj26.*

def fib(n: Int): Int =
  if n < 2 then
    return n
  else
    val a = n - 1
    val b = n - 2
    val x = fib(a)
    val y = fib(b)
    return x + y

@main def main(): Unit =
  val f = fib(20)
  print(f)
  print("\n")
