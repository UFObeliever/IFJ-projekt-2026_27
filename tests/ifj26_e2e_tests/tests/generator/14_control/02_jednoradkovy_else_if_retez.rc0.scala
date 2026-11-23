import ifj26.*

def f(x: Int): Unit =
  if x < 0 then print("n") else if x == 0 then print("z") else print("p")

@main def main(): Unit =
  f(0 - 2)
  f(0)
  f(2)
