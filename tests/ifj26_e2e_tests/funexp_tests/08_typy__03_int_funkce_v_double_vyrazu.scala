import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  print(toStr(toInt(dbl(3) * 1.5)) + " " + toStr(toInt(dbl(3) / 4.0 * 10.0)))
