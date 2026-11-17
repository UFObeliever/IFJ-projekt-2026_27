import ifj26.*

def vol(a: Int, b: Int)(c: Int, d: Int): Int = a * b + c * d

@main
def main(): Unit =
  val r = vol(2, 3)(4, 5)
  print(r)
