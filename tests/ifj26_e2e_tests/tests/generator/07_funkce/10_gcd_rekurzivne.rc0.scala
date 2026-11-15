import ifj26.*

def gcd(a: Int, b: Int): Int =
  if b == 0 then
    return a
  else
    val r = a - a / b * b
    val g = gcd(b, r)
    return g

@main def main(): Unit =
  val g = gcd(48, 18)
  print(g)
  print("\n")
