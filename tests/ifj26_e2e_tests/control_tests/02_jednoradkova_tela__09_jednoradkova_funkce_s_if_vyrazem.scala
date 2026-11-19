import ifj26.*

def sign(x: Int): Int = if x < 0 then -1 else if x == 0 then 0 else 1

@main
def main(): Unit =
  val a = sign(-5)
  val b = sign(0)
  val c = sign(3)
  print(a)
  print(" ")
  print(b)
  print(" ")
  print(c)
