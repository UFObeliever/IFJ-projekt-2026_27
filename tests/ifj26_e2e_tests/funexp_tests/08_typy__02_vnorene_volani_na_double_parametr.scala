import ifj26.*

def hd(x: Double): Double = x / 2.0

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if hd(dbl(2)) == 2.0 then
    print("y")
  else
    print("n")
