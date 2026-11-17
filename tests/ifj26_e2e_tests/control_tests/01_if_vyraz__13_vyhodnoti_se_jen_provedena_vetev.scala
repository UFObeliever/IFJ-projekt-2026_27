import ifj26.*

@main
def main(): Unit =
  val d = 0
  val r = if d != 0 then 10 / d else 0
  val q = if d == 0 then 0 else 10 / d
  print(r)
  print(q)
