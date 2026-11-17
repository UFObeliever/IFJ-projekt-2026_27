import ifj26.*

@main
def main(): Unit =
  val x = 0
  val r = if x < 0 then
    -1
  else if x == 0 then
    0
  else
    1
  print(r)
