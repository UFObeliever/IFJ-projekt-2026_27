import ifj26.*

def f(x: Int): Unit =
  if x < 0 then
    print("n")
  else if x == 0 then
    print("z")

@main def main(): Unit =
  f(1)
  f(0)
  f(0 - 1)
