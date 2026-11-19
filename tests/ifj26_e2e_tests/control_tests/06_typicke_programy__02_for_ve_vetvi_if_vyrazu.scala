import ifj26.*

@main
def main(): Unit =
  val x = 4
  val r = if x > 0 then
    var s = 0
    for i <- 1 to x do s = s + i
    s
  else
    0
  print(r)
