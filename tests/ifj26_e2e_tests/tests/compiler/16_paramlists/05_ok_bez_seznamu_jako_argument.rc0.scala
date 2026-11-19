import ifj26.*

def two: Int = 2
def inc(a: Int): Int = a + 1

@main def main(): Unit =
  val x = inc(two)
  print(x)
