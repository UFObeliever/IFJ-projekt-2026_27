import ifj26.*

@main
def main(): Unit =
  val x = -3
  val m = if x > 0 then x else
    0 - x
  print(m)
