import ifj26.*

@main
def main(): Unit =
  val x = 7
  val ok = x > 5 && x < 10
  if ok then
    print("in")
  else
    print("out")
