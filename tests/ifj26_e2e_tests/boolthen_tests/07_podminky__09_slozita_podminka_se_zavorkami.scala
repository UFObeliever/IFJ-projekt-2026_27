import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = false
  val c = true
  if (a || b) && !(b && c) then
    print("y")
  else
    print("n")
