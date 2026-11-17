import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if dbl(1) < dbl(2) then
    print(toStr(dbl(10)))
  else
    print(toStr(dbl(20)))
