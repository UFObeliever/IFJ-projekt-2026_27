import ifj26.*

def max(a: Int, b: Int): Int = if a > b then a else b

@main
def main(): Unit =
  val r1 = max(3, 9)
  val r2 = max(10, 2)
  print(r1)
  print(" ")
  print(r2)
