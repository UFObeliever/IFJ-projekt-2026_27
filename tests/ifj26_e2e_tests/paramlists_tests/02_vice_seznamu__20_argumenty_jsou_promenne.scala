import ifj26.*

def add2(a: Int)(b: Int): Int = a + b

@main
def main(): Unit =
  val x = 3
  val y = 4
  val r = add2(x)(y)
  print(r)
