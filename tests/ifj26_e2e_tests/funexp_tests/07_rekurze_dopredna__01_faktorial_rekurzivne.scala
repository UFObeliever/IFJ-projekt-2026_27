import ifj26.*

def fact(n: Int): Int =
  if n <= 1 then
    return 1
  else
    return n * fact(n - 1)

@main
def main(): Unit =
  print(toStr(fact(5)))
