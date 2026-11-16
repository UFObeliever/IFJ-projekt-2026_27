import ifj26.*

def f(a: Int): Int =
  if a < 0 then
    if a < 10 then
      return 1
    else
      return 2
  else
    return 3

@main def main(): Unit =
  val m = 0
