import ifj26.*

@main
def main(): Unit =
  val x = 5
  val r = if x > 0 then
    if x > 3 then
      2
    else
      1
  else
    0
  print(r)
