import ifj26.*

def add2(a: Int)(b: Int): Int = a + b

@main
def main(): Unit =
  val r = add2(3)(4)
  print(r)
