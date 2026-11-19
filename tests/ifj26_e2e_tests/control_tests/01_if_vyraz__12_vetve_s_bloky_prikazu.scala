import ifj26.*

@main
def main(): Unit =
  val x = 5
  val r = if x > 0 then
    val t = x * 2
    t + 1
  else
    0
  print(r)
