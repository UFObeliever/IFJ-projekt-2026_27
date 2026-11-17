import ifj26.*

@main
def main(): Unit =
  val r = late(1)(2)
  print(r)

def late(a: Int)(b: Int): Int = a + b
