import ifj26.*

def f(a: Int): Int =
  if a < 0 then
    1
  else
    2

@main def main(): Unit =
  print(f(0 - 1))
  print(f(1))
