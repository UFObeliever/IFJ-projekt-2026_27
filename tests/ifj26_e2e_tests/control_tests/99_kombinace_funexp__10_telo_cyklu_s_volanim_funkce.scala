import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  for i <- 1 to 3 do print(toStr(dbl(i)))
