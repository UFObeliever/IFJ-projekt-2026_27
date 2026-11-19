import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = if a then !a else a
  print(b)
