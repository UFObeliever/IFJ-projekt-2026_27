import ifj26.*

@main
def main(): Unit =
  val x = 0
  var r = 0
  if x < 0 then
    r = -1
  else if x == 0 then
    r = 0
  else
    r = 1
  print(r)
