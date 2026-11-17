import ifj26.*

@main
def main(): Unit =
  val x = 1
  val y = -1
  val r1 = 1 + (if x > 0 then 2 else 3)
  val r2 = 1 + (if y > 0 then 2 else 3)
  print(r1)
  print(r2)
