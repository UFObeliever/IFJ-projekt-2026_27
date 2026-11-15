import ifj26.*

def inc(a: Int): Int =
  val b = a + 1
  return b

@main def main(): Unit =
  val x = 5
  val y = inc(x)
  print(x)
  print("\n")
  print(y)
  print("\n")
