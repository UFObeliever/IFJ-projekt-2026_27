import ifj26.*

def f(x: Int): Int =
  if x < 0 then
    return 0
  return 1

@main def main(): Unit =
  val r = f(1)
  print(r)
