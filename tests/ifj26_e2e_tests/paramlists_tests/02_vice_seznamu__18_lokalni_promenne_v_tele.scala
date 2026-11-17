import ifj26.*

def calc(a: Int)(b: Int): Int =
  val c = a + b
  val d = c * c
  d - a

@main
def main(): Unit =
  val r = calc(2)(3)
  print(r)
