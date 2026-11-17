import ifj26.*

@main
def main(): Unit =
  val x = 15
  if x < 10 then
    print("small")
  else if x < 20 then
    if x < 15 then
      print("mid-low")
    else
      print("mid-high")
  else
    print("big")
