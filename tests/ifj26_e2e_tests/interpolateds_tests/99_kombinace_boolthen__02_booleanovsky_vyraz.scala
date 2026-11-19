import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = false
  print(s"${a && b}/${a || b}")
