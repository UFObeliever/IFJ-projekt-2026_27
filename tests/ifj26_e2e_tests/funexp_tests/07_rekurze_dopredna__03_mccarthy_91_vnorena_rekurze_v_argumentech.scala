import ifj26.*

def mc(n: Int): Int =
  if n > 100 then
    return n - 10
  else
    return mc(mc(n + 11))

@main
def main(): Unit =
  print(toStr(mc(50)) + " " + toStr(mc(150)))
