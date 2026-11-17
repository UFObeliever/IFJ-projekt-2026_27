import ifj26.*

def both(a: Boolean)(b: Boolean): Boolean = a && b

@main
def main(): Unit =
  val r = both(true)(false)
  print(r)
