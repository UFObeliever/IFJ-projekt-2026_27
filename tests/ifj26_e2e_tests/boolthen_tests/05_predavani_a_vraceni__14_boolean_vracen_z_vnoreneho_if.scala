import ifj26.*

def sgnPositive(x: Int): Boolean =
  if x > 0 then
    return true
  else
    if x < 0 then
      return false
    else
      return false

@main
def main(): Unit =
  val a = sgnPositive(3)
  val b = sgnPositive(-3)
  val c = sgnPositive(0)
  print(a)
  print(b)
  print(c)
