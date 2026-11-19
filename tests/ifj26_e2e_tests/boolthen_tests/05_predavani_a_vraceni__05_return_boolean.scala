import ifj26.*

def isBig(n: Int): Boolean =
  if n > 100 then
    return true
  else
    return false

@main
def main(): Unit =
  val a = isBig(200)
  val b = isBig(5)
  print(a)
  print(b)
