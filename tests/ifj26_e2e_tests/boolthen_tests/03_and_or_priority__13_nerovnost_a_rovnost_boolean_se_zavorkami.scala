import ifj26.*

@main
def main(): Unit =
  val a = 1 < 2
  val b = 3 < 4
  val r = (a && b) == (a || b)
  print(r)
