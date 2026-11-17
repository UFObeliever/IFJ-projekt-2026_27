import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(-dbl(3)) + " " + toStr(-dbl(3) + 10) + " " + toStr(-(dbl(1) + 1)))
