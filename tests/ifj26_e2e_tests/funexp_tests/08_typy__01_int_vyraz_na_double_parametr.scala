import ifj26.*

def hd(x: Double): Double = x / 2.0

@main
def main(): Unit =
  if hd(1 + 2) == 1.5 then
    print("y")
  else
    print("n")
