import ifj26.*

def f(b: Boolean): Unit =
  if b then
    print("t")

@main def main(): Unit =
  f(1 == 1)
