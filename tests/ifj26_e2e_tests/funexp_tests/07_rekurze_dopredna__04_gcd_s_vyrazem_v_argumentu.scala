import ifj26.*

def gcd(a: Int, b: Int): Int =
  if b == 0 then
    return a
  else
    return gcd(b, a - (a / b) * b)

@main
def main(): Unit =
  print(toStr(gcd(48, 18)))
