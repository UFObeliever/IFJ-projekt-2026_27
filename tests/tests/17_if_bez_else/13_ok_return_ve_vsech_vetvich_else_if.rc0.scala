import ifj26.*

def f(x: Int): Int =
  if x < 0 then
    return 0
  else if x == 0 then
    return 1
  else
    return 2

@main def main(): Unit =
  val r = f(0)
  print(r)
