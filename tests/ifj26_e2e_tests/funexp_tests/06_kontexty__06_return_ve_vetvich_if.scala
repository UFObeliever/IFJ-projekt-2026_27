import ifj26.*

def dbl(x: Int): Int = x * 2

def abs2(x: Int): Int =
  if x < 0 then
    return -dbl(x)
  else
    return dbl(x)

@main
def main(): Unit =
  print(toStr(abs2(-2)) + " " + toStr(abs2(3)))
