import ifj26.*

@main
def main(): Unit =
  var x = 5
  val m = if x > 0 then
    x * 2
  else 0
  x = -1
  val n = if x > 0 then
    x * 2
  else 0
  print(m)
  print(" ")
  print(n)
