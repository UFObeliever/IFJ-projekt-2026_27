import ifj26.*

def hd(x: Double): Double = x / 2.0

@main
def main(): Unit =
  val d = hd(5) + 1
  if d == 3.5 then
    print("y")
  else
    print("n")
