import ifj26.*

def safeDiv(a: Int, b: Int): Int = a / b

@main
def main(): Unit =
  val z = 0
  val r = z != 0 && safeDiv(10, z) > 1
  print(r)
