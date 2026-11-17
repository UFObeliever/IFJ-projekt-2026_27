import ifj26.*

def isPositive(n: Int): Boolean = n > 0

@main
def main(): Unit =
  val r = isPositive(5)
  val s = isPositive(-5)
  print(r)
  print(s)
