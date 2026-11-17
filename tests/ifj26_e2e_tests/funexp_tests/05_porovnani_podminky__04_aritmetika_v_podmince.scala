import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if dbl(2) + 1 > dbl(1) * 2 then
    print("y")
  else
    print("n")
