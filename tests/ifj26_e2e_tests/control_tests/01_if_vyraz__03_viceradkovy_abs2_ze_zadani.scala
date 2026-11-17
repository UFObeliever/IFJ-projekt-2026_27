import ifj26.*

@main
def main(): Unit =
  var x = -5
  val abs2 = if x < 0 then
    0 - x
  else
    x
  print(abs2)
