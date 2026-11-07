import ifj26.*

def gcd(a: Int, b: Int): Int =
  var x = a
  var y = b
  while y != 0 do
    val t = y
    y = x - x / y * y
    x = t
  return x

@main def main(): Unit =
  val a = readInt()
  val b = readInt()
  val g = gcd(a, b)
  print(g)
