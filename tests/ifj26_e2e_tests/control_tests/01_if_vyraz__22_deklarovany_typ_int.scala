import ifj26.*

@main
def main(): Unit =
  val x = -2
  val r: Int = if x > 0 then x else 0 - x
  print(r)
