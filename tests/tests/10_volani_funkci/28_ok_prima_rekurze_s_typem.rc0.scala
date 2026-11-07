import ifj26.*

def fact(n: Int): Int =
  if n < 2 then
    return 1
  else
    val m = n - 1
    val r = fact(m)
    return n * r

@main def main(): Unit =
  val m = 0
