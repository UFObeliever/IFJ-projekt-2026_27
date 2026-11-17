import ifj26.*

@main
def main(): Unit =
  val x = 5
  val r = !(x > 3) || x * 2 == 10
  print(r)
