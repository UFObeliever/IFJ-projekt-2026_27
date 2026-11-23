import ifj26.*

def f(x: Int): Int =
  if x < 0 then return 0 else return x

@main def main(): Unit =
  print(f(3))
