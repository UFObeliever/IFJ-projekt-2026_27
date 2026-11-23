import ifj26.*

def id(b: Boolean): Boolean = b
def g(n: Int): Boolean = id(n > 2)

@main def main(): Unit =
  val r = g(5)
  print(r)
